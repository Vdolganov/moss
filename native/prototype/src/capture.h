#pragma once
#include <stdint.h>
namespace moss {
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
