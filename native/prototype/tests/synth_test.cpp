#include "../src/app.h"
#undef NDEBUG
#include <assert.h>
#include <initializer_list>
#include <time.h>
using namespace moss;
static Engine a, b;
static App app;
static int16_t out[2048], other[2048];
static SynthTables tables;
static void advance(Engine &e, unsigned frames) {
  while (frames) {
    unsigned n = frames > 1024 ? 1024 : frames;
    e.render(out, n);
    frames -= n;
  }
}
static void gesture(int face, int dir, uint64_t time) {
  app.button(face, true, time);
  app.button(dir, true, time + 10000);
  app.button(face, false, time + 20000);
  app.tick(time + 700000);
  app.button(dir, false, time + 710000);
}
static void codecAndInput() {
  Project p = makeSynthDemo();
  p.tracks[0].steps[63].note = 96;
  p.tracks[0].steps[63].synthLocks[2][3] = 10000;
  p.tracks[1].synthParams[0][0] = 12000;
  p.tracks[2].synthMode[2] = 1;
  Codec c;
  assert(valid(p) && c.encode(p));
  Project loaded;
  assert(c.decode(loaded, c.cursor));
  // Compare the wire representation; padding bytes in C++ structs are not data.
  static Codec roundtrip;
  assert(roundtrip.encode(loaded) && roundtrip.cursor == c.cursor);
  assert(!memcmp(roundtrip.bytes, c.bytes, c.cursor));
  assert(c.cursor < sizeof(c.bytes));
  printf("PASS synth project: MOS6 %u bytes, all banks round trip\n", c.cursor);
  p.tracks[1].steps[3].note = 97;
  assert(!valid(p));
  p.tracks[1].steps[3].note = 60;
  p.tracks[2].steps[3].synthLocks[2][1] = 8;
  assert(!valid(p));
  app.project = makeSynthDemo();
  app.engine = &a;
  a.init(app.project, nullptr, 44100, 0);
  app.track = 1;
  app.page = Pattern;
  app.step = 0;
  int before = app.project.tracks[1].steps[0].note;
  gesture(lab::Y, lab::Right, 1000000);
  assert(app.project.tracks[1].steps[0].note == before + 1 && app.step == 0 &&
         !app.project.tracks[1].steps[0].on);
  app.bank = 1;
  gesture(lab::Y, lab::Right, 2000000);
  assert(app.project.tracks[1].steps[0].synthLocks[0][0] == 1201);
  assert(app.project.tracks[1].steps[0].locks[0] == Inherit);
  app.page = Sampler;
  int note = app.project.tracks[1].audition;
  gesture(lab::B, lab::Right, 3000000);
  assert(app.project.tracks[1].synthParams[0][1] == 31 &&
         app.project.tracks[1].audition == note && app.page == Sampler);
  app.option = app.optionIndex(3);
  app.chooseOption();
  assert(app.voicePicker == 1);
  Project unchanged = app.project;
  app.voiceChoice = 3;
  app.action(lab::B);
  assert(!memcmp(&unchanged, &app.project, sizeof(Project)));
  app.option = app.optionIndex(3);
  app.chooseOption();
  app.voiceChoice = 3;
  app.button(lab::Select, true, 3800000);
  app.button(lab::Select, false, 3810000);
  assert(app.voicePicker == 0 && app.page == Sampler &&
         !memcmp(&unchanged, &app.project, sizeof(Project)));
  app.option = app.optionIndex(3);
  app.chooseOption();
  app.voiceChoice = 3;
  app.action(lab::A);
  assert(app.project.tracks[1].instrument == FmInstrument &&
         app.project.tracks[1].keys[1].asset == 1);
  app.option = app.optionIndex(4);
  app.chooseOption();
  assert(app.voicePicker == 2);
  Project beforePreset = app.project;
  app.voiceChoice = 2;
  app.button(lab::Select, true, 3900000);
  app.button(lab::Select, false, 3910000);
  assert(app.voicePicker == 0 && app.page == Sampler &&
         !memcmp(&beforePreset, &app.project, sizeof(Project)));
  app.option = app.optionIndex(4);
  app.chooseOption();
  app.voiceChoice = 1;
  app.action(lab::A);
  assert(app.project.tracks[1].synthParams[2][0] == 70);
  app.page = Pattern;
  app.bank = 1;
  gesture(lab::B, lab::Right, 4000000);
  assert(app.project.tracks[1].steps[0].synthLocks[2][1] == 6);
  assert(app.project.tracks[1].steps[0].synthLocks[0][0] == 1201);
  app.option = app.optionIndex(4);
  app.chooseOption();
  assert(app.project.tracks[1].steps[0].synthLocks[2][1] == Inherit);
  assert(app.project.tracks[1].steps[0].synthLocks[0][0] == 1201);
  puts("PASS synth input: note edits, release order, picker cancel, presets, "
       "separate locks and preserved sampler maps");
}
static void musical() {
  for (int kind = 1; kind <= 3; kind++) {
    Project p = makeProject(false);
    p.bpm = 120;
    auto &t = p.tracks[0];
    t.instrument = kind;
    t.cycle = 256;
    t.steps[0].on = 1;
    t.steps[0].length = 64;
    t.steps[0].synthLocks[kind - 1][0] = synthMax(kind, 0);
    a.init(p, nullptr, 8000, 0);
    a.handle({Op::Transport});
    advance(a, 1);
    assert(a.synth[0].target[0] == synthMax(kind, 0));
    advance(a, 1000);
    assert(a.synth[0].target[0] ==
           synthMax(kind, 0)); // locks remain through rests
    advance(a, 14999);
    assert(a.synth[0].gate);
    advance(a, 1);
    assert(!a.synth[0].gate);
    advance(a, 8000 * 5);
    assert(!a.synth[0].active); // before the 64-step cycle restarts
    a.handle({Op::Stop});
    advance(a, 256);
    a.handle({Op::Audition, 0, 0});
    advance(a, 1);
    assert(a.synth[0].gate);
    advance(a, 16000);
    assert(!a.synth[0].gate);
    a.handle({Op::Instrument, 0, 0, 0, 0});
    advance(a, 129);
    assert(!a.synth[0].active && !a.synthTails[0].active);
    p.tracks[0].steps[0].probability = 0;
    a.init(p, nullptr, 8000, 0);
    a.handle({Op::Transport});
    advance(a, 80000);
    assert(!a.triggerCount[0]);
  }
  Project p = makeSynthDemo();
  a.init(p, nullptr, 44100, 0);
  b.init(p, nullptr, 44100, 0);
  a.handle({Op::Transport});
  b.handle({Op::Transport});
  for (int block = 0; block < 500; block++) {
    a.render(out, 1024);
    for (int n = 0; n < 8; n++)
      b.render(other + n * 256, 128);
    assert(!memcmp(out, other, sizeof(out)));
  }
  assert(a.seenInstrumentMask == 14);
  puts("PASS synth audio: sample-free playback, 16-step gates, latched note "
       "locks, "
       "audition timeout, instrument switch, probability, block invariance");
}
static void boundsAndPitch() {
  tables.init();
  for (unsigned rate : {8000u, 44100u, 48000u, 96000u})
    for (int kind = 1; kind <= 3; kind++)
      for (int mode = 0; mode < 2; mode++)
        for (int high = 0; high < 2; high++) {
          int p[8];
          for (int i = 0; i < 8; i++)
            p[i] = high ? synthMax(kind, i) : synthMin(kind, i);
          SynthVoice v;
          v.on(kind, high ? 96 : 24, tables.frequency[high ? 96 : 24], 127, 64,
               p, mode, rate, 17);
          for (unsigned n = 0; n < rate / 2; n++) {
            int l = 0, r = 0;
            v.render(l, r, tables);
            assert(isfinite(v.env) && isfinite(v.dcOut) &&
                   isfinite(v.monoVoice.state[3]));
            assert(l == r && abs(l) < 32113);
          }
          v.stop();
          for (int n = 0; n < 129; n++) {
            int l = 0, r = 0;
            v.render(l, r, tables);
          }
          assert(!v.active);
        }
  for (int kind : {1, 3}) {
    int p[8];
    for (int i = 0; i < 8; i++)
      p[i] = presetValues(kind, 0)[i];
    for (int i = 4; i < 8; i++)
      p[i] = 0; // isolate fundamental for pitch measurement
    if (kind == 1) {
      p[0] = 10000;
      p[1] = 0;
      p[2] = 0;
    } else {
      p[0] = 0;
      p[2] = 0;
      p[3] = 10000;
    }
    SynthVoice v;
    v.on(kind, 69, 440, 127, 64, p, 0, 44100, 1);
    int crossings = 0, previous = 0;
    double sum = 0, energy = 0;
    for (int n = 0; n < 88200; n++) {
      int l = 0, r = 0;
      v.render(l, r, tables);
      if (n >= 44100) {
        if (l >= 0 && previous < 0)
          crossings++;
        sum += l;
        energy += double(l) * l;
      }
      previous = l;
    }
    assert(crossings >= 438 && crossings <= 442);
    assert(sqrt(energy / 44100) > 500);
    assert(fabs(sum / 44100) < 30);
  }
  puts("PASS synth limits: both modes, parameter extrema at 8/44.1/48/96 kHz, "
       "no NaN, bounded output, A4 pitch and DC");
}
static void stress() {
  Project p = makeSynthDemo();
  p.fx.enabled = 1;
  p.fx.type = 1;
  for (int t = 0; t < 3; t++) {
    p.tracks[t].fx.enabled = 1;
    p.tracks[t].fx.type = t;
    p.tracks[t].steps[0].on = 1;
    p.tracks[t].steps[0].length = 64;
  }
  a.init(p, nullptr, 44100, 0);
  a.handle({Op::Transport});
  a.handle({Op::CaptureStart, 0, 0, 0, 8});
  uint64_t energy = 0;
  int peak = 0;
  clock_t start = clock();
  for (unsigned left = 44100 * 60; left;) {
    unsigned n = left > 1024 ? 1024 : left;
    a.render(out, n);
    left -= n;
    for (unsigned i = 0; i < n * 2; i++) {
      peak = peak > abs(out[i]) ? peak : abs(out[i]);
      energy += uint64_t(int64_t(out[i]) * out[i]);
    }
  }
  assert(energy > 10000000 && peak < 32767);
  assert(a.capture.frames > 0 && a.capture.state == 2);
  assert(a.seenInstrumentMask == 14 && a.seenFxMask == 7);
  printf("PASS 60-second synth + three FX + master POND + resample: peak %d; "
         "desktop CPU %.3fs (not ARM timing)\n",
         peak, double(clock() - start) / CLOCKS_PER_SEC);
}
int main() {
  codecAndInput();
  musical();
  boundsAndPitch();
  stress();
}
