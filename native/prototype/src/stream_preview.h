#pragma once
#include <stdint.h>
namespace moss {
// Single UI producer, audio consumer. PCM storage is fixed; no files in render.
struct StreamPreview {
  static constexpr unsigned Capacity = 65536;
  int16_t pcm[Capacity][2] = {};
  uint32_t readIndex = 0, writeIndex = 0, state = 0, eof = 0, shownFrame = 0,
           underruns = 0;
  unsigned sourceRate = 0, first = 0, total = 0;
  uint64_t fraction = 0, increment = 0;
  bool starving = false;
  void reset() {
    readIndex = writeIndex = state = eof = shownFrame = 0;
    fraction = 0;
    starving = false;
  }
  void begin(unsigned hz, unsigned outputRate, unsigned start,
             unsigned length) {
    reset();
    sourceRate = hz;
    first = start;
    total = length;
    shownFrame = start;
    increment = (uint64_t(hz) << 32) / outputRate;
  }
  void render(int &left, int &right) {
    if (__atomic_load_n(&state, __ATOMIC_ACQUIRE) != 1)
      return;
    unsigned r = __atomic_load_n(&readIndex, __ATOMIC_RELAXED),
             w = __atomic_load_n(&writeIndex, __ATOMIC_ACQUIRE),
             available = w - r;
    unsigned consume = unsigned((fraction + increment) >> 32);
    bool ended = __atomic_load_n(&eof, __ATOMIC_ACQUIRE) != 0;
    if (!available || (!ended && available < (consume > 1 ? consume : 2))) {
      if (ended) {
        __atomic_store_n(&state, 2, __ATOMIC_RELEASE);
        return;
      }
      if (!starving) {
        underruns++;
        starving = true;
      }
      return;
    }
    starving = false;
    int *output[] = {&left, &right};
    unsigned next = available > 1 ? r + 1 : r;
    for (int ch = 0; ch < 2; ch++) {
      int a = pcm[r % Capacity][ch], b = pcm[next % Capacity][ch];
      int value = a + int((int64_t(b - a) * int64_t(uint32_t(fraction))) >> 32);
      *output[ch] += value;
    }
    fraction += increment;
    consume = unsigned(fraction >> 32);
    fraction &= 0xffffffffu;
    if (consume > available)
      consume = available;
    __atomic_store_n(&readIndex, r + consume, __ATOMIC_RELEASE);
    __atomic_store_n(&shownFrame,
                     first + (r + consume > total ? total : r + consume),
                     __ATOMIC_RELEASE);
    if (ended && consume == available)
      __atomic_store_n(&state, 2, __ATOMIC_RELEASE);
  }
};
} // namespace moss
