#include "../src/app.h"
#undef NDEBUG
#include <assert.h>
using namespace moss;
static Library library;
static Engine engine, reference;
static App app;
static FxState effect;
static int16_t out[2048], other[2048];
static void advance(Engine &e, unsigned frames) {
  while (frames) {
    unsigned n = frames > 1024 ? 1024 : frames;
    e.render(out, n);
    frames -= n;
  }
}
static void fixture(const char *path, int bits, int format = 1) {
  FILE *f = fopen(path, "wb");
  assert(f);
  unsigned frames = 800, channels = 2, bytes = frames * channels * (bits / 8);
  fwrite("RIFF", 1, 4, f);
  put32(f, 36 + bytes);
  fwrite("WAVEfmt ", 1, 8, f);
  put32(f, 16);
  put16(f, format);
  put16(f, channels);
  put32(f, 8000);
  put32(f, 8000 * channels * (bits / 8));
  put16(f, channels * (bits / 8));
  put16(f, bits);
  fwrite("data", 1, 4, f);
  put32(f, bytes);
  for (unsigned i = 0; i < frames; i++)
    for (unsigned ch = 0; ch < channels; ch++) {
      int value = (i % 100 < 50 ? 12000 : -12000) * (ch ? -1 : 1);
      if (format == 3) {
        float v = value / 32768.f;
        uint32_t raw;
        memcpy(&raw, &v, 4);
        put32(f, raw);
      } else if (bits == 16)
        put16(f, unsigned(value) & 65535);
      else if (bits == 24) {
        unsigned v = unsigned(value * 256);
        fputc(v & 255, f);
        fputc((v >> 8) & 255, f);
        fputc((v >> 16) & 255, f);
      } else
        put32(f, uint32_t(int64_t(value) * 65536));
    }
  assert(!fclose(f));
}
static void gestures() {
  app = App{};
  app.engine = &engine;
  app.library = &library;
  app.page = Sampler;
  app.track = 0;
  app.key = 0;
  engine.init(app.project, library.pcm, 8000, Assets);
  app.button(lab::Y, true, 1000000);
  app.button(lab::Right, true, 1010000);
  app.button(lab::Y, false, 1020000);
  app.tick(1900000);
  app.button(lab::Right, false, 1910000);
  assert(app.key == 0 && app.project.tracks[0].keys[0].pitch ==
                             -11); // No navigation after face released first.
  app.page = Space;
  app.fxDetail = true;
  app.project.tracks[0].fx.type = 2;
  app.edit(1, -1, 100);
  assert(app.project.tracks[0].fx.values[2][1] == 2);
  app.editing = false;
  app.page = Pattern;
  app.bank = 2;
  app.step = 3;
  app.edit(2, 1, 5);
  assert(app.project.tracks[0].steps[3].fxLocks[2][2] == 7);
  assert(app.project.tracks[0].steps[3].fxLocks[0][2] == Inherit);
  app.option = app.optionIndex(4);
  app.chooseOption();
  assert(app.project.tracks[0].steps[3].fxLocks[2][2] == Inherit);
  app.page = Mix;
  app.mixTab = 1;
  app.edit(1, 1, 3);
  assert(app.project.fx.values[0][1] == 38);
  app.page = Tape;
  app.tapeTab = 0;
  app.edit(0, 1, 3);
  app.edit(1, 1, 7);
  assert(app.captureSource == 3 && app.captureBars == 8);
  assert(app.optionCount() == 0); // SOURCE uses direct capture/import controls
  app.parameterPage(-1);
  assert(app.tapeTab == 2 && !app.naming);
  app.page = Tape;
  app.tapeTab = 1;
  app.sliceCount = 1;
  app.source.rate = 8000;
  app.source.frames = 480000;
  app.slices[0] = {0, 60000, 0, 0};
  app.clearInput();
  app.button(lab::B, true, 3000000);
  app.button(lab::Left, true, 3010000);
  assert(app.slices[0].end == 59999);
  app.tick(5010000);
  assert(app.slices[0].end == 59994);
  app.tick(7010000);
  assert(app.slices[0].end == 59989);
  app.clearInput();
  app.page = Sampler;
  app.key = 1;
  app.pickAsset();
  assert(app.assignMode && app.page == Tape);
  app.asset = 0;
  app.action(lab::A);
  assert(app.page == Sampler && app.project.tracks[0].keys[1].asset == 0);
  app.project.tracks[0].keys[2].asset = 31;
  assert(!app.available(31));
  assert(app.assignedKey(1, 1) == 1); // no later assigned/available slot
}
static void polymeter() {
  Project p = makeProject(false);
  p.bpm = 120;
  for (int t = 0; t < Tracks; t++) {
    p.tracks[t].cycle = t == 2 ? 15 : 16;
    p.tracks[t].hits[0][0].on = 1;
  }
  engine.init(p, library.pcm, 8000, Assets);
  engine.handle({Op::Transport});
  for (unsigned tick = 0; tick <= 240; tick++) {
    engine.render(out, 1);
    assert(engine.shownStep[0] == tick % 16 &&
           engine.shownStep[1] == tick % 16 &&
           engine.shownStep[2] == tick % 15);
    assert(engine.triggerCount[0] == tick / 16 + 1 &&
           engine.triggerCount[2] == tick / 15 + 1);
    advance(engine, 249);
  }
  assert(engine.shownStep[0] == 0 && engine.shownStep[2] == 0);
  engine.handle({Op::Track, 2, 0, 0, 7});
  assert(engine.shownStep[2] == engine.tick % 7);
}
static void fxTests() {
  for (int type = 0; type < 4; type++) {
    effect.reset();
    Effect e;
    e.enabled = 1;
    e.type = type;
    e.values[type][type == 0 ? 2 : 3] = 100;
    long long early = 0, late = 0;
    for (int i = 0; i < 44100 * 3; i++) {
      int l = i < 1000 ? 12000 : 0, r = i < 1000 ? -5000 : 0;
      effect.process(l, r, e, nullptr, 44100, 120);
      assert(l >= -32768 && l <= 32767 && r >= -32768 && r <= 32767);
      if (i < 22050)
        early += abs(l) + abs(r);
      if (i > 44100 * 2)
        late += abs(l) + abs(r);
    }
    assert(early > 0);
    assert(type == 2 ? late == 0 : late < early);
    // A type switch/re-enable must not resurrect an older effect tail.
    e.enabled = 0;
    int l = 0, r = 0;
    for (int i = 0; i < 2048; i++)
      effect.process(l, r, e, nullptr, 44100, 120);
    e.enabled = 1;
    for (int i = 0; i < 44100; i++) {
      l = r = 0;
      effect.process(l, r, e, nullptr, 44100, 120);
      assert(l == 0 && r == 0);
    }
    // Effect + lock output does not depend on callback buffer size.
    Project p = makeProject(true);
    p.bpm = 120;
    p.tracks[0].fx = e;
    p.fx = e;
    p.tracks[0].steps[0].fxLocks[type][0] = fxHigh(type, 0);
    p.tracks[0].steps[1].on = 0;
    engine.init(p, library.pcm, 8000, Assets);
    reference.init(p, library.pcm, 8000, Assets);
    engine.handle({Op::Transport});
    reference.handle({Op::Transport});
    for (int block = 0; block < 5; block++) {
      engine.render(out, 1024);
      for (int i = 0; i < 8; i++)
        reference.render(other + i * 256, 128);
      assert(!memcmp(out, other, sizeof(out)));
    }
    assert(engine.project.tracks[0].fx.values[type][0] ==
           p.tracks[0].fx.values[type][0]);
    Codec codec;
    assert(codec.encode(p));
    Project decoded;
    assert(codec.decode(decoded, codec.cursor));
    assert(decoded.tracks[0].steps[0].fxLocks[type][0] == fxHigh(type, 0));
    assert(decoded.fx.enabled == 1 && decoded.fx.type == type);
  }
}
static void captureTests() {
  Project p = makeProject(true);
  p.bpm = 120;
  p.tracks[0].fx.type = 2;
  p.tracks[0].fx.enabled = 1;
  p.fx.type = 0;
  p.fx.enabled = 1;
  engine.init(p, library.pcm, 8000, Assets);
  engine.handle({Op::Transport});
  engine.handle({Op::CaptureStart, 0, 0, 0, 1});
  unsigned offset = 0;
  while (offset < 16000) {
    unsigned n = 16000 - offset;
    if (n > 1024)
      n = 1024;
    engine.render(out, n);
    for (unsigned i = 0; i < n * 2; i++)
      assert(out[i] == Engine::monitor(engine.capture.pcm[offset * 2 + i]));
    offset += n;
  }
  assert(engine.capture.state == 2 && engine.capture.frames == 16000 &&
         engine.capture.shownFrames == 16000);
  engine.handle({Op::CaptureStart, 1, 0, 0, 1});
  assert(engine.capture.source == -1); // Finished data cannot be overwritten.
  app = App{};
  app.engine = &engine;
  app.library = &library;
  app.page = Tape;
  app.flushCapture();
  assert(engine.capture.state == 0 && exists("captures/take-0001.wav") &&
         app.source.frames == 16000);
  engine.handle({Op::CaptureStart, 1, 0, 0, 1});
  advance(engine, 1000);
  engine.handle({Op::CaptureStop});
  assert(engine.capture.state == 2 && engine.capture.frames == 1000);
  bool audible = false;
  for (unsigned i = 0; i < 2000; i++)
    audible |= engine.capture.pcm[i] != 0;
  assert(audible);
  engine.handle({Op::CaptureDiscard});
  assert(engine.capture.state == 0);
  engine.handle({Op::CaptureStart, 0, 0, 0, 1});
  advance(engine, 100);
  engine.handle({Op::CaptureStop});
  app.page = Tape;
  app.tapeTab = 0;
  app.option = app.optionIndex(5);
  app.chooseOption();
  assert(app.discardPending);
  app.flushCapture();
  assert(!exists("captures/take-0002.wav"));
  engine.render(out, 1);
  app.flushCapture();
  assert(!app.captureBusy());
  p.bpm = 70;
  engine.init(p, library.pcm, 44100, Assets);
  engine.handle({Op::Transport});
  engine.handle({Op::CaptureStart, 0, 0, 0, 1});
  advance(engine, 151200);
  app.flushCapture();
  assert(
      app.source.frames == 151200 &&
      app.sliceFrames(app.slices[0]) ==
          151200); // Millisecond display must not shorten a full recorded bar.
  engine.handle({Op::CaptureStart, 0, 0, 0, 1});
  advance(engine, 100);
  engine.handle({Op::CaptureStop});
  assert(rename("captures", "captures-off") == 0);
  app.flushCapture();
  assert(app.captureFailed && engine.capture.state == 2 &&
         engine.capture.frames == 100);
  assert(rename("captures-off", "captures") == 0);
  app.captureFailed = false;
  app.flushCapture();
  assert(!app.captureBusy() && exists("captures/take-0003.wav"));
}
static void stress() {
  Project p = makeProject(true);
  p.bpm = 30;
  for (auto &t : p.tracks) {
    t.fx.type = 1;
    t.fx.enabled = 1;
    t.fx.values[1][0] = 100;
    t.fx.values[1][1] = 95;
  }
  p.fx.type = 1;
  p.fx.enabled = 1;
  p.fx.values[1][0] = 100;
  p.fx.values[1][1] = 95;
  engine.init(p, library.pcm, 44100, Assets);
  engine.handle({Op::Transport});
  engine.handle({Op::CaptureStart, 0, 0, 0, 8});
  unsigned left = 44100 * 60, peak = 0;
  uint64_t energy = 0;
  while (left) {
    unsigned n = left > 1024 ? 1024 : left;
    engine.render(out, n);
    left -= n;
    for (unsigned i = 0; i < n * 2; i++) {
      unsigned v = unsigned(abs(out[i]));
      energy += v;
      if (v > peak)
        peak = v;
    }
  }
  assert(engine.capture.state == 1 && engine.capture.frames == 44100 * 60 &&
         energy > 0 && peak < 32767);
  engine.handle({Op::Stop});
  assert(engine.capture.state == 2);
  engine.handle({Op::CaptureDiscard});
  puts("PASS stress: 60 seconds at 44.1 kHz, three POND track effects + master "
       "POND + master capture; audible, no full-scale clipping");
}
int main() {
  char base[MAX_PATH];
  GetCurrentDirectoryA(sizeof(base), base);
  char dir[MAX_PATH];
  snprintf(dir, sizeof(dir), "build/features-%lu", GetCurrentProcessId());
  makeDir(dir);
  assert(SetCurrentDirectoryA(dir));
  makeDir("imports");
  makeDir("projects");
  fixture("imports/pcm16.wav", 16);
  fixture("imports/pcm24.wav", 24);
  fixture("imports/pcm32.wav", 32);
  fixture("imports/float32.wav", 32, 3);
  library.init();
  WavSource source;
  for (const char *path : {"imports/pcm16.wav", "imports/pcm24.wav",
                           "imports/pcm32.wav", "imports/float32.wav"}) {
    assert(source.open(path) && source.frames == 800 && source.rate == 8000);
    source.overview();
    assert(source.peaks[0] > 0);
    assert(source.extract(library.scratch, 100, 700, 0, 0));
    assert(library.scratch.frames == 600 && library.scratch.pcm[0] == 12000 &&
           library.scratch.pcm[1] == -12000);
  }
  assert(source.extract(library.scratch, 100, 700, 5, 5));
  assert(library.scratch.pcm[0] == 0 && library.scratch.pcm[1199] == 0);
  assert(!source.extract(library.scratch, 700, 100, 0, 0));
  FILE *f = fopen("imports/broken.wav", "wb");
  fputs("RIFFbroken", f);
  fclose(f);
  assert(!source.open("imports/broken.wav"));
  assert(source.open("imports/pcm24.wav"));
  int asset = library.saveSlice(source, 0, 800, 0, 0);
  assert(asset == 0 && library.pcm[0].frames == 800);
  strcpy(library.names[0], "MANUAL CUT");
  assert(library.saveName(0));
  library.init();
  assert(!strcmp(library.names[0], "MANUAL CUT"));
  for (int i = 1; i < 3; i++)
    assert(library.saveSlice(source, 0, 800, 0, 0) == i);
  // Exact one-MiB asset survives a reload; an oversized selection is refused.
  for (unsigned i = 0; i < moss::SampleBudget / 2; i++)
    library.scratch.pcm[i] = i % 2 ? 1000 : -1000;
  assert(writeWav("imports/one-mib.wav", library.scratch.pcm,
                  moss::SampleBudget / 4, 44100, 2));
  assert(library.scratch.load("imports/one-mib.wav"));
  assert(library.scratch.bytes == moss::SampleBudget);
  assert(source.open("imports/one-mib.wav"));
  source.frames++;
  assert(!source.extract(library.scratch, 0, source.frames, 0, 0));
  gestures();
  polymeter();
  fxTests();
  captureTests();
  app.openSource("imports/pcm24.wav");
  app.service();
  assert(app.sliceCount == 1 && app.page == Tape && app.tapeTab == 1);
  app.slices[0].end = 50;
  app.option = app.optionIndex(6);
  app.chooseOption();
  assert(app.sliceCount == 2 && app.slices[0].end == 50 &&
         app.slices[1].start == 50);
  app.slices[1].start = 60;
  app.slices[1].end = 90;
  app.job = App::SaveAll;
  app.service();
  assert(library.count() == 5 && library.pcm[4].frames == 240);
  app.page = Tape;
  app.tapeTab = 2;
  app.asset = 4;
  assert(!app.assetUsed(4));
  app.job = App::DeleteAsset;
  app.service();
  assert(!library.pcm[4].frames);
  library.init();
  assert(!library.pcm[4].frames);
  app.asset = 0;
  assert(app.assetUsed(0));
  app.job = App::DeleteAsset;
  app.service();
  assert(library.pcm[0].frames);
  app.page = Tape;
  app.tapeTab = 1;
  app.sliceIndex = 0;
  app.job = App::PreviewSlice;
  app.service();
  assert(engine.stream.state == 1 &&
         engine.stream.total == app.sliceFrames(app.slices[app.sliceIndex]));
  engine.render(out, 100);
  bool audible = false;
  for (int n : out)
    audible |= n != 0;
  assert(audible);
  stress();
  assert(SetCurrentDirectoryA(base));
  puts("PASS features: PCM16/24/32/float, bounded slices, fades, 1 MiB reload, "
       "library persistence, key assignment, protected delete, capture "
       "master/track + FX, 15/16 playheads through 240 steps, effect "
       "tails/bounds/block invariance, FX locks/codec, parameter release "
       "order, manual cuts and preview");
}
