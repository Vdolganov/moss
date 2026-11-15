#pragma once
#include "reel_tables.h"
#include <math.h>
#include <stdint.h>

namespace moss {
// Stage-one tape transport. Full-wet, stereo-linked; no colour, noise, clipping,
// gain change or dry/wet comb filtering. All instances reserve the same latency.
struct ReelMotionParams {
  float wowCents = 0, wowHz = .3f, flutterCents = 0, flutterHz = 6;
  float amount = 1;
};
struct ReelMotion {
  static constexpr unsigned Size = 4096, Mask = Size - 1;
  static constexpr float LatencySeconds = .016f;
  static constexpr float MaxDelaySlew = .026f; // bounded speed during hard locks
  float history[Size][2] = {};
  struct Modulator {
    uint32_t phase = 0, idleIncrement = 0;
    float increment = 0, targetIncrement = 0;
    float amplitude = 0, targetAmplitude = 0;
  } mod[2];
  unsigned rate = 0, cursor = 0, written = 0, latency = 0;
  float smoothing = 0, position = 0;
  bool moving = false;

  static float clamp(float v, float lo, float hi) {
    return !isfinite(v) ? lo : v < lo ? lo : v > hi ? hi : v;
  }
  void clearHistory() { cursor = written = 0; }
  void reset(unsigned hz) {
    rate = hz < 8000 ? 8000 : hz > 96000 ? 96000 : hz;
    latency = unsigned(rate * LatencySeconds + .5f);
    position = float(latency);
    smoothing = 1.f / (1.f + rate * .05f);
    mod[0] = Modulator{}; mod[1] = Modulator{};
    moving = false;
    clearHistory(); // Logical clear: no large memset on an audio event.
  }
  void prepare(const ReelMotionParams &p, unsigned hz) {
    unsigned validRate = hz < 8000 ? 8000 : hz > 96000 ? 96000 : hz;
    if (rate != validRate) reset(validRate);
    const float frequencies[] = {clamp(p.wowHz, .1f, 4.f),
                                 clamp(p.flutterHz, 2.f, 30.f)};
    const float depths[] = {clamp(p.wowCents, 0, 15),
                            clamp(p.flutterCents, 0, 15)};
    float amount = clamp(p.amount, 0, 1);
    for (unsigned k = 0; k < 2; ++k) {
      auto &m = mod[k];
      m.targetIncrement = frequencies[k] * (4294967296.f / rate);
      m.idleIncrement = uint32_t(m.targetIncrement);
      if (!m.increment) m.increment = m.targetIncrement;
      // Depth describes speed deviation in cents, not a delay in milliseconds.
      // Dividing by frequency keeps steady-state depth stable across rate edits.
      m.targetAmplitude = (exp2f(depths[k] * amount / 1200.f) - 1.f) *
                          rate / (6.28318530718f * frequencies[k]);
    }
    moving = mod[0].targetAmplitude != 0 || mod[1].targetAmplitude != 0 ||
             mod[0].amplitude != 0 || mod[1].amplitude != 0 || position != float(latency);
  }
  static float sine(uint32_t phase) {
    unsigned cell = phase >> 24;
    float fraction = (phase & 0xffffffu) * (1.f / 16777216.f);
    return ReelSine[cell] + fraction * (ReelSine[cell + 1] - ReelSine[cell]);
  }
  float nextPosition() {
    float target = float(latency);
    for (auto &m : mod) {
      m.increment += smoothing * (m.targetIncrement - m.increment);
      m.phase += uint32_t(m.increment);
      m.amplitude += smoothing * (m.targetAmplitude - m.amplitude);
      if (m.targetAmplitude == 0 && m.amplitude < 1e-5f) m.amplitude = 0;
      target += m.amplitude * sine(m.phase);
    }
    // Changing depth or rate also moves the head. Limit that extra velocity so
    // abrupt parameter locks cannot produce read jumps or extreme pitch bursts.
    float delta = target - position;
    position += delta < -MaxDelaySlew ? -MaxDelaySlew :
                delta > MaxDelaySlew ? MaxDelaySlew : delta;
    return position;
  }
  float sample(unsigned distance, unsigned channel) const {
    return distance < written ? history[(cursor - distance) & Mask][channel] : 0;
  }
  void read(float delay, float &left, float &right) const {
    unsigned whole = unsigned(delay);
    float phase = (delay - whole) * 256.f;
    unsigned cell = unsigned(phase);
    float fraction = phase - cell;
#if defined(__arm__) && defined(__ARM_NEON)
    // Same 12-tap kernel as the portable path. Process four stereo taps at
    // once; only the ring seam / initial warmup need individual bounds checks.
    unsigned start = (cursor - whole - 6) & Mask;
    if (written == Size && start <= Size - 12) {
      const float *input = &history[start][0], *a = ReelSinc[cell],
                  *b = ReelSinc[cell + 1];
      float output[2]; unsigned groups = 3;
      // Zig 0.14.1 cannot lower NEON intrinsics with our gnueabi flags. Use an
      // explicit Cortex-A9 kernel without changing the build's calling ABI.
      // Declare D-register aliases: Q clobbers are not preserved by this target.
      asm volatile(
        ".fpu neon\n\t"
        "veor q0, q0, q0\n\t"
        "veor q1, q1, q1\n\t"
        "vld1.32 {d14[], d15[]}, [%[fraction]]\n\t"
        "1:\n\t"
        "vld2.32 {d4-d7}, [%[input]]!\n\t"
        "vld1.32 {d8-d9}, [%[a]]!\n\t"
        "vld1.32 {d10-d11}, [%[b]]!\n\t"
        "vsub.f32 q5, q5, q4\n\t"
        "vmla.f32 q4, q5, q7\n\t"
        "vmla.f32 q0, q2, q4\n\t"
        "vmla.f32 q1, q3, q4\n\t"
        "subs %[groups], %[groups], #1\n\t"
        "bne 1b\n\t"
        "vadd.f32 d0, d0, d1\n\t"
        "vadd.f32 d2, d2, d3\n\t"
        "vpadd.f32 d0, d0, d0\n\t"
        "vpadd.f32 d2, d2, d2\n\t"
        "vzip.32 d0, d2\n\t"
        "vst1.32 {d0}, [%[output]]\n\t"
        ".fpu vfpv3-fp16\n\t"
        : [input] "+&r"(input), [a] "+&r"(a), [b] "+&r"(b), [groups] "+&r"(groups)
        : [fraction] "r"(&fraction), [output] "r"(output)
        : "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
          "d8", "d9", "d10", "d11", "d14", "d15", "cc", "memory");
      left = output[0]; right = output[1];
      return;
    }
#endif
    left = right = 0;
    for (unsigned k = 0; k < 12; ++k) {
      float coefficient = ReelSinc[cell][k] + fraction *
                          (ReelSinc[cell + 1][k] - ReelSinc[cell][k]);
      unsigned distance = whole + 6 - k;
      if (distance < written) {
        unsigned at = (cursor - distance) & Mask;
        left += coefficient * history[at][0];
        right += coefficient * history[at][1];
      }
    }
  }
  void process(float &left, float &right) {
    history[cursor][0] = left; history[cursor][1] = right;
    if (written < Size) ++written;
    if (!moving) {
      // Keep the motor phases running without doing silent interpolation or
      // per-sample floating-point smoothing in a clean/bypassed transport.
      for (auto &m : mod) {m.phase += m.idleIncrement; m.increment = m.targetIncrement;}
      left = sample(latency, 0); right = sample(latency, 1);
    } else {
      float delay = nextPosition();
      if (delay == float(latency) && mod[0].amplitude == 0 && mod[1].amplitude == 0) {
        moving = false;
        left = sample(latency, 0); right = sample(latency, 1);
      } else read(delay, left, right);
    }
    cursor = (cursor + 1) & Mask;
  }
};
} // namespace moss
