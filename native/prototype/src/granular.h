#pragma once
#include "sample.h"
#include <math.h>

namespace moss {
// Fixed-pool DSP used by the native GRANULAR instrument and isolated lab.
// Original implementation of conventional granular synthesis; no Mangl code.
struct GranularParameters {
  float speed = 1, pitch = 0, sizeMs = 100, density = 20;
  float position = 0, start = 0, end = 1, scatter = 0;
  float spread = 0, irregularity = 0, cutoff = 12000, level = 1;
  float grainIn = .5f, grainOut = .5f, attackMs = 100, releaseMs = 100;
  bool freeze = false;
};

struct GranularTables {
  static constexpr unsigned Size = 1024;
  float hann[Size + 1] = {}, exp2[257] = {};
  void init() {
    for (unsigned i = 0; i <= Size; ++i)
      hann[i] = float(.5 - .5 * cos(6.283185307179586 * i / Size));
    hann[0] = hann[Size] = 0;
    for (unsigned i = 0; i <= 256; ++i)
      exp2[i] = float(pow(2., double(i) / 128 - 1));
  }
  float densityFactor(float x) const {
    float at = (x + 1) * 128;
    unsigned i = unsigned(at);
    return i >= 256 ? exp2[256] : exp2[i] + (exp2[i + 1] - exp2[i]) * (at - i);
  }
};

// Two cascaded fixed-Q TPT lowpasses. Coefficient ramps preserve the states;
// coefficients are prepared on parameter changes, not per sample.
struct GranularFilter {
  float c[3] = {1, 0, 0}, target[3] = {1, 0, 0}, delta[3] = {};
  float state[2][2][2] = {};
  unsigned ramp = 0, cleanup = 0;
  void tune(float cutoff, unsigned rate, bool immediate = false) {
    float g = tanf(3.14159265358979323846f * cutoff / rate);
    target[0] = 1.f / (1 + g * (g + 1.41421356237f));
    target[1] = g * target[0];
    target[2] = g * target[1];
    ramp = immediate ? 0 : 128;
    for (unsigned i = 0; i < 3; ++i) {
      if (immediate)
        c[i] = target[i];
      delta[i] = (target[i] - c[i]) / 128;
    }
  }
  void process(float &l, float &r) {
    if (ramp) {
      for (unsigned i = 0; i < 3; ++i)
        c[i] += delta[i];
      if (!--ramp)
        memcpy(c, target, sizeof(c));
    }
    float *channels[] = {&l, &r};
    for (unsigned ch = 0; ch < 2; ++ch)
      for (unsigned stage = 0; stage < 2; ++stage) {
        float *s = state[ch][stage];
        float v3 = *channels[ch] - s[1];
        float v1 = c[0] * s[0] + c[1] * v3;
        float v2 = s[1] + c[1] * s[0] + c[2] * v3;
        s[0] = 2 * v1 - s[0];
        s[1] = 2 * v2 - s[1];
        *channels[ch] = v2;
      }
    if (++cleanup == 32) {
      cleanup = 0;
      for (auto &channel : state)
        for (auto &stage : channel)
          for (float &v : stage)
            if (fabsf(v) < 1e-20f)
              v = 0;
    }
  }
  void clear() {
    memset(state, 0, sizeof(state));
  }
};

struct GranularVoice {
  // Conservative RG35XX integration budget; larger pools are for measurement only.
  static constexpr unsigned NativeGrains = 8;
  static constexpr unsigned MaxGrains = 32;
  // Nominal RMS for decorrelated grains relative to their source. Prepared
  // once on edits, never an input-dependent compressor or per-sample sqrt.
  static constexpr float NominalGain = .75f;
  struct Grain {
    unsigned index = 0, stepWhole = 0, age = 0, length = 0;
    unsigned first = 0, last = 1, zone = 0;
    float fraction = 0, stepFraction = 0;
    unsigned attackFrames = 1, releaseStart = 1;
    float attackStep = 0, releaseStep = 0;
    float origin = 0, panLeft = 1, panRight = 1;
  } grains[MaxGrains];
  struct Statistics {
    uint64_t attempts = 0, spawned = 0, skipped = 0;
    unsigned peak = 0;
  } stats;
  const SampleBuffer *sample = nullptr;
  const GranularTables *tables = nullptr;
  GranularParameters parameters;
  GranularFilter filter;
  unsigned rate = 32000, capacity = NativeGrains, count = 0, grainFrames = 3200;
  unsigned control = 0;
  uint32_t randomState = 1;
  int64_t head = 0, scanFirst = 0, scanLast = 0, scanStep = 0;
  float readStep = 1, note = 0, velocity = 1, envelope = 0, attackStep = 1, releaseStep = 1;
  float outputGain = 0, gainTarget = 1, gainSmoothing = .001f;
  float currentVelocity = 0;
  float scheduler = 0, schedulerStep = 0, noise = 0, noiseTarget = 0;
  float noiseStep = 0, noiseClock = 1;
  bool gate = false, running = false;

  static float bounded(float v, float lo, float hi, float fallback) {
    if (!isfinite(v))
      return fallback;
    return v < lo ? lo : v > hi ? hi : v;
  }
  float randomBipolar() {
    randomState ^= randomState << 13;
    randomState ^= randomState >> 17;
    randomState ^= randomState << 5;
    return float(randomState >> 8) * (1.f / 8388608.f) - 1;
  }
  bool init(const SampleBuffer &source, const GranularTables &lookup, unsigned outputRate,
            unsigned limit = NativeGrains, uint32_t seed = 357) {
    *this = GranularVoice{};
    if (outputRate < 8000 || outputRate > 96000 || !limit || limit > MaxGrains ||
        source.rate < 8000 || source.rate > 96000 || !source.frames ||
        (source.channels != 1 && source.channels != 2) ||
        source.frames > SampleBudget / (2 * source.channels))
      return false;
    sample = &source;
    tables = &lookup;
    rate = outputRate;
    gainSmoothing = 1.f - expf(-1.f / (rate * .02f));
    capacity = limit;
    randomState = seed ? seed : 1;
    setParameters(parameters);
    filter.tune(parameters.cutoff, rate, true);
    return true;
  }
  void preparePitch() {
    float semitones = bounded(parameters.pitch + note, -48, 48, 0);
    readStep = sample ? float(double(sample->rate) / rate * pow(2., semitones / 12.)) : 1;
  }
  void setParameters(GranularParameters p) {
    p.speed = bounded(p.speed, -3, 3, 0);
    p.pitch = bounded(p.pitch, -24, 24, 0);
    p.sizeMs = bounded(p.sizeMs, 1, 500, 100);
    p.density = bounded(p.density, 0, 512, 20);
    p.position = bounded(p.position, 0, 1, 0);
    p.start = bounded(p.start, 0, 1, 0);
    p.end = bounded(p.end, p.start, 1, 1);
    p.scatter = bounded(p.scatter, 0, 1, 0);
    p.spread = bounded(p.spread, 0, 1, 0);
    p.irregularity = bounded(p.irregularity, 0, 1, 0);
    p.cutoff = bounded(p.cutoff, 20, rate * .4375f, rate * .375f);
    p.level = bounded(p.level, 0, 2, 1);
    p.grainIn = bounded(p.grainIn, 0, 1, .5f);
    p.grainOut = bounded(p.grainOut, 0, 1, .5f);
    float sum = p.grainIn + p.grainOut;
    if (sum > 1) { p.grainIn /= sum; p.grainOut /= sum; }
    p.attackMs = bounded(p.attackMs, 0, 9000, 100);
    p.releaseMs = bounded(p.releaseMs, 0, 9000, 100);
    bool seekChanged = p.position != parameters.position;
    bool filterChanged = p.cutoff != parameters.cutoff;
    parameters = p;
    grainFrames = unsigned(p.sizeMs * .001f * rate + .5f);
    if (grainFrames < 2)
      grainFrames = 2;
    attackStep = 1.f / (rate * (p.attackMs < 2 ? .002f : p.attackMs * .001f));
    releaseStep = 1.f / (rate * (p.releaseMs < 2 ? .002f : p.releaseMs * .001f));
    float overlap = bounded(p.density * p.sizeMs * .001f, 1, float(capacity), 1);
    float windowEnergy = 1 - .625f * (p.grainIn + p.grainOut);
    gainTarget = p.level * NominalGain / sqrtf(fmaxf(1, overlap * windowEnergy));
    if (filterChanged)
      filter.tune(p.cutoff, rate);
    preparePitch();
    if (sample) {
      unsigned first = unsigned(p.start * sample->frames);
      if (first >= sample->frames)
        first = sample->frames - 1;
      unsigned last = unsigned(p.end * sample->frames);
      if (last <= first)
        last = first + 1;
      scanFirst = int64_t(first) << 32;
      scanLast = int64_t(last) << 32;
      scanStep =
          int64_t(double(sample->rate) / rate * p.speed * 4294967296.0) % (scanLast - scanFirst);
      if (head < scanFirst || head >= scanLast)
        head = scanFirst;
      if (seekChanged)
        seek(p.position);
    }
    // Preserve event/noise phases across edits and render-block boundaries.
    control = 0;
  }
  void seek(float normalized) {
    if (!sample)
      return;
    normalized = bounded(normalized, 0, 1, 0);
    head = int64_t(double(normalized) * sample->frames * 4294967296.0);
    if (head < scanFirst)
      head = scanFirst;
    if (head >= scanLast)
      head = scanLast - 1;
  }
  void noteOn(float semitones = 0, float strength = 1) {
    if (!sample)
      return;
    note = bounded(semitones, -48, 48, 0);
    velocity = bounded(strength, 0, 1, 1);
    preparePitch();
    gate = running = true;
    scheduler = 1;
  }
  void noteOff() {
    gate = false;
  }
  void stop() {
    gate = running = false;
    count = 0;
    envelope = 0;
    filter.clear();
  }
  void spawn() {
    ++stats.attempts;
    if (count == capacity) {
      ++stats.skipped;
      return; // Never steal a grain or enlarge the pool.
    }
    // Stratify births across eight regions. A randomly chosen least-occupied
    // region prevents a full cloud clustering around one scan head. Zone is
    // latched per grain, so replacement preserves coverage without ordered scans.
    unsigned population[NativeGrains] = {};
    for (unsigned i = 0; i < count; ++i) ++population[grains[i].zone];
    unsigned least = MaxGrains, candidates[NativeGrains], choices = 0;
    for (unsigned i = 0; i < NativeGrains; ++i) {
      if (population[i] < least) { least = population[i]; choices = 0; }
      if (population[i] == least) candidates[choices++] = i;
    }
    unsigned zone = candidates[unsigned((randomBipolar() + 1) * .5f * choices)];
    double fraction = (zone + (randomBipolar() + 1) * .5) / NativeGrains;
    unsigned first = unsigned(scanFirst >> 32), last = unsigned(scanLast >> 32);
    unsigned span = last - first;
    // At full scatter the anchor, scan speed and freeze do not bias births.
    // At smaller widths the window is centered on the head and wraps locally.
    double position = first + fraction * span;
    if (parameters.scatter < 1) {
      position = double(head >> 32) + double(uint32_t(head)) / 4294967296. +
                 (fraction - .5) * parameters.scatter * span;
      if (position < first) position += span;
      else if (position >= last) position -= span;
    }
    Grain &g = grains[count++];
    g.index = unsigned(position);
    g.fraction = float(position - g.index);
    g.first = first; g.last = last; g.zone = zone;
    g.origin = float(position / sample->frames);
    float balance = randomBipolar() * parameters.spread;
    g.panLeft = sqrtf(1 - balance); g.panRight = sqrtf(1 + balance);
    // Tiny sources are valid. Reduce the integer stride once at birth.
    g.stepWhole = unsigned(readStep) % span;
    g.stepFraction = readStep - unsigned(readStep);
    g.age = 0;
    g.length = grainFrames;
    g.attackFrames = unsigned((g.length - 1) * parameters.grainIn);
    unsigned releaseFrames = unsigned((g.length - 1) * parameters.grainOut);
    if (!g.attackFrames) g.attackFrames = 1;
    if (!releaseFrames) releaseFrames = 1;
    // Even nominally zero fades retain one endpoint sample. Take that guard
    // from the longer ramp rather than overlapping attack and release.
    if (g.attackFrames + releaseFrames > g.length - 1) {
      if (g.attackFrames >= releaseFrames) g.attackFrames = g.length - 1 - releaseFrames;
      else releaseFrames = g.length - 1 - g.attackFrames;
    }
    g.releaseStart = g.length - 1 - releaseFrames;
    g.attackStep = float(GranularTables::Size / 2) / g.attackFrames;
    g.releaseStep = float(GranularTables::Size / 2) / releaseFrames;
    ++stats.spawned;
    if (count > stats.peak)
      stats.peak = count;
  }
  float grainWindow(const Grain &g) const {
    if (!g.age || g.age + 1 >= g.length) return 0;
    float phase;
    if (g.age < g.attackFrames) phase = g.age * g.attackStep;
    else if (g.age >= g.releaseStart)
      phase = GranularTables::Size / 2 + (g.age - g.releaseStart) * g.releaseStep;
    else return 1;
    unsigned at = unsigned(phase);
    return at >= GranularTables::Size ? 0 : tables->hann[at] +
      (tables->hann[at + 1] - tables->hann[at]) * (phase - at);
  }
  void render(float &left, float &right) {
    left = right = 0;
    if (!running || !sample)
      return;
    if (gate) {
      envelope += attackStep;
      if (envelope > 1)
        envelope = 1;
    } else {
      envelope -= releaseStep;
      if (envelope <= 0) {
        stop();
        return;
      }
    }
    if (!parameters.freeze) {
      head += scanStep;
      if (head >= scanLast)
        head -= scanLast - scanFirst;
      else if (head < scanFirst)
        head += scanLast - scanFirst;
    }
    if (!control) {
      control = 32;
      float controlPeriod = 32.f / rate;
      noiseClock += parameters.density * controlPeriod;
      if (noiseClock >= 1) {
        noiseClock -= floorf(noiseClock);
        noiseTarget = randomBipolar();
        noiseStep = (noiseTarget - noise) * parameters.density * controlPeriod;
      }
      noise = bounded(noise + noiseStep, -1, 1, 0);
      schedulerStep =
          parameters.density / rate * tables->densityFactor(noise * parameters.irregularity);
    }
    --control;
    if (parameters.density > 0) {
      if (scheduler >= 1) {
        scheduler -= 1;
        spawn();
      }
      scheduler += schedulerStep;
    }
    for (unsigned i = 0; i < count;) {
      Grain &g = grains[i];
      float window = grainWindow(g);
      unsigned next = g.index + 1 == g.last ? g.first : g.index + 1;
      unsigned at = g.index * sample->channels, after = next * sample->channels;
      float l = sample->pcm[at];
      l += (sample->pcm[after] - l) * g.fraction;
      float r = l;
      if (sample->channels == 2) {
        r = sample->pcm[at + 1];
        r += (sample->pcm[after + 1] - r) * g.fraction;
      }
      left += l * (window * g.panLeft);
      right += r * (window * g.panRight);
      g.fraction += g.stepFraction;
      g.index += g.stepWhole;
      if (g.fraction >= 1) {
        g.fraction -= 1;
        ++g.index;
      }
      if (g.index >= g.last)
        g.index -= g.last - g.first;
      if (++g.age == g.length)
        grains[i] = grains[--count];
      else
        ++i;
    }
    outputGain += (gainTarget - outputGain) * gainSmoothing;
    left *= outputGain / 32768.f;
    right *= outputGain / 32768.f;
    filter.process(left, right);
    currentVelocity += (velocity - currentVelocity) * .015625f;
    left *= envelope * currentVelocity;
    right *= envelope * currentVelocity;
  }
  void renderBlock(float *stereo, unsigned frames) {
    for (unsigned n = 0; n < frames; ++n)
      render(stereo[n * 2], stereo[n * 2 + 1]);
  }
};
} // namespace moss
