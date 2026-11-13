#pragma once
#include <math.h>
#include <stdint.h>
#include <string.h>
namespace moss {
struct Effect {
  int type = 0, enabled = 0;
  int values[4][4] = {
      {3, 35, 25, 60}, {60, 60, 45, 25}, {20, 12, 2, 25}, {20, 30, 10, 35}};
  int enabledTypes[4] = {}, order[4] = {0, 1, 2, 3};
  bool active(int id) const {
    return id == type ? enabled != 0 : enabledTypes[id] != 0;
  }
  void select(int id) {
    enabledTypes[type] = enabled;
    type = id;
    enabled = enabledTypes[id];
  }
  void move(int id, int direction) {
    for (int i = 0; i < 4; i++)
      if (order[i] == id) {
        int to = i + direction;
        if (to >= 0 && to < 4) {
          int old = order[to];
          order[to] = id;
          order[i] = old;
        }
        break;
      }
  }
};
inline int fxLow(int type, int slot) {
  return type == 2 && slot == 1 ? 2 : type == 2 && slot == 2 ? 1 : 0;
}
inline int fxHigh(int type, int slot) {
  return type == 0 && slot == 0   ? 7
         : type == 0 && slot == 1 ? 95
         : type == 2 && slot == 1 ? 16
         : type == 2 && slot == 2 ? 32
                                  : 100;
}
inline const char *fxName(int type) {
  const char *n[] = {"ORBIT", "POND", "CRUSH", "REEL"};
  return n[type];
}
inline const char *fxLabel(int type, int slot) {
  const char *n[4][4] = {{"TIME", "FEEDBACK", "MIX", "SPREAD"},
                         {"SIZE", "DECAY", "DAMP", "MIX"},
                         {"DRIVE", "BITS", "RATE", "MIX"},
                         {"WOBBLE", "WEAR", "NOISE", "MIX"}};
  return n[type][slot];
}
struct FxState {
  static constexpr unsigned Delay = 384001;
  int16_t delay[Delay][2] = {};
  unsigned cursor = 0;
  float comb[2][4][8192] = {}, filter[2][4] = {}, allpass[2][2][1024] = {};
  unsigned combPos[4] = {}, apPos[2] = {}, heldAt = 0;
  float held[2] = {}, wetness[4] = {}, delayTime = 1;
  int16_t reel[4096][2] = {};
  unsigned reelPos = 0, reelWritten = 0;
  float reelPhase[2] = {}, reelTone[2] = {}, reelEnv = 0;
  uint32_t reelRandom = 357;
  bool lastEnabled[4] = {};
  float reelToneCoefficient = .5f;
  int reelWear = -1;
  unsigned reelRate = 0;
  static float sine(float x) {
    float v = 4 * x * (1 - fabsf(x));
    return .225f * (v * fabsf(v) - v) + v;
  }
  unsigned delayWritten = 0;
  void reset() {
    memset(this, 0, sizeof(*this));
    delayTime = 1;
    reelRandom = 357;
    reelWear = -1;
  }
  static float bound(float v) { return v > 1 ? 1 : v < -1 ? -1 : v; }
  static float clean(float v) { return fabsf(v) < 1e-15f ? 0 : v; }
  void process(int &left, int &right, const Effect &e,
               const int *overrideValues, unsigned rate, int bpm) {
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
                   unsigned rate, int bpm) {
    if (enabled && !lastEnabled[type]) {
      if (type == 0) {
        cursor = delayWritten = 0;
        delayTime = 1;
      }
      if (type == 1) {
        memset(combPos, 0, sizeof(combPos));
        memset(apPos, 0, sizeof(apPos));
        memset(filter, 0, sizeof(filter));
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
    float desired = enabled ? (type == 0 ? p[2] : p[3]) / 100.f : 0;
    wet += (desired - wet) * .015625f;
    if (!enabled && wet < .00001f) {
      wet = 0;
      return;
    }
    float dry[2] = {left / 32768.f, right / 32768.f}, signal[2] = {};
    if (type == 0) {
      const float beats[] = {.25f, .5f, .75f, 1, 1.5f, 2, 3, 4};
      float target = beats[p[0]] * 60 * rate / bpm;
      if (target > Delay - 2)
        target = Delay - 2;
      delayTime += (target - delayTime) * .002f;
      unsigned whole = unsigned(delayTime);
      float fraction = delayTime - whole;
      unsigned index = (cursor + Delay - whole) % Delay,
               previous = (index + Delay - 1) % Delay;
      for (int ch = 0; ch < 2; ch++)
        signal[ch] = whole + 1 <= delayWritten
                         ? (delay[index][ch] * (1 - fraction) +
                            delay[previous][ch] * fraction) /
                               32768.f
                         : 0;
      for (int ch = 0; ch < 2; ch++) {
        float feedback =
            signal[ch] * (1 - p[3] / 100.f) + signal[1 - ch] * p[3] / 100.f;
        delay[cursor][ch] =
            int16_t(bound(dry[ch] + feedback * p[1] / 100.f) * 32767);
      }
      cursor = (cursor + 1) % Delay;
      if (delayWritten < Delay)
        delayWritten++;
    } else if (type == 1) {
      const int lengths[] = {1557, 1617, 1491, 1422};
      float size = .45f + p[0] * .013f, feedback = .45f + p[1] * .005f,
            damp = .05f + p[2] * .009f;
      for (int k = 0; k < 4; k++) {
        unsigned len = unsigned(lengths[k] * size * rate / 44100.f);
        if (len > 8100)
          len = 8100;
        if (len < 1)
          len = 1;
        for (int ch = 0; ch < 2; ch++) {
          unsigned channelLen = len + unsigned(ch * 23);
          unsigned at = combPos[k] % channelLen;
          float v = combPos[k] < channelLen ? 0 : comb[ch][k][at];
          filter[ch][k] = clean(v * (1 - damp) + filter[ch][k] * damp);
          comb[ch][k][at] = bound(dry[ch] * .25f + filter[ch][k] * feedback);
          signal[ch] += v * .25f;
        }
        combPos[k]++;
      }
      const unsigned apLength[] = {225, 556};
      for (int k = 0; k < 2; k++) {
        for (int ch = 0; ch < 2; ch++) {
          unsigned len = apLength[k] + unsigned(ch * 11), at = apPos[k] % len;
          float old = apPos[k] < len ? 0 : allpass[ch][k][at];
          float v = signal[ch];
          signal[ch] = old - v;
          allpass[ch][k][at] = clean(v + old * .5f);
        }
        apPos[k]++;
      }
    } else if (type == 2) {
      if (!heldAt) {
        float drive = 1 + p[0] * .15f;
        int levels = 1 << (p[1] - 1);
        for (int ch = 0; ch < 2; ch++) {
          float v = dry[ch] * drive;
          v = v / (1 + fabsf(v));
          held[ch] = roundf(v * levels) / levels;
        }
      }
      heldAt = (heldAt + 1) % unsigned(p[2]);
      signal[0] = held[0];
      signal[1] = held[1];
    } else {
      reelPhase[0] += .62f / rate;
      reelPhase[1] += 12.4f / rate;
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
      reelEnv += (fmaxf(fabsf(dry[0]), fabsf(dry[1])) - reelEnv) *
                 (1.f / (rate * .03f));
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
                      .0006f * fminf(1, reelEnv * 12);
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
struct Capture {
  static constexpr unsigned MaxFrames = 48000 * 64;
  int16_t pcm[MaxFrames * 2];
  unsigned frames = 0, limit = 0, rate = 44100;
  int source = -1;
  uint32_t state = 0, shownFrames = 0,
           shownLimit = 0; // 0 idle, 1 recording, 2 immutable finished buffer
  uint32_t startedCount = 0, finishedCount = 0;
  void begin(unsigned hz, unsigned length, int src) {
    startedCount++;
    frames = 0;
    rate = hz;
    limit = length > MaxFrames ? MaxFrames : length;
    source = src;
    __atomic_store_n(&shownFrames, 0, __ATOMIC_RELEASE);
    __atomic_store_n(&shownLimit, limit, __ATOMIC_RELEASE);
    __atomic_store_n(&state, 1, __ATOMIC_RELEASE);
  }
  void finish() {
    if (__atomic_load_n(&state, __ATOMIC_ACQUIRE) == 1) {
      finishedCount++;
      __atomic_store_n(&state, 2, __ATOMIC_RELEASE);
    }
  }
  void append(int l, int r) {
    if (__atomic_load_n(&state, __ATOMIC_RELAXED) != 1)
      return;
    if (frames < limit) {
      pcm[frames * 2] = int16_t(l);
      pcm[frames * 2 + 1] = int16_t(r);
      frames++;
    }
    if (frames >= limit)
      finish();
  }
};
} // namespace moss
