#pragma once
#include "../src/granular.h"
#include "../src/engine.h"

namespace moss {
// Isolated load generator, never connected to project storage or the UI model.
struct GranularHarness {
  SampleBuffer samples[32];
  GranularTables tables;
  GranularVoice voices[4];
  FxState racks[5];
  Effect effects[5];
  Capture capture;
  unsigned tracks = 4, capacity = GranularVoice::NativeGrains, mode = 0, mask = 10;
  uint64_t frame = 0;
  unsigned untilEdit = 0, edits = 0;
  int peak = 0;
  uint32_t hash = 2166136261u;
  bool hashOutput = false;
  void init(unsigned voiceCount, unsigned grainCount, unsigned effectMask, unsigned scenario = 0) {
    tracks = voiceCount;
    capacity = grainCount;
    mask = effectMask;
    mode = scenario;
    tables.init();
    for (unsigned asset = 0; asset < 32; ++asset) {
      auto &s = samples[asset];
      s.rate = mode == 3 || asset % 2 ? 96000 : 44100;
      s.channels = 2;
      s.frames = SampleBudget / 4;
      s.bytes = SampleBudget;
      uint32_t random = 357 + asset;
      for (unsigned i = 0; i < s.frames * 2; ++i) {
        random = random * 1664525u + 1013904223u;
        s.pcm[i] = int16_t((int(random >> 16) - 32768) / 2);
      }
    }
    for (unsigned i = 0; i < 5; ++i) {
      racks[i].reset();
      auto &e = effects[i];
      e = Effect{};
      e.enabled = effectMask & 1;
      for (unsigned type = 0; type < EffectTypes; ++type) {
        e.enabledTypes[type] = (effectMask >> type) & 1;
        e.order[type] = (type + i) % EffectTypes;
      }
      e.values[0][0] = 7;
      e.values[0][1] = 95;
      e.values[0][3] = 100;
      e.values[1][0] = 100;
      e.values[1][1] = 100;
      e.values[1][3] = 100;
      e.values[2][0] = 100;
      e.values[2][1] = 2;
      e.values[2][2] = 1;
      e.values[2][3] = 100;
      e.values[3][0] = 100;
      e.values[3][1] = 100;
      e.values[3][2] = 100;
      e.values[3][3] = 100;
      racks[i].prepare(e.values, AudioRate, 240);
    }
    for (unsigned i = 0; i < tracks; ++i) {
      voices[i].init(samples[i], tables, AudioRate, capacity, 357 + i * 17);
      voices[i].noteOn(mode == 3 ? 24 : 0);
    }
    frame = 0;
    untilEdit = edits = 0;
    peak = 0;
    hash = 2166136261u;
    capture.begin(AudioRate, AudioRate * 8, -1);
  }
  void edit() {
    for (unsigned i = 0; i < tracks; ++i) {
      GranularParameters p;
      p.density = 512;
      p.sizeMs = mode == 1 ? 1 : mode == 2 && edits % 4 == 3 ? 1 : 500;
      p.pitch = mode == 3 ? 24 : ((edits + i) % 3 - 1.f) * 24;
      p.speed = i % 2 ? -3 : 3;
      p.scatter = 1;
      p.spread = 1;
      p.irregularity = 1;
      p.cutoff = (edits + i) % 2 ? 14000 : 20;
      p.attackMs = p.releaseMs = 20;
      p.freeze = mode == 2 && edits % 7 == 3;
      p.position = ((edits * 13 + i * 7) % 100) / 100.f;
      p.start = mode == 2 ? .3f : 0;
      p.end = mode == 2 ? .3001f : 1;
      voices[i].setParameters(p);
      if (mode == 2 && edits % 31 == 0)
        voices[i].noteOn(float(i * 7) - 12, .8f);
      auto &e = effects[i];
      e.values[0][0] = edits % 8;
      e.values[1][0] = (edits + i) % 2 ? 100 : 30;
      e.values[3][1] = (edits + i) % 2 ? 100 : 0;
      racks[i].prepare(e.values, AudioRate, 240);
    }
    auto &master = effects[4];
    master.values[1][0] = edits % 2 ? 100 : 30;
    master.values[3][1] = edits % 2 ? 100 : 0;
    racks[4].prepare(master.values, AudioRate, 240);
    ++edits;
  }
  void render(int16_t *output, unsigned frames) {
    for (unsigned n = 0; n < frames; ++n, ++frame) {
      if (!untilEdit) {
        edit();
        untilEdit = 500;
      } // 240 BPM, every 1/64 step.
      --untilEdit;
      int mix[2] = {};
      for (unsigned t = 0; t < tracks; ++t) {
        float l, r;
        voices[t].render(l, r);
        int left = int(l * 32768), right = int(r * 32768);
        racks[t].processChain(left, right, effects[t], effects[t].values, AudioRate, 240);
        mix[0] += left;
        mix[1] += right;
      }
      racks[4].processChain(mix[0], mix[1], effects[4], effects[4].values, AudioRate, 240);
      capture.append(mix[0] > 32767    ? 32767
                     : mix[0] < -32768 ? -32768
                                       : mix[0],
                     mix[1] > 32767    ? 32767
                     : mix[1] < -32768 ? -32768
                                       : mix[1]);
      for (unsigned ch = 0; ch < 2; ++ch) {
        int v = Engine::monitor(mix[ch]);
        output[n * 2 + ch] = int16_t(v);
        int magnitude = v < 0 ? -v : v;
        if (magnitude > peak)
          peak = magnitude;
        if (hashOutput) {
          hash ^= uint16_t(v);
          hash *= 16777619u;
        }
      }
    }
  }
};
} // namespace moss
