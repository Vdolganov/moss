#include "../src/lab_input.h"
#include "../src/lab_ui.h"
#include "hold_trace.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
using namespace lab;
static Canvas canvas;
int main() {
  uint64_t frameStart = 1000000, playbackStart = 1000400;
  assert(frameStart - playbackStart > 12000000); // Reproduces 0.1.1 immediate watchdog.
  assert(!deadlineReached(frameStart, playbackStart, 12000000));
  assert(!deadlineReached(playbackStart, playbackStart, 12000000));
  assert(!deadlineReached(playbackStart + 11999999, playbackStart, 12000000));
  assert(deadlineReached(playbackStart + 12000000, playbackStart, 12000000));

  View view;
  RawInputState input;
  input.profile = InputProfile::GarlicGamepad;
  ParameterGesture gesture;
  unsigned attempts[4] = {};
  size_t index = 0;
  const size_t count = sizeof(holdTrace) / sizeof(holdTrace[0]);
  // Tick every millisecond to include event edges and idle time between them.
  for (unsigned ms = holdTrace[0].ms; ms <= holdTrace[count - 1].ms; ms++) {
    auto adjust = [&]() {
      if (gesture.tick(view.buttons.held, uint64_t(ms) * 1000))
        for (int f = 0; f < 4; f++)
          if (gesture.faces & (1u << f)) {
            attempts[f]++;
            view.values[f] = clamp(view.values[f] + gesture.direction * gesture.step, 0, 100);
          }
    };
    while (index < count && holdTrace[index].ms == ms) {
      const HeldEvent &e = holdTrace[index++];
      if (e.type == 1)
        input.key(e.code, e.value);
      else
        input.axis(e.code, e.value);
      for (int b = 0; b < ButtonCount; b++)
        if (bool(view.buttons.held & bit(b)) != bool(input.held() & bit(b))) {
          view.buttons.event(b, (input.held() & bit(b)) != 0);
          view.holds.update(view.buttons.held, uint64_t(ms) * 1000);
          adjust();
        }
    }
    view.holds.update(view.buttons.held, uint64_t(ms) * 1000);
    adjust();
    if (ms == 165100) {
      assert(view.buttons.held == (bit(Y) | bit(Up)));
      assert(view.holds.liveMs[Y] == 1949);
      render(canvas, view);
      int x = 18 + (Y % 5) * 122, y = 133 + (Y / 5) * 46;
      assert(canvas.pixels[(y + 1) * W + x + 1] == TEXT); // Tile stays down without new events.
      assert(canvas.pixels[346 * W + 18 + 3 * 153] == slots[3]); // Parameter armed highlight.
    }
  }
  assert(view.buttons.held == 0);
  assert(view.holds.maxMs[Y] == 3790);
  assert(view.holds.longHolds[Y] == 2);
  assert(view.holds.maxMs[X] == 1464 && view.holds.maxMs[A] == 1332 && view.holds.maxMs[B] == 1164);
  assert(view.values[0] == 52 && view.values[1] == 52 && view.values[2] == 52 &&
         view.values[3] == 61);
  assert(attempts[0] == 2 && attempts[1] == 2 && attempts[2] == 2 && attempts[3] == 11);

  HoldStats hold;
  ParameterGesture repeat;
  int value = 50;
  unsigned updates = 0;
  uint32_t chord = bit(A) | bit(Right);
  for (uint64_t now = 0; now <= 2000000; now += 10000) {
    hold.update(chord, now);
    if (repeat.tick(chord, now)) {
      value = clamp(value + repeat.step, 0, 100);
      updates++;
    }
  }
  assert(updates > 15 && value > 75);
  assert(hold.liveMs[A] == 2000 && hold.longHolds[A] == 1);
  assert(!repeat.tick(bit(A), 2010000));
  hold.update(bit(A), 2010000);
  assert(hold.liveMs[Right] == 0 && hold.liveMs[A] == 2010);
  hold.update(0, 2500000);
  assert(hold.maxMs[A] == 2500 && hold.liveMs[A] == 0);
  puts("PASS: start-frame audio watchdog, real 3.790s holds, 17 parameter edits, continuous repeat "
       "and hold rendering");
}
