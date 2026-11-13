#include "../src/app.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
using namespace moss;
static moss::SampleBuffer samples[3];
static Engine a, b;
static App app;
static int16_t out[4096], other[4096];
static void advance(Engine &e, unsigned frames) {
  while (frames) {
    unsigned n = frames > 2048 ? 2048 : frames;
    e.render(out, n);
    frames -= n;
  }
}
int main() {
  for (int i = 0; i < 3; i++)
    assert(samples[i].load(lab::sampleFiles[i]));
  Project p = makeProject(true);
  assert(valid(p));
  Codec c;
  assert(c.encode(p));
  Project decoded;
  assert(c.decode(decoded, c.cursor));
  assert(decoded.tracks[0].steps[0].length == 64);
  c.bytes[45] ^= 1;
  assert(!c.decode(decoded, c.cursor));
  assert(saveProject("build/test-project.moss", p));
  assert(loadProject("build/test-project.moss", decoded));
  remove("build/test-project.moss");
  Queue q;
  for (int i = 0; i < 256; i++)
    assert(q.push({Op::Global, 0, 0, 0, i}));
  assert(!q.push({Op::Rewind}));
  Command command;
  for (int i = 0; i < 256; i++) {
    assert(q.pop(command));
    assert(command.value == i);
  }
  assert(!q.pop(command));
  a.init(p, samples, 44100);
  b.init(p, samples, 44100);
  a.handle({Op::Transport});
  b.handle({Op::Transport});
  for (int block = 0; block < 100; block++) {
    a.render(out, 1024);
    for (int n = 0; n < 8; n++)
      b.render(other + n * 256, 128);
    assert(!memcmp(out, other, 4096));
  }
  p = makeProject(false);
  p.bpm = 120;
  p.tracks[0].cycle = 7;
  p.tracks[1].cycle = 15;
  for (int t = 0; t < 3; t++) {
    p.tracks[t].hits[0][0].on = 1;
    p.tracks[t].steps[0].length = 16;
  }
  a.init(p, samples, 8000);
  a.handle({Op::Transport});
  advance(a, 26251);
  assert(a.tick == 105);
  assert(a.triggerCount[0] == 16 && a.triggerCount[1] == 8);
  assert(a.lastTrigger[0] == 26250 && a.lastTrigger[1] == 26250);
  p.tracks[0].cycle = 512;
  p.tracks[1].mute = 1;
  p.tracks[2].mute = 1;
  a.init(p, samples, 8000);
  a.handle({Op::Transport});
  advance(a, 1);
  assert(a.voices[0].active && a.voices[0].mode == 0);
  unsigned duration =
      unsigned(uint64_t(samples[0].frames) * 8000 / samples[0].rate);
  advance(a, duration + 1);
  assert(
      !a.voices[0].active); // one shot ends even though the pattern continues
  p.tracks[0].hits[0][0].locks[0] = 12;
  a.init(p, samples, 8000);
  a.handle({Op::Transport});
  advance(a, 1);
  auto locked = a.voices[0].increment;
  advance(a, 250);
  assert(a.voices[0].increment == locked); // hit settings last for the voice
  assert(a.project.tracks[0].keys[0].pitch == 0);
  assert(a.project.tracks[0].hits[0][0].locks[0] == 12);
  a.handle({Op::Key, 0, 0, 0, -1});
  advance(a, 2000);
  assert(!a.voices[0].active);
  p.tracks[0].hits[0][0].probability = 0;
  a.init(p, samples, 8000);
  a.handle({Op::Transport});
  advance(a, 70000);
  assert(a.triggerCount[0] == 0);
  p = makeProject(true);
  a.init(p, samples, 44100);
  a.handle({Op::Transport});
  advance(a, 10000);
  a.handle({Op::Transport});
  advance(a, 129);
  a.render(out, 1024);
  for (int i = 0; i < 2048; i++)
    assert(out[i] == 0);
  assert(nextKey(p.tracks[0], 0, 1) == 1 && nextKey(p.tracks[0], 1, 1) == 2 &&
         nextKey(p.tracks[0], 2, 1) == 2);
  app.engine = &a;
  app.project = p;
  app.page = Pattern;
  app.track = 1;
  app.step = 4;
  app.button(lab::L1, true, 1000000);
  assert(app.track == 1);
  app.button(lab::R1, true, 1020000);
  assert(app.track == 1 && app.options);
  app.button(lab::L1, false, 1040000);
  app.button(lab::R1, false, 1050000);
  assert(app.track == 1);
  app.options = false;
  int before = app.project.tracks[1].hits[0][4].velocity;
  int on = app.project.tracks[1].hits[0][4].on;
  app.button(lab::Y, true, 2000000);
  app.button(lab::Right, true, 2020000);
  app.button(lab::Right, false, 2040000);
  app.button(lab::Y, false, 2050000);
  assert(app.page == Pattern && app.step == 4 &&
         app.project.tracks[1].hits[0][4].velocity == before + 1 &&
         app.project.tracks[1].hits[0][4].on == on);
  app.bank = 3;
  app.edit(0, -1, 9);
  assert(app.project.tracks[1].cycle == 28);
  app.edit(0, 1, 25);
  assert(app.project.tracks[1].cycle == 128);
  puts("PASS music: codec and checksum, atomic queue limits, block-invariant "
       "audio, "
       "7/15 cycles, one-shot completion, latched hit locks, deleted keys, "
       "probability, "
       "stop fade, shoulder chord, parameter gestures");
}
