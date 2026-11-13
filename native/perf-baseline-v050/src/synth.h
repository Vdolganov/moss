#pragma once
#include "synth_defs.h"
#include <math.h>
#include <stdint.h>
#include <string.h>
namespace moss {
// Original MOSS DSP. The algorithms are inspired by the instrument families,
// not component-accurate emulations or ports of their firmware.
struct SynthTables {
  float sine[2049], frequency[128];
  void init() {
    for (int i = 0; i <= 2048; i++)
      sine[i] = float(sin(6.283185307179586 * i / 2048));
    for (int i = 0; i < 128; i++)
      frequency[i] = float(440 * pow(2, (i - 69) / 12.0));
  }
  static float wrap(float p) {
    int k = int(p);
    if (p < k)
      --k;
    return p - k;
  }
  float sinCycle(float p) const {
    float index = wrap(p) * 2048;
    int i = int(index);
    float f = index - i;
    return sine[i] + (sine[i + 1] - sine[i]) * f;
  }
};
struct SynthVoice {
  bool active = false, gate = false;
  int kind = MonoInstrument, mode = 0, note = 48, remaining = 0;
  unsigned rate = 44100, control = 0, previewFrames = 0, forced = 0,
           forcedTotal = 0;
  float phase[4] = {}, previous[4] = {}, drift[4] = {}, driftTarget[4] = {};
  float lfo[2] = {}, params[8] = {}, target[8] = {}, env = 0, gain = 0,
        targetGain = 0, base = 130.8128f;
  float filter[4] = {}, G = .05f, resonance = 0, attackStep = .001f,
        releaseCoefficient = .999f;
  float fmDecay = 1, fmCoefficient = .999f, ratio = 1, dcIn = 0, dcOut = 0,
        dcCoefficient = .999f;
  float colorState = 0, colorCoefficient = .5f, filterEnv = 1,
        filterDecay = .999f;
  float fir[17] = {};
  unsigned firPos = 0, randomAt = 0;
  uint32_t random = 1;
  static float bounded(float v, float lo, float hi) {
    return v < lo ? lo : v > hi ? hi : v;
  }
  static float saturate(float x) { return x / (1 + fabsf(x)); }
  static float blep(float p, float dt) {
    if (p < dt) {
      p /= dt;
      return p + p - p * p - 1;
    }
    if (p > 1 - dt) {
      p = (p - 1) / dt;
      return p * p + p + p + 1;
    }
    return 0;
  }
  static float saw(float p, float dt) { return 2 * p - 1 - blep(p, dt); }
  static float pulse(float p, float dt) {
    float second = p + .5f;
    if (second >= 1)
      second -= 1;
    return (p < .5f ? 1.f : -1.f) + blep(p, dt) - blep(second, dt);
  }
  float randomBipolar() {
    random ^= random << 13;
    random ^= random >> 17;
    random ^= random << 5;
    return float(random & 65535) / 32767.5f - 1;
  }
  void setTarget(const int *v, int selectedMode) {
    mode = selectedMode;
    for (int i = 0; i < 8; i++)
      target[i] = float(v[i]);
  }
  void on(int type, int midi, float hz, int velocity, int length, const int *v,
          int selectedMode, unsigned sampleRate, uint32_t seed) {
    bool continuous =
        active && kind == OrganismInstrument && type == OrganismInstrument;
    if (!continuous) {
      *this = SynthVoice{};
      kind = type;
      random = seed ? seed : 1;
      phase[1] = .173f;
      phase[2] = .391f;
      phase[3] = .731f;
      for (int i = 0; i < 8; i++)
        params[i] = float(v[i]);
    }
    kind = type;
    note = midi;
    base = hz;
    targetGain = velocity / 127.f;
    remaining = length;
    rate = sampleRate;
    active = gate = true;
    forced = 0;
    previewFrames = 0;
    fmDecay = filterEnv = 1;
    control = 0;
    setTarget(v, selectedMode);
  }
  void off() { gate = false; }
  void stop(unsigned samples = 128) {
    if (active) {
      forced = forcedTotal = samples;
      gate = false;
    }
  }
  void coefficients() {
    dcCoefficient = expf(-50.26548246f / rate);
    if (kind == MonoInstrument) {
      float cut = bounded(params[0] * powf(2, params[4] * .06f * filterEnv), 20,
                          rate * .4f);
      filterDecay = expf(-4.60517f / (rate * (.08f + params[3] * .0004f)));
      float g = tanf(3.141592653589793f * cut / (rate * 2));
      G = g / (1 + g);
      resonance = bounded(params[1] * .039f, 0, 3.9f);
      attackStep =
          1.f /
          (rate * (params[2] > .0f ? fmaxf(.003f, params[2] / 1000) : .003f));
      releaseCoefficient =
          expf(-9.21034037f / (rate * fmaxf(.005f, params[3] / 1000)));
    } else if (kind == OrganismInstrument) {
      attackStep = 1.f / (rate * fmaxf(.003f, params[6] / 1000));
      colorCoefficient =
          1 -
          expf(-6.2831853f * (100 + params[4] * params[4] * 1.2f) / (rate * 2));
      releaseCoefficient =
          expf(-9.21034037f / (rate * fmaxf(.005f, params[7] / 1000)));
    } else {
      attackStep = 1.f / (rate * fmaxf(.003f, params[6] / 1000));
      releaseCoefficient =
          expf(-9.21034037f / (rate * fmaxf(.08f, params[3] / 2000)));
      fmCoefficient =
          expf(-4.605170186f / (rate * fmaxf(.05f, params[3] / 1000)));
      const float ratios[] = {.5f, 1, 1.5f, 2, 3, 4, 5, 7};
      ratio = ratios[int(bounded(target[1], 0, 7))];
    }
  }
  float mono() {
    float dt = bounded(base / (rate * 2), .000001f, .4f),
          x = mode ? pulse(phase[0], dt) : saw(phase[0], dt);
    phase[0] += dt;
    if (phase[0] >= 1)
      phase[0] -= 1;
    phase[1] += dt * .5f;
    if (phase[1] >= 1)
      phase[1] -= 1;
    float sub = 1 - 4 * fabsf(phase[1] - .5f);
    x = (x + sub * params[5] * .014f + randomBipolar() * params[6] * .008f) /
        (1 + params[5] * .006f);
    // Four trapezoidal one-poles with solved global feedback, then bounded
    // drive.
    float g2 = G * G, g3 = g2 * G, g4 = g3 * G;
    float state =
        (1 - G) * (g3 * filter[0] + g2 * filter[1] + G * filter[2] + filter[3]);
    float u = (saturate(x * (1.35f + params[7] * .05f)) - resonance * state) /
              (1 + resonance * g4);
    u = bounded(u, -4, 4);
    for (int i = 0; i < 4; i++) {
      float v = (u - filter[i]) * G;
      u = v + filter[i];
      filter[i] = u + v;
      if (fabsf(filter[i]) < 1e-15f)
        filter[i] = 0;
    }
    return saturate(u * (1 + resonance * .35f)) * 1.1f;
  }
  float organism(const SynthTables &t) {
    float tension = params[0] / 100, depth = params[1] / 100,
          chaos = params[2] / 100, density = params[3] / 100;
    lfo[0] += .073f / (rate * 2);
    lfo[1] += .119f / (rate * 2);
    for (float &v : lfo)
      if (v >= 1)
        v -= 1;
    float slow = t.sinCycle(lfo[0]) * .6f + t.sinCycle(lfo[1]) * .4f;
    if (!randomAt) {
      for (int i = 0; i < 4; i++)
        driftTarget[i] = randomBipolar();
      randomAt = rate / 2;
    }
    randomAt--;
    const float ratios[] = {1, 1, 1.5f, 2};
    float signal[4], sum = 0, weight = 0;
    for (int i = 0; i < 4; i++) {
      drift[i] += (driftTarget[i] - drift[i]) / (rate * .7f);
      int source = mode ? (i + 1) % 4 : (i ^ 1);
      float feedback = previous[source] * tension * .3f +
                       previous[(i + 2) % 4] * chaos * .2f;
      float detune =
          1 + depth * (drift[i] * .008f + slow * (i & 1 ? .004f : -.004f));
      float dt =
          bounded(base * ratios[i] * detune * (1 + feedback) / (rate * 2),
                  .000001f, .4f);
      float triangle = 1 - 4 * fabsf(phase[i] - .5f);
      signal[i] = triangle * (1 - tension * .65f) +
                  pulse(phase[i], dt) * tension * .65f;
      phase[i] += dt;
      if (phase[i] >= 1)
        phase[i] -= 1;
      float w = i == 0 ? 1 : bounded(density * 3 - i + 1, 0, 1);
      sum += signal[i] * w;
      weight += w;
    }
    for (int i = 0; i < 4; i++)
      previous[i] = signal[i];
    float raw = sum / weight + randomBipolar() * params[5] * .009f;
    colorState += (raw - colorState) * colorCoefficient;
    return saturate(colorState * (1 + chaos * 1.2f));
  }
  float fm(const SynthTables &t) {
    float index = params[0] / 100 * (fmDecay + (1 - fmDecay) * params[7] / 100),
          feedback = params[2] / 100;
    // Reduce modulation before the sidebands approach the output Nyquist limit.
    index *= bounded(float(rate) * .12f / (base * (ratio + 1)), .04f, 1);
    float frequency[] = {base, base * ratio, base * (mode ? ratio + 1 : 2.002f),
                         base * (mode ? .5f : ratio * 2.002f)};
    frequency[2] *= 1 + params[4] * .00059463f;
    frequency[3] *= 1 - params[4] * .00028f;
    float excitation = randomBipolar() * params[5] * .0015f;
    for (int i = 0; i < 4; i++) {
      phase[i] += bounded(frequency[i] / (rate * 2), .000001f, .45f);
      if (phase[i] >= 1)
        phase[i] -= 1;
    }
    float m3 = t.sinCycle(phase[3] + previous[3] * feedback * .22f), m1, s;
    if (mode) {
      float m2 = t.sinCycle(phase[2] + m3 * index * .30f);
      m1 = t.sinCycle(phase[1] + m2 * index * .45f + excitation);
      s = t.sinCycle(phase[0] + m1 * index * .70f);
    } else {
      m1 = t.sinCycle(phase[1] + previous[1] * feedback * .22f + excitation);
      s = t.sinCycle(phase[0] + m1 * index * .70f) * .8f +
          t.sinCycle(phase[2] + m3 * index * .35f) * .2f;
    }
    previous[1] = m1;
    previous[3] = m3;
    return s * .65f;
  }
  void render(int &left, int &right, const SynthTables &t) {
    if (!active)
      return;
    for (int i = 0; i < 8; i++)
      params[i] += (target[i] - params[i]) * (1.f / (rate * .005f));
    gain += (targetGain - gain) * (1.f / (rate * .005f));
    if (!control) {
      coefficients();
      control = 32;
    }
    control--;
    if (previewFrames && !--previewFrames)
      off();
    if (gate)
      env = fminf(1, env + attackStep);
    else
      env *= releaseCoefficient;
    if (kind == MonoInstrument) {
      filterEnv *= filterDecay;
      if (filterEnv < 1e-10f)
        filterEnv = 0;
    }
    if (kind == FmInstrument) {
      fmDecay *= fmCoefficient;
      if (fmDecay < 1e-10f)
        fmDecay = 0;
    }
    if (!gate && env < .00001f) {
      active = false;
      return;
    }
    for (int os = 0; os < 2; os++) {
      float sample = kind == MonoInstrument       ? mono()
                     : kind == OrganismInstrument ? organism(t)
                                                  : fm(t);
      fir[firPos] = sample;
      firPos = (firPos + 1) % 17;
    }
    // Windowed-sinc half-band decimation; fixed storage, same path on
    // desktop/ARM.
    static const float h[17] = {
        0, -.005239181f, 0,           .023211101f,  0, -.076105845f,
        0, .307698779f,  .500870292f, .307698779f,  0, -.076105845f,
        0, .023211101f,  0,           -.005239181f, 0};
    float sample = 0;
    for (unsigned i = 0; i < 17; i++)
      sample += h[i] * fir[(firPos + 16 - i) % 17];
    float clean = sample - dcIn + dcCoefficient * dcOut;
    dcIn = sample;
    dcOut = clean;
    float amplitude =
        env * gain *
        (kind == FmInstrument ? (fmDecay + (1 - fmDecay) * params[7] / 100)
                              : 1);
    if (forced) {
      amplitude *= float(forced) / forcedTotal;
      if (!--forced)
        active = false;
    }
    int result = int(bounded(clean * amplitude, -.98f, .98f) * 32767);
    left += result;
    right += result;
  }
};
} // namespace moss
