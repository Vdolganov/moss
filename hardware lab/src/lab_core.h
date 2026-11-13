#ifndef MOSS_LAB_CORE_H
#define MOSS_LAB_CORE_H
#include <stdint.h>
#include <math.h>
#include <string.h>

namespace lab {
constexpr int W = 640, H = 480;
enum Button { Up, Down, Left, Right, A, B, X, Y, Start, Select, L1, R1, L2, R2, Menu, ButtonCount };
static const char *const names[ButtonCount] = {"UP", "DOWN", "LEFT", "RIGHT", "A",
                                               "B",  "X",    "Y",    "START", "SELECT",
                                               "L1", "R1",   "L2",   "R2",    "MENU"};
// Original RG35XX Linux input codes, verified against MinUI-Legacy-RG35XX defines.h.
inline int rawButton(int code) {
  return code >= 0x5a && code <= 0x68 ? code - 0x5a : -1;
}
inline uint32_t bit(int b) {
  return 1u << b;
}
// Events handled later in a frame can start a timer after frameStart.
// Never let an unsigned subtraction turn that small difference into a timeout.
inline uint64_t elapsedUs(uint64_t now, uint64_t start) {
  return now >= start ? now - start : 0;
}
inline bool deadlineReached(uint64_t now, uint64_t start, uint64_t duration) {
  return now >= start && elapsedUs(now, start) >= duration;
}
struct HoldStats {
  uint32_t held = 0, counted = 0;
  uint64_t since[ButtonCount] = {};
  unsigned liveMs[ButtonCount] = {}, maxMs[ButtonCount] = {}, longHolds[ButtonCount] = {};
  void update(uint32_t next, uint64_t now) {
    for (int b = 0; b < ButtonCount; b++) {
      uint32_t m = bit(b);
      if ((next & m) && !(held & m)) {
        since[b] = now;
        counted &= ~m;
      }
      if ((held | next) & m) {
        uint64_t ms = elapsedUs(now, since[b]) / 1000;
        unsigned duration = unsigned(ms > 0xffffffffu ? 0xffffffffu : ms);
        if (duration > maxMs[b])
          maxMs[b] = duration;
        if (duration >= 500 && !(counted & m)) {
          longHolds[b]++;
          counted |= m;
        }
        liveMs[b] = (next & m) ? duration : 0;
      }
      if (!(next & m))
        counted &= ~m;
    }
    held = next;
  }
};
struct Buttons {
  uint32_t held = 0, seen = 0, released = 0, changes = 0;
  uint16_t chords = 0;
  bool shoulders = false;
  unsigned presses[ButtonCount] = {};
  void event(int b, bool down) {
    if (b < 0 || b >= ButtonCount)
      return;
    uint32_t m = bit(b);
    if (down) {
      if (!(held & m)) {
        presses[b]++;
        changes++;
      }
      held |= m;
      seen |= m;
    } else {
      if (held & m) {
        released |= m;
        changes++;
      }
      held &= ~m;
    }
    observe();
  }
  void observe() {
    for (int f = 0; f < 4; f++)
      for (int d = 0; d < 4; d++)
        if ((held & bit(A + f)) && (held & bit(d)))
          chords |= uint16_t(1u << (f * 4 + d));
    if ((held & bit(L1)) && (held & bit(R1)))
      shoulders = true;
  }
  void clearHeld() {
    held = 0;
  }
  int completed() const {
    int n = 0;
    for (int i = 0; i < ButtonCount; i++)
      if ((seen & released) & bit(i))
        n++;
    return n;
  }
};
inline int clamp(int v, int lo, int hi) {
  return v < lo ? lo : v > hi ? hi : v;
}
inline unsigned popcount(uint32_t v) {
  unsigned n = 0;
  while (v) {
    n += v & 1;
    v >>= 1;
  }
  return n;
}
struct ParameterGesture {
  uint32_t previous = 0;
  uint64_t since = 0, next = 0;
  unsigned faces = 0;
  int direction = 0, step = 1;
  bool tick(uint32_t held, uint64_t now) {
    uint32_t chord = held & 255;
    faces = (chord >> 4) & 15;
    direction = 0;
    step = 1;
    if (!(chord & 15) || !faces) {
      previous = 0;
      faces = 0;
      return false;
    }
    if (chord != previous) {
      previous = chord;
      since = now;
      next = now;
    }
    if (now < next)
      return false;
    uint64_t elapsed = now - since;
    next = now + (elapsed < 400000 ? 350000 : elapsed < 1200000 ? 120000 : 50000);
    direction =
        ((chord & (bit(Right) | bit(Up))) ? 1 : 0) - ((chord & (bit(Left) | bit(Down))) ? 1 : 0);
    step = elapsed > 1600000 ? 5 : 1;
    return direction != 0;
  }
};
struct Tone {
  int16_t wave[2048] = {};
  uint32_t phase = 0, delta = 0, position = 0, end = 0;
  int rate = 44100, channel = 3, fade = 441;
  bool done = true;
  void prepare(int sampleRate, int hz, int mode, int seconds = 10) {
    rate = sampleRate;
    channel = mode;
    phase = position = 0;
    delta = uint32_t((uint64_t(hz) << 32) / rate);
    end = uint32_t(rate * seconds);
    fade = rate / 100;
    done = false;
    for (int i = 0; i < 2048; i++)
      wave[i] = int16_t(sin(double(i) * 6.283185307179586 / 2048) * 1640.0);
  }
  void stop() {
    uint32_t tail = position + uint32_t(fade);
    if (tail < end)
      end = tail;
  }
  void render(int16_t *out, unsigned frames) {
    for (unsigned i = 0; i < frames; i++) {
      int value = 0, mode = channel;
      if (position < end) {
        int ramp = clamp(int(position), 0, fade);
        int tail = int(end - position);
        if (tail < ramp)
          ramp = tail;
        value = int(wave[phase >> 21]) * ramp / fade;
        phase += delta;
        if (mode == 3) {
          unsigned section = position / uint32_t(rate * 2);
          mode = section % 3 == 0 ? 1 : section % 3 == 1 ? 2 : 0;
        }
        position++;
      }
      out[i * 2] = (mode == 0 || mode == 1) ? int16_t(value) : 0;
      out[i * 2 + 1] = (mode == 0 || mode == 2) ? int16_t(value) : 0;
    }
    done = position >= end;
  }
};
struct Timing {
  uint32_t count = 0, overBudget = 0, lateIntervals = 0;
  uint64_t totalUs = 0, maxUs = 0, maxGapUs = 0, lastStart = 0;
  void record(uint64_t start, uint64_t finish, uint64_t budget) {
    uint64_t elapsed = finish >= start ? finish - start : 0;
    count++;
    totalUs += elapsed;
    if (elapsed > maxUs)
      maxUs = elapsed;
    if (elapsed > budget)
      overBudget++;
    if (lastStart && start >= lastStart) {
      uint64_t gap = start - lastStart;
      if (gap > maxGapUs)
        maxGapUs = gap;
      if (gap > budget * 3 / 2)
        lateIntervals++;
    }
    lastStart = start;
  }
};
struct Canvas {
  uint32_t pixels[W * H];
  void clear(uint32_t color) {
    for (int i = 0; i < W * H; i++)
      pixels[i] = color;
  }
  void rect(int x, int y, int w, int h, uint32_t color) {
    int x1 = clamp(x, 0, W), y1 = clamp(y, 0, H), x2 = clamp(x + w, 0, W), y2 = clamp(y + h, 0, H);
    for (int r = y1; r < y2; r++)
      for (int c = x1; c < x2; c++)
        pixels[r * W + c] = color;
  }
  void box(int x, int y, int w, int h, uint32_t color) {
    rect(x, y, w, 1, color);
    rect(x, y + h - 1, w, 1, color);
    rect(x, y, 1, h, color);
    rect(x + w - 1, y, 1, h, color);
  }
};
} // namespace lab
#endif
