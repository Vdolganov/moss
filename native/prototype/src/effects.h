#pragma once
#include "dsp_math.h"
#include "effect_defs.h"
#include "fx_character.h"
#include "reel_dsp.h"
#include <math.h>
#include <stdint.h>
#include <string.h>
namespace moss {
struct FxState {
  static constexpr unsigned Delay = 384001;
  float delay[Delay][2] = {};
  unsigned cursor = 0;
  // Avoid placing all eight POND histories on identical L1 cache sets.
  static constexpr unsigned CombStride=8192+16;

  float comb[2][4][CombStride] = {}, filter[2][4] = {}, allpass[2][2][1024] = {};
  unsigned combPos[4] = {}, apPos[2] = {}, heldAt = 0;
  // Independent channel cursors avoid runtime integer division in the sample
  // loop (Cortex-A9 implements integer division in software).
  unsigned combIndex[2][4] = {}, apIndex[2][2] = {}, combLength[2][4] = {};
  unsigned pondRate = 0;
  int pondSize = -1;
  float held[2] = {}, wetness[EffectTypes] = {}, delayTime = 1;
  bool lastEnabled[EffectTypes] = {};
  struct Coefficients {
    float mix = 0, time = 0, cross = 0, feedback = 0, damp = 0;
    float drive = 0, levels = 0, inverseLevels = 0;
    float phase0 = 0, phase1 = 0, envelope = 0;
  } prepared[EffectTypes];
  OrbitDSP orbit;
  DriveDSP drive;
  ReelDSP tape;
  int previousValues[EffectTypes][EffectParameters] = {};
  unsigned preparedRate[EffectTypes] = {};
  int preparedBpm = 0;
  void prepare(const int values[EffectTypes][EffectParameters], unsigned rate, int bpm) {
    for (int type = 0; type < EffectTypes; ++type) {
      const int *p = values[type];
      if (preparedRate[type] == rate && (type != Orbit || preparedBpm == bpm) &&
          !memcmp(previousValues[type], p, sizeof(previousValues[type]))) continue;
      memcpy(previousValues[type], p, sizeof(previousValues[type]));
      preparedRate[type] = rate;
      auto &c = prepared[type];
      c.mix = p[3] / 100.f;
      if (type == 0) {
        orbit.prepare(p, rate, bpm, Delay);
      } else if (type == 1) {
        preparePond(p[0], rate);
        c.feedback = .45f + p[1] * .005f;
        c.damp = .05f + p[2] * .009f;
      } else if (type == 2) {
        c.drive = 1 + p[0] * .15f;
        c.levels = float(1 << (p[1] - 1));
        c.inverseLevels = 1.f / c.levels;
      } else if (type == Reel) {
        tape.prepareReel(p,rate,tape.lastEnabled);
        c.phase0 = .62f / rate;
        c.phase1 = 12.4f / rate;
        c.envelope = 1.f / (rate * .03f);
      } else {
        drive.prepare(p, rate);
      }
    }
    preparedBpm = bpm;
  }
  static float sine(float x) {
    float v = 4 * x * (1 - fabsf(x));
    return .225f * (v * fabsf(v) - v) + v;
  }
  unsigned delayWritten = 0;
  void invalidate() {
    tape.resetReel(tape.motion.rate ? tape.motion.rate : AudioRate,tape.life.random);
    tape.lastEnabled=false;
    preparedRate[Reel] = 0; // A removed/reused SONG track must rebind its controls.
    // Each effect clears its logical history on the next enable edge.
    for (int i = 0; i < EffectTypes; ++i) {
      lastEnabled[i] = false;
      wetness[i] = 0;
    }
  }
  void reset() {
    memset(this, 0, sizeof(*this));
    delayTime = 1;
    tape.resetReel(AudioRate);
    pondSize = -1;
  }
  void preparePond(int sizeValue, unsigned rate) {
    if (pondSize == sizeValue && pondRate == rate)
      return;
    pondSize = sizeValue;
    pondRate = rate;
    static constexpr int lengths[] = {1557, 1617, 1491, 1422};
    float size = .45f + sizeValue * .013f;
    for (int k = 0; k < 4; k++) {
      unsigned len = unsigned(lengths[k] * size * rate / 44100.f);
      if (len > 8100)
        len = 8100;
      if (len < 1)
        len = 1;
      for (int ch = 0; ch < 2; ch++) {
        combLength[ch][k] = len + unsigned(ch * 23);
        combIndex[ch][k] = combPos[k] % combLength[ch][k];
      }
    }
  }
  static float bound(float v) { return v > 1 ? 1 : v < -1 ? -1 : v; }
  static float clean(float v) { return fabsf(v) < 1e-15f ? 0 : v; }
  void process(int &left, int &right, const Effect &e,
               const int *overrideValues, unsigned rate, int bpm) {
    int values[EffectTypes][EffectParameters];
    memcpy(values, e.values, sizeof(values));
    if (overrideValues)
      memcpy(values[e.type], overrideValues, sizeof(values[0]));
    prepare(values, rate, bpm);
    processChain(left, right, e, values, rate, bpm);
  }
  void processChain(int &left, int &right, const Effect &e,
                    const int values[EffectTypes][EffectParameters], unsigned rate, int bpm) {
    float l=left/32768.f, r=right/32768.f;
    for (int type : e.order) {
      bool enabled=e.active(type);
      if (type == Reel) {
        // Fixed transport latency even in bypass, on all four tracks + master.
        // Bypass ramps the motion depth; it never crossfades unaligned audio.
        tape.processReel(l,r,enabled,values[Reel],rate);
        continue;
      }
      if(!enabled && wetness[type]==0) { lastEnabled[type]=false;continue; }
      processFloat(l, r, type, enabled, values[type], rate, bpm);
    }
    // Preserve headroom between inserts; convert only once per complete chain.
    left=int(fxClamp(l,-8,8)*32768);right=int(fxClamp(r,-8,8)*32768);
  }
  void processFloat(float &left, float &right, int type, bool enabled, const int *p,
                   unsigned /*rate*/, int /*bpm*/) {
    const auto &coeff = prepared[type];
    if (enabled && !lastEnabled[type]) {
      if (type == 0) {
        cursor = delayWritten = 0;
        orbit.start();
      }
      if (type == Drive) drive.start();
      if (type == 1) {
        memset(combPos, 0, sizeof(combPos));
        memset(apPos, 0, sizeof(apPos));
        memset(filter, 0, sizeof(filter));
        memset(combIndex, 0, sizeof(combIndex));
        memset(apIndex, 0, sizeof(apIndex));
      }
      if (type == 2) {
        heldAt = 0;
        held[0] = held[1] = 0;
      }
    }
    lastEnabled[type] = enabled;
    float &wet = wetness[type];
    float desired = enabled ? coeff.mix : 0;
    wet += (desired - wet) * .015625f;
    if (!enabled && wet < .00001f) {
      wet = 0;
      return;
    }
    float dry[2] = {left, right}, signal[2] = {};
    if (type == 0) {
      orbit.process(dry, signal, delay, cursor, delayWritten);
    } else if (type == 1) {
      float feedback = coeff.feedback, damp = coeff.damp;
      for (int k = 0; k < 4; k++) {
        for (int ch = 0; ch < 2; ch++) {
          unsigned channelLen = combLength[ch][k];
          unsigned at = combIndex[ch][k];
          float v = combPos[k] < channelLen ? 0 : comb[ch][k][at];
          filter[ch][k] = clean(v * (1 - damp) + filter[ch][k] * damp);
          comb[ch][k][at] = bound(dry[ch] * .25f + filter[ch][k] * feedback);
          signal[ch] += v * .25f;
          if (++combIndex[ch][k] == channelLen)
            combIndex[ch][k] = 0;
        }
        if (++combPos[k] == 0)
          combIndex[0][k] = combIndex[1][k] = 0;
      }
      const unsigned apLength[] = {225, 556};
      for (int k = 0; k < 2; k++) {
        for (int ch = 0; ch < 2; ch++) {
          unsigned len = apLength[k] + unsigned(ch * 11), at = apIndex[ch][k];
          float old = apPos[k] < len ? 0 : allpass[ch][k][at];
          float v = signal[ch];
          signal[ch] = old - v;
          allpass[ch][k][at] = clean(v + old * .5f);
          if (++apIndex[ch][k] == len)
            apIndex[ch][k] = 0;
        }
        if (++apPos[k] == 0)
          apIndex[0][k] = apIndex[1][k] = 0;
      }
    } else if (type == 2) {
      if (!heldAt) {
        for (int ch = 0; ch < 2; ch++) {
          float v = dry[ch] * coeff.drive;
          v = v / (1 + fabsf(v));
          held[ch] = roundf(v * coeff.levels) * coeff.inverseLevels;
        }
      }
      ++heldAt;
      if (heldAt == unsigned(p[2]))
        heldAt = 0;
      else if (heldAt > unsigned(p[2]))
        heldAt %= unsigned(p[2]);
      signal[0] = held[0];
      signal[1] = held[1];
    } else if (type == Drive) {
      drive.process(dry, signal);
    }
    left = dry[0] + wet * (signal[0] - dry[0]);
    right = dry[1] + wet * (signal[1] - dry[1]);
  }
};
} // namespace moss
