#pragma once
#include "model.h"
namespace moss {
// Timing is measured in hundredths of a canonical 1/64 tick. No rounding to
// audio blocks, no change to cycle duration. The continuous pair warp also
// orders events hidden between cells after changing the displayed resolution.
inline int stepOffset(const Track &track, int at) {
  const int stride = track.stride();
  const int within = at % stride;
  const int swing = (track.swing - SwingStraight) * 2 *
                    ((at / stride) % 2 ? stride - within : within);
  const int shift = track.steps[at].shift == Inherit ? 0 : track.steps[at].shift;
  return swing + shift * stride;
}
struct ScheduledStep {
  uint64_t due = 0, nominal = 0;
  int at = 0;
};
struct StepSchedule {
  // Maximum advance is four ticks, maximum delay eight. One entry per tick;
  // the fixed queue covers both plus the current tick without allocation.
  static constexpr int Lookahead = 4, Capacity = 16;
  ScheduledStep items[Capacity];
  int count = 0;
  bool overflow = false;
  static bool before(const ScheduledStep &a, const ScheduledStep &b) {
    return a.due < b.due || (a.due == b.due && a.nominal < b.nominal);
  }
  void insert(ScheduledStep item) {
    if(count == Capacity) { overflow = true; return; }
    int i=count++;
    while(i && before(item,items[i-1])) { items[i]=items[i-1]; --i; }
    items[i]=item;
  }
  ScheduledStep pop() {
    ScheduledStep result=items[0];
    for(int i=1;i<count;++i) items[i-1]=items[i];
    --count;
    return result;
  }
};
} // namespace moss
