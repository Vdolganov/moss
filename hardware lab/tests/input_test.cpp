#include "../src/lab_input.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include "device_trace.h"
using namespace lab;
static void reconcile(Buttons &buttons, const RawInputState &state) {
  for (int b = 0; b < ButtonCount; b++)
    buttons.event(b, (state.held() & bit(b)) != 0);
}
int main() {
  RawInputState input;
  input.profile = InputProfile::GarlicGamepad;
  const int codes[] = {304, 305, 307, 308, 310, 311, 314, 315, 316};
  const int labels[] = {A, B, X, Y, L1, R1, Select, Start, Menu};
  for (int i = 0; i < 9; i++) {
    assert(input.key(codes[i], 1));
    assert(input.held() == bit(labels[i]));
    assert(input.key(codes[i], 2));
    assert(input.held() == bit(labels[i]));
    assert(input.key(codes[i], 0));
    assert(input.held() == 0);
  }
  assert(!input.key(317, 1));
  assert(!input.key(116, 1));
  assert(input.held() == 0);
  assert(!input.key(304, 3));
  assert(!input.key(0x5a, 1));
  assert(input.axis(16, -1));
  assert(input.held() == bit(Left));
  assert(input.axis(17, -1));
  assert(input.held() == (bit(Left) | bit(Up)));
  assert(input.axis(16, 1));
  assert(input.held() == (bit(Right) | bit(Up)));
  assert(input.axis(17, 0));
  assert(input.held() == bit(Right));
  assert(input.axis(16, 0));
  assert(input.held() == 0);
  assert(!input.axis(0, 32767));
  assert(!input.axis(2, 255));
  const int ranges[][2] = {{0, 255}, {-32768, 32767}, {0, 1}};
  for (const auto &range : ranges) {
    input.clear();
    input.triggers[0].configure(range[0], range[1]);
    input.triggers[1].configure(range[0], range[1]);
    Buttons buttons;
    assert(input.axis(2, range[0]));
    assert(input.axis(5, range[0]));
    assert(input.held() == 0);
    assert(input.axis(2, range[1]));
    reconcile(buttons, input);
    assert(input.held() == bit(L2));
    assert(input.axis(2, range[1]));
    reconcile(buttons, input);
    assert(buttons.presses[L2] == 1);
    assert(input.axis(5, range[1]));
    assert(input.held() == (bit(L2) | bit(R2)));
    assert(input.axis(2, range[0]));
    assert(input.held() == bit(R2));
    assert(input.axis(5, range[0]));
    assert(input.held() == 0);
  }
  input.triggers[0].configure(0, 255);
  input.axis(2, 200);
  assert(input.held() == bit(L2));
  input.axis(2, 128);
  assert(input.held() == bit(L2));
  input.axis(2, 80);
  assert(input.held() == 0);
  input.axis(2, 128);
  assert(input.held() == 0);
  input.axis(2, 200);
  assert(input.held() == bit(L2));
  input.key(304, 1);
  input.axis(17, -1);
  input.clear();
  assert(input.held() == 0);
  input.key(305, 1);
  input.axis(16, 1);
  input.axis(5, 255);
  assert(input.held() == (bit(B) | bit(Right) | bit(R2)));
  input.clear();
  Buttons chords;
  const int axes[] = {17, 17, 16, 16}, values[] = {-1, 1, -1, 1};
  for (int face = 0; face < 4; face++)
    for (int direction = 0; direction < 4; direction++) {
      input.key(codes[face], 1);
      input.axis(axes[direction], values[direction]);
      reconcile(chords, input);
      input.key(codes[face], 0);
      input.axis(axes[direction], 0);
      reconcile(chords, input);
    }
  assert(chords.chords == 65535 && chords.held == 0);
  RawInputState legacy;
  legacy.profile = InputProfile::LegacyKeys;
  assert(legacy.key(0x5a, 1) && legacy.held() == bit(Up));
  assert(!legacy.key(304, 1));
  RawInputState unknown;
  assert(!unknown.key(304, 1));
  assert(!unknown.axis(16, -1));
  Buttons replay;
  unsigned recognized = 0;
  for (const auto &event : deviceTrace) {
    if (input.key(event.code, event.value))
      recognized++;
    reconcile(replay, input);
  }
  assert(recognized == deviceTraceRecognized);
  assert(replay.held == 0 && replay.presses[A] > 0 && replay.presses[B] > 0);
  assert(replay.presses[L1] > 0 && replay.presses[R1] > 0 && replay.presses[Menu] > 0);
  printf(
      "PASS: Garlic gamepad, axes, trigger calibration, all 16 chords, %u recorded events mapped\n",
      recognized);
}
