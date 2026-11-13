#pragma once
#include "dsp_math.h"
#include "effect_defs.h"
#include <math.h>
#include <stdint.h>
#include <string.h>
namespace moss {
struct FxState {
  static constexpr unsigned Delay = 384001;
  int16_t delay[Delay][2] = {};
  unsigned cursor = 0;
  float comb[2][4][8192] = {}, filter[2][4] = {}, allpass[2][2][1024] = {};
  unsigned combPos[4] = {}, apPos[2] = {}, heldAt = 0;
  // Independent channel cursors avoid runtime integer division in the sample
  // loop (Cortex-A9 implements integer division in software).
  unsigned combIndex[2][4] = {}, apIndex[2][2] = {}, combLength[2][4] = {};
  unsigned pondRate = 0;
  int pondSize = -1;
  float held[2] = {}, wetness[4] = {}, delayTime = 1;
  int16_t reel[4096][2] = {};
  unsigned reelPos = 0, reelWritten = 0;
  float reelPhase[2] = {}, reelTone[2] = {}, reelEnv = 0;
  uint32_t reelRandom = 357;
  bool lastEnabled[4] = {};
  float reelToneCoefficient = .5f;
  int reelWear = -1;
  unsigned reelRate = 0;
  struct Coefficients {
    float mix = 0, time = 0, cross = 0, feedback = 0, damp = 0;
    float drive = 0, levels = 0, inverseLevels = 0;
    float phase0 = 0, phase1 = 0, envelope = 0;
  } prepared[4];
  void prepare(const int values[4][4], unsigned rate, int bpm) {
    for (int type = 0; type < 4; ++type) {
      const int *p = values[type];
      auto &c = prepared[type];
      c.mix = (type == 0 ? p[2] : p[3]) / 100.f;
      if (type == 0) {
        static constexpr float beats[] = {.25f, .5f, .75f, 1, 1.5f, 2, 3, 4};
        c.time = minimum(beats[p[0]] * 60 * rate / bpm, Delay - 2);
        c.cross = p[3] / 100.f;
      } else if (type == 1) {
        preparePond(p[0], rate);
        c.feedback = .45f + p[1] * .005f;
        c.damp = .05f + p[2] * .009f;
      } else if (type == 2) {
        c.drive = 1 + p[0] * .15f;
        c.levels = float(1 << (p[1] - 1));
        c.inverseLevels = 1.f / c.levels;
      } else {
        c.phase0 = .62f / rate;
        c.phase1 = 12.4f / rate;
        c.envelope = 1.f / (rate * .03f);
      }
    }
  }
  static float sine(float x) {
    float v = 4 * x * (1 - fabsf(x));
    return .225f * (v * fabsf(v) - v) + v;
  }
  unsigned delayWritten = 0;
  void invalidate() {
    // Each effect clears its logical history on the next enable edge.
    for (int i = 0; i < 4; ++i) {
      lastEnabled[i] = false;
      wetness[i] = 0;
    }
  }
  void reset() {
    memset(this, 0, sizeof(*this));
    delayTime = 1;
    reelRandom = 357;
    reelWear = -1;
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
    int values[4][4];
    memcpy(values, e.values, sizeof(values));
    if (overrideValues)
      memcpy(values[e.type], overrideValues, sizeof(values[0]));
    prepare(values, rate, bpm);
    for (int type : e.order)
      processType(left, right, type, e.active(type),
                  overrideValues && type == e.type ? overrideValues
                                                   : e.values[type],
                  rate, bpm);
  }
  void processChain(int &left, int &right, const Effect &e,
                    const int values[4][4], unsigned rate, int bpm) {
    for (int type : e.order)
      processType(left, right, type, e.active(type), values[type], rate, bpm);
  }
  void processType(int &left, int &right, int type, bool enabled, const int *p,
                   unsigned rate, int /*bpm*/) {
    const auto &coeff = prepared[type];
    if (enabled && !lastEnabled[type]) {
      if (type == 0) {
        cursor = delayWritten = 0;
        delayTime = 1;
      }
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
      if (type == 3) {
        reelPos = reelWritten = 0;
        reelTone[0] = reelTone[1] = reelEnv = 0;
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
    float dry[2] = {left / 32768.f, right / 32768.f}, signal[2] = {};
    if (type == 0) {
      delayTime += (coeff.time - delayTime) * .002f;
      unsigned whole = unsigned(delayTime);
      float fraction = delayTime - whole;
      unsigned index =
                   cursor >= whole ? cursor - whole : cursor + Delay - whole,
               previous = index ? index - 1 : Delay - 1;
      for (int ch = 0; ch < 2; ch++)
        signal[ch] = whole + 1 <= delayWritten
                         ? (delay[index][ch] * (1 - fraction) +
                            delay[previous][ch] * fraction) /
                               32768.f
                         : 0;
      for (int ch = 0; ch < 2; ch++) {
        float feedback =
            signal[ch] * (1 - coeff.cross) + signal[1 - ch] * coeff.cross;
        delay[cursor][ch] =
            int16_t(bound(dry[ch] + feedback * p[1] / 100.f) * 32767);
      }
      if (++cursor == Delay)
        cursor = 0;
      if (delayWritten < Delay)
        delayWritten++;
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
    } else {
      reelPhase[0] += coeff.phase0;
      reelPhase[1] += coeff.phase1;
      for (float &phase : reelPhase)
        if (phase >= 1)
          phase -= 2;
      float wobble =
          (sine(reelPhase[0]) * .004f + sine(reelPhase[1]) * .00035f) * p[0] /
          100;
      float offset = rate * (.006f + wobble);
      unsigned whole = unsigned(offset);
      float frac = offset - whole;
      unsigned at = (reelPos + 4096 - whole) % 4096, prev = (at + 4095) % 4096;
      reelEnv +=
          (maximum(fabsf(dry[0]), fabsf(dry[1])) - reelEnv) * coeff.envelope;
      reelEnv = clean(reelEnv);
      if (reelWear != p[1] || reelRate != rate) {
        reelWear = p[1];
        reelRate = rate;
        reelToneCoefficient =
            1.f - expf(-6.2831853f * (16000 - p[1] * 148.f) / rate);
      }
      float wear = p[1] / 100.f, tone = reelToneCoefficient;
      for (int ch = 0; ch < 2; ch++) {
        reel[reelPos][ch] = int16_t(bound(dry[ch]) * 32767);
        float v =
            reelWritten > whole + 1
                ? (reel[at][ch] * (1 - frac) + reel[prev][ch] * frac) / 32768.f
                : 0;
        reelTone[ch] = clean(reelTone[ch] + tone * (v - reelTone[ch]));
        v = reelTone[ch] * (1 + wear * 3);
        v = v / (1 + fabsf(v)) / (1 + wear);
        reelRandom ^= reelRandom << 13;
        reelRandom ^= reelRandom >> 17;
        reelRandom ^= reelRandom << 5;
        float noise = (float(reelRandom & 65535) / 32767.5f - 1) * p[2] *
                      .0006f * minimum(1, reelEnv * 12);
        signal[ch] = v * (1 - wear * .1f * (1 + sine(reelPhase[0]))) + noise;
      }
      reelPos = (reelPos + 1) % 4096;
      if (reelWritten < 4096)
        reelWritten++;
    }
    left = int(bound(dry[0] * (1 - wet) + signal[0] * wet) * 32767);
    right = int(bound(dry[1] * (1 - wet) + signal[1] * wet) * 32767);
  }
};
} // namespace moss
