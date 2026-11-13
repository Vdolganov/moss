#pragma once
#include "model.h"
#include "stream_preview.h"
#include "synth.h"
namespace moss {
struct Voice {
  const moss::SampleBuffer *sample = nullptr;
  uint64_t phase = 0, increment = 0;
  unsigned start = 0, end = 0, age = 0;
  int key = 0, velocity = 0, level = 0, remaining = 0, release = 0, mode = 0;
  int16_t locks[4] = {Inherit, Inherit, Inherit, Inherit};
  unsigned fadeInFrames = 0, fadeOutSource = 0;
  bool active = false;
  void stop() {
    if (active && !release)
      release = 128;
  }
  void render(int &l, int &r) {
    if (!active)
      return;
    unsigned index = unsigned(phase >> 32);
    if (index >= end) {
      active = false;
      return;
    }
    unsigned next = index + 1 < end ? index + 1 : mode == 2 ? start : index;
    unsigned frac = unsigned((phase >> 16) & 65535);
    int gain = int(age < 128 ? age : 128);
    if (release && release < gain)
      gain = release;
    unsigned edge = (end - start) / 2;
    if (edge > 64)
      edge = 64;
    unsigned dist = index - start;
    if (end - 1 - index < dist)
      dist = end - 1 - index;
    if (edge && dist < edge)
      gain = gain * int(dist) / int(edge);
    if (fadeInFrames && age < fadeInFrames)
      gain = int(int64_t(gain) * age / fadeInFrames);
    if (fadeOutSource && end - 1 - index < fadeOutSource)
      gain = int(int64_t(gain) * (end - 1 - index) / fadeOutSource);
    int *output[] = {&l, &r};
    for (int c = 0; c < 2; c++) {
      int ch = sample->channels == 1 ? 0 : c;
      int a = sample->pcm[index * sample->channels + ch],
          b = sample->pcm[next * sample->channels + ch];
      int value = int((int64_t(a) * (65536 - frac) + int64_t(b) * frac) >> 16);
      *output[c] +=
          int(int64_t(value) * gain * velocity * level / (128 * 127 * 100));
    }
    age++;
    phase += increment;
    if (phase >= (uint64_t(end) << 32)) {
      if (mode == 2)
        phase = (uint64_t(start) << 32) + (phase - (uint64_t(start) << 32)) %
                                              (uint64_t(end - start) << 32);
      else
        active = false;
    }
    if (release && !--release)
      active = false;
  }
};
struct Engine {
  static int monitor(int value) {
    int64_t scaled = int64_t(value) * 135 / 100;
    int sign = scaled < 0 ? -1 : 1;
    int64_t magnitude = scaled * sign;
    constexpr int knee = 27852, headroom = 4915;
    if (magnitude > knee) {
      int64_t excess = magnitude - knee;
      magnitude = knee + excess * headroom / (excess + headroom);
    }
    return int(magnitude) * sign;
  }
  Project project;
  Queue commands;
  const moss::SampleBuffer *samples = nullptr;
  Voice voices[Tracks], tails[Tracks], preview;
  Voice extraVoices[Tracks][Keys - 1], extraTails[Tracks][Keys - 1];
  Voice &voice(int t, int k) { return k ? extraVoices[t][k - 1] : voices[t]; }
  Voice &tail(int t, int k) { return k ? extraTails[t][k - 1] : tails[t]; }
  void stopSamples(int t) {
    for (int k = 0; k < Keys; k++) {
      voice(t, k).stop();
      tail(t, k).stop();
    }
  }
  SynthTables tables;
  SynthVoice synth[Tracks], synthTails[Tracks];
  float muteGain[Tracks] = {1, 1, 1};
  uint64_t rates[32][49] = {};
  unsigned rate = 44100;
  unsigned seenFxMask = 0;
  unsigned seenInstrumentMask = 0;
  int sampleCount = 3;
  FxState fx[Tracks + 1];
  Capture capture;
  StreamPreview stream;
  int activeFxStep[Tracks] = {-1, -1, -1};
  int activeVoiceStep[Tracks] = {-1, -1, -1};
  uint32_t shownStep[Tracks] = {};
  uint64_t clock = 0, frame = 0, tick = 0;
  bool playing = false, first = true;
  uint32_t shownTick = 0, shownPlaying = 0, shownPeak[Tracks] = {},
           triggerCount[Tracks] = {}, lastTrigger[Tracks] = {};
  void init(const Project &p, const moss::SampleBuffer *buffers, unsigned hz,
            int count = 3) {
    preview = Voice{};
    stream.reset();
    project = p;
    samples = buffers;
    rate = hz;
    tables.init();
    sampleCount = count;
    for (auto &f : fx)
      f.reset();
    capture.state = 0;
    for (int t = 0; t < Tracks; t++) {
      activeFxStep[t] = -1;
      activeVoiceStep[t] = -1;
      __atomic_store_n(&shownStep[t], 0, __ATOMIC_RELEASE);
    }
    clock = frame = tick = 0;
    playing = false;
    first = true;
    commands.readIndex = commands.writeIndex = 0;
    for (int t = 0; t < Tracks; t++) {
      voices[t] = Voice{};
      tails[t] = Voice{};
      for (int k = 1; k < Keys; k++) {
        voice(t, k) = Voice{};
        tail(t, k) = Voice{};
      }
      synth[t] = SynthVoice{};
      synthTails[t] = SynthVoice{};
      muteGain[t] = p.tracks[t].mute ? 0 : 1;
      triggerCount[t] = lastTrigger[t] = 0;
    }
    for (int s = 0; s < sampleCount; s++)
      for (int pitch = -24; pitch <= 24; pitch++)
        rates[s][pitch + 24] = uint64_t(double(samples[s].rate) / rate *
                                        pow(2.0, pitch / 12.0) * 4294967296.0);
    __atomic_store_n(&shownPlaying, 0, __ATOMIC_RELEASE);
    __atomic_store_n(&shownTick, 0, __ATOMIC_RELEASE);
  }
  static unsigned probability(uint32_t seed, int track, uint64_t step) {
    uint32_t x = seed ^ uint32_t(step) ^ uint32_t(step >> 32) * 0x85ebca6bu ^
                 uint32_t(track + 1) * 0x9e3779b9u;
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x % 100;
  }
  void settings(int t, const Step *locks, int keyId = 0) {
    auto &v = voice(t, keyId);
    const auto &k = project.tracks[t].keys[v.key];
    if (!v.active)
      return;
    if (k.asset < 0 || k.asset >= sampleCount ||
        &samples[k.asset] != v.sample) {
      v.stop();
      return;
    }
    int params[] = {k.pitch, k.start, k.end, k.level};
    if (locks)
      for (int i = 0; i < 4; i++)
        v.locks[i] = locks->locks[i];
    for (int i = 0; i < 4; i++)
      if (v.locks[i] != Inherit)
        params[i] = v.locks[i];
    params[1] = clamp(params[1], 0, 99);
    params[2] = clamp(params[2], params[1] + 1, 100);
    v.increment = rates[k.asset][clamp(params[0], -24, 24) + 24];
    v.level = params[3];
    v.start = unsigned(uint64_t(v.sample->frames) * params[1] / 100);
    v.end = unsigned(uint64_t(v.sample->frames) * params[2] / 100);
    if (v.end <= v.start)
      v.end = v.start + 1;
    if ((v.phase >> 32) < v.start || (v.phase >> 32) >= v.end)
      v.phase = uint64_t(v.start) << 32;
  }
  void trigger(int t, const Step &step) {
    const auto &tr = project.tracks[t];
    if (tr.instrument != SampleInstrument) {
      int params[8];
      synthSettings(t, &step, params);
      if (!(synth[t].active && synth[t].kind == OrganismInstrument &&
            tr.instrument == OrganismInstrument)) {
        synthTails[t] = synth[t];
        synthTails[t].stop();
        synth[t] = SynthVoice{};
      }
      synth[t].on(tr.instrument, step.note, tables.frequency[step.note],
                  step.velocity, step.length, params,
                  tr.synthMode[tr.instrument - 1], rate,
                  project.seed ^ uint32_t(t + 1) * 73537u ^ uint32_t(tick));
      seenInstrumentMask |= 1u << tr.instrument;
      triggerCount[t]++;
      lastTrigger[t] = uint32_t(frame);
      return;
    }
    const auto &k = project.tracks[t].keys[step.key];
    if (k.asset < 0 || k.asset >= sampleCount || !samples[k.asset].frames)
      return;
    tail(t, step.key) = voice(t, step.key);
    tail(t, step.key).stop();
    voice(t, step.key) = Voice{};
    auto &v = voice(t, step.key);
    v.sample = &samples[k.asset];
    v.active = true;
    v.key = step.key;
    v.velocity = step.velocity;
    v.remaining = step.length;
    v.mode = 0;
    settings(t, &step, step.key);
    v.phase = uint64_t(v.start) << 32;
    triggerCount[t]++;
    lastTrigger[t] = uint32_t(frame);
    seenInstrumentMask |= 1;
  }
  void synthSettings(int t, const Step *step, int *params) {
    const auto &tr = project.tracks[t];
    int type = tr.instrument - 1;
    for (int i = 0; i < 8; i++)
      params[i] = step && step->synthLocks[type][i] != Inherit
                      ? step->synthLocks[type][i]
                      : tr.synthParams[type][i];
  }
  void refreshSynth(int t) {
    const auto &tr = project.tracks[t];
    if (!tr.instrument)
      return;
    int params[8];
    synthSettings(
        t, activeVoiceStep[t] >= 0 ? &tr.steps[activeVoiceStep[t]] : nullptr,
        params);
    synth[t].setTarget(params, tr.synthMode[tr.instrument - 1]);
  }
  void boundary() {
    for (int t = 0; t < project.trackCount; t++) {
      if (synth[t].active && synth[t].gate && !synth[t].previewFrames &&
          --synth[t].remaining <= 0)
        synth[t].off();
      const auto &tr = project.tracks[t];
      int at = int(tick % tr.cycle);
      const Step &s = tr.steps[at];
      bool fired = false;
      if (tr.instrument) {
        fired = s.on && !tr.mute &&
                probability(project.seed, t, tick) < unsigned(s.probability);
        if (fired)
          trigger(t, s);
      } else
        for (int k = 0; k < Keys; k++) {
          const auto &h = tr.hits[k][at];
          bool fire = h.on && !tr.mute &&
                      probability(project.seed ^ uint32_t(k * 173), t, tick) <
                          unsigned(h.probability);
          if (!fire)
            continue;
          Step hit;
          hit.on = 1;
          hit.key = k;
          hit.velocity = h.velocity;
          for (int i = 0; i < 4; i++)
            hit.locks[i] = h.locks[i];
          trigger(t, hit);
          auto &v = voice(t, k);
          v.fadeInFrames = unsigned(uint64_t(h.fadeIn) * rate / 1000);
          v.fadeOutSource =
              unsigned((uint64_t(h.fadeOut) * rate / 1000 * v.increment) >> 32);
          fired = true;
        }
      if (fired)
        activeFxStep[t] = activeVoiceStep[t] = at;
      __atomic_store_n(&shownStep[t], unsigned(at), __ATOMIC_RELEASE);
      refreshSynth(t);
    }
    __atomic_store_n(&shownTick, uint32_t(tick), __ATOMIC_RELEASE);
  }
  void auditionBuffer(const SampleBuffer &s) {
    preview = Voice{};
    if (!s.frames)
      return;
    preview.sample = &s;
    preview.active = true;
    preview.end = s.frames;
    preview.increment = uint64_t(double(s.rate) / rate * 4294967296.0);
    preview.velocity = 100;
    preview.level = 75;
  }
  void handle(Command c) {
    if (c.op == Op::CaptureDiscard) {
      __atomic_store_n(&capture.state, 0, __ATOMIC_RELEASE);
      return;
    }

    if (c.op == Op::CaptureStart) {
      if (__atomic_load_n(&capture.state, __ATOMIC_ACQUIRE))
        return;
      capture.begin(rate,
                    unsigned(uint64_t(rate) * 60 * 4 * c.value / project.bpm),
                    c.track - 1);
      return;
    }
    if (c.op == Op::CaptureStop) {
      capture.finish();
      return;
    }
    if (c.op == Op::Transport || c.op == Op::Stop) {
      __atomic_store_n(&stream.state, 0, __ATOMIC_RELEASE);
      preview.stop();
      playing = c.op == Op::Stop ? false : !playing;
      if (playing) {
        clock = 0;
        tick = 0;
        first = true;
      } else {
        capture.finish();
        preview.stop();
        __atomic_store_n(&stream.state, 0, __ATOMIC_RELEASE);
        for (int t = 0; t < Tracks; t++) {
          stopSamples(t);
          synth[t].stop();
          synthTails[t].stop();
        }
      }
      __atomic_store_n(&shownPlaying, playing ? 1u : 0u, __ATOMIC_RELEASE);
    } else if (c.op == Op::Rewind) {
      clock = tick = 0;
      first = true;
    } else if (c.op == Op::Audition) {
      activeVoiceStep[c.track] = -1;
      Step s;
      s.on = 1;
      s.key = c.index;
      s.note = project.tracks[c.track].audition;
      s.length = 1;
      trigger(c.track, s);
      voice(c.track, c.index).mode = 0;
      if (project.tracks[c.track].instrument)
        synth[c.track].previewFrames = rate * 2;
    } else {
      if (c.op == Op::Instrument) {
        stopSamples(c.track);
        synthTails[c.track] = synth[c.track];
        synthTails[c.track].stop();
        synth[c.track] = SynthVoice{};
        activeFxStep[c.track] = -1;
        activeVoiceStep[c.track] = -1;
      }
      apply(project, c);
      if (c.op == Op::Key)
        settings(c.track, nullptr, c.index);
      if (c.op == Op::Track && c.field == 0) {
        __atomic_store_n(&shownStep[c.track],
                         unsigned(tick % project.tracks[c.track].cycle),
                         __ATOMIC_RELEASE);
        activeFxStep[c.track] = -1;
      }
      if (c.op == Op::Track && c.field == 3 && c.value) {
        stopSamples(c.track);
        synth[c.track].stop();
        synthTails[c.track].stop();
      }
    }
  }
  void render(int16_t *output, unsigned count) {
    Command c;
    while (commands.pop(c))
      handle(c);
    for (int t = 0; t < project.trackCount; t++)
      refreshSynth(t);
    for (int t = 0; t <= Tracks; t++) {
      const Effect &e = t == Tracks ? project.fx : project.tracks[t].fx;
      for (int type = 0; type < 4; type++)
        if (e.active(type) && (t == Tracks || t < project.trackCount))
          seenFxMask |= 1u << type;
    }
    int params[Tracks + 1][4][4];
    auto refreshFx = [&]() {
      for (int t = 0; t <= Tracks; t++) {
        const Effect &e = t == Tracks ? project.fx : project.tracks[t].fx;
        for (int type = 0; type < 4; type++)
          for (int i = 0; i < 4; i++) {
            params[t][type][i] = e.values[type][i];
            if (t < Tracks && activeFxStep[t] >= 0) {
              int v = project.tracks[t].steps[activeFxStep[t]].fxLocks[type][i];
              if (v != Inherit)
                params[t][type][i] = v;
            }
          }
      }
    };
    refreshFx();
    unsigned peaks[Tracks] = {};
    for (unsigned n = 0; n < count; n++, frame++) {
      if (playing) {
        if (first) {
          first = false;
          boundary();
          refreshFx();
        }
        while (clock >= uint64_t(rate) * 60) {
          clock -= uint64_t(rate) * 60;
          tick++;
          boundary();
          refreshFx();
        }
      }
      int mix[2] = {};
      for (int t = 0; t < project.trackCount; t++) {
        int l = 0, r = 0;
        for (int k = 0; k < Keys; k++) {
          voice(t, k).render(l, r);
          tail(t, k).render(l, r);
        }
        synth[t].render(l, r, tables);
        synthTails[t].render(l, r, tables);
        auto &tr = project.tracks[t];
        fx[t].processChain(l, r, tr.fx, params[t], rate, project.bpm);
        muteGain[t] += ((tr.mute ? 0.f : 1.f) - muteGain[t]) * .015625f;
        int lg =
                int(tr.level * (100 - (tr.pan > 0 ? tr.pan : 0)) * muteGain[t]),
            rg =
                int(tr.level * (100 + (tr.pan < 0 ? tr.pan : 0)) * muteGain[t]);
        l = int(int64_t(l) * lg / 10000);
        r = int(int64_t(r) * rg / 10000);
        unsigned peak = unsigned(l < 0 ? -l : l);
        if (unsigned(r < 0 ? -r : r) > peak)
          peak = unsigned(r < 0 ? -r : r);
        if (peak > peaks[t])
          peaks[t] = peak;
        if (capture.source == t)
          capture.append(clamp(l, -32768, 32767), clamp(r, -32768, 32767));
        mix[0] += l;
        mix[1] += r;
      }
      mix[0] = clamp(mix[0] * project.master / 300, -32768, 32767);
      mix[1] = clamp(mix[1] * project.master / 300, -32768, 32767);
      fx[Tracks].processChain(mix[0], mix[1], project.fx, params[Tracks], rate,
                              project.bpm);
      if (capture.source < 0)
        capture.append(mix[0], mix[1]);
      int pl = 0, pr = 0;
      preview.render(pl, pr);
      stream.render(pl, pr);
      output[n * 2] = int16_t(monitor(mix[0] + pl / 2));
      output[n * 2 + 1] = int16_t(monitor(mix[1] + pr / 2));
      if (playing)
        clock += unsigned(project.bpm) * 16;
    }
    __atomic_store_n(&capture.shownFrames, capture.frames, __ATOMIC_RELEASE);
    for (int t = 0; t < Tracks; t++)
      __atomic_store_n(&shownPeak[t], peaks[t], __ATOMIC_RELEASE);
  }
};
} // namespace moss
