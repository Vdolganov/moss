#include "../src/lab_core.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <initializer_list>
#include "../src/lab_ui.h"
using namespace lab;
int main() {
  for (int c = 32; c < 127; c++)
    assert(strlen(glyph(char(c))) == 35);
  ParameterGesture g;
  assert(!g.tick(bit(A), 100));
  assert(g.tick(bit(A) | bit(Right), 200));
  assert(g.faces == 1 && g.direction == 1 && g.step == 1);
  assert(!g.tick(bit(A) | bit(Right), 300));
  assert(!g.tick(0, 400));
  assert(g.tick(bit(A) | bit(Right), 500));
  assert(g.step == 1);
  assert(g.tick(bit(A) | bit(Right), 1700500));
  assert(g.step == 5);
  assert(g.tick(bit(X) | bit(Left), 1800000));
  assert(g.faces == 4 && g.direction == -1 && g.step == 1);
  assert(!g.tick(bit(X) | bit(Left) | bit(Right), 1900000));
  Buttons b;
  for (int i = 0; i < ButtonCount; i++) {
    assert(rawButton(0x5a + i) == i);
    b.event(i, true);
    b.event(i, true);
    assert(b.presses[i] == 1);
    b.event(i, false);
  }
  assert(b.completed() == 15);
  assert(b.held == 0);
  assert(rawButton(0x69) == -1);
  for (int f = A; f <= Y; f++)
    for (int d = Up; d <= Right; d++) {
      b.event(d, true);
      b.event(f, true);
      b.event(d, false);
      b.event(f, false);
    }
  assert(b.chords == 65535);
  b.event(L1, true);
  b.event(R1, true);
  assert(b.shoulders);
  b.clearHeld();
  assert(b.held == 0);
  Tone t;
  int16_t buffer[4098];
  for (int rate : {44100, 48000})
    for (int mode = 0; mode < 4; mode++) {
      t.prepare(rate, 440, mode, 1);
      int peak = 0;
      uint64_t energy = 0;
      while (!t.done) {
        buffer[4096] = 123;
        buffer[4097] = 456;
        t.render(buffer, 2048);
        assert(buffer[4096] == 123 && buffer[4097] == 456);
        for (int i = 0; i < 4096; i++) {
          int a = abs(int(buffer[i]));
          if (a > peak)
            peak = a;
          energy += a;
          if (mode == 1 && i % 2)
            assert(buffer[i] == 0);
          if (mode == 2 && !(i % 2))
            assert(buffer[i] == 0);
        }
      }
      assert(t.position == (unsigned)rate);
      assert(peak <= 1640 && peak > 1000);
      assert(energy > 0);
      t.render(buffer, 2048);
      for (int i = 0; i < 4096; i++)
        assert(buffer[i] == 0);
    }
  t.prepare(44100, 880, 0);
  t.render(buffer, 256);
  t.stop();
  while (!t.done)
    t.render(buffer, 256);
  assert(t.position <= 697);
  Timing stats;
  stats.record(1000, 1100, 500);
  stats.record(1600, 2200, 500);
  assert(stats.count == 2 && stats.overBudget == 1 && stats.maxUs == 600);
  puts("PASS: raw map, releases, all 16 chords, L1+R1, bounded PCM, channels, fades, timing");
  return 0;
}
