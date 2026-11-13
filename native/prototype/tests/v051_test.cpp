#include "../src/app.h"
#undef NDEBUG
#include <assert.h>
using namespace moss;
static App app;
static Engine engine;
static SampleBuffer sample;
static int16_t audio[2048];
static void tap(int button, uint64_t time) {
  app.button(button, true, time);
  app.button(button, false, time + 10000);
}
static void gesture(int button, uint64_t time) {
  app.button(button, true, time);
  app.button(lab::Right, true, time + 10000);
  app.button(button, false, time + 20000);
  app.tick(time + 600000);
  app.button(lab::Right, false, time + 610000);
}
static void advance(unsigned frames) {
  while (frames) {
    unsigned n = frames > 1024 ? 1024 : frames;
    engine.render(audio, n);
    frames -= n;
  }
}
static bool hasOption(const char *text) {
  for (int i = 0; i < app.optionCount(); ++i)
    if (!strcmp(app.optionName(i), text))
      return true;
  return false;
}
int main() {
  static Project legacy;
  assert(loadProject("tests/fixtures/v5-sampler.moss", legacy));
  assert(legacy.trackCount == 4 && !strcmp(legacy.name, "MOS5 FIXTURE"));
  for (const auto &tr : legacy.tracks) {
    assert(tr.fx.count() == 2 && tr.fx.active(3) && tr.fx.active(1));
    for (int k = 0; k < Keys; ++k) {
      assert(tr.keys[k].asset == k && tr.keys[k].pitch == k * 3);
      assert(tr.hits[k][k * 4].on && tr.hits[k][k * 4].velocity == 70 + k);
      assert(tr.hits[k][k * 4].length == 0);
    }
  }
  sample.rate = 8000;
  sample.channels = 1;
  sample.frames = 16000;
  for (unsigned i = 0; i < sample.frames; ++i)
    sample.pcm[i] = int16_t(i % 64 < 32 ? 12000 : -12000);
  Project p;
  p.trackCount = 1;
  p.bpm = 120;
  auto &tr = p.tracks[0];
  tr.keys[0].asset = 0;
  tr.cycle = 512;
  tr.hits[0][0].on = 1;
  tr.hits[0][0].length = 4;
  engine.init(p, &sample, 8000, 1);
  engine.handle({Op::Transport});
  advance(1000);
  assert(engine.voice(0, 0).active);
  advance(129);
  assert(!engine.voice(0, 0).active && engine.triggerCount[0] == 1);
  tr.hits[0][0].length = 0;
  engine.init(p, &sample, 8000, 1);
  engine.handle({Op::Transport});
  advance(1200);
  assert(engine.voice(0, 0).active);
  sample.frames = 80;
  tr.hits[0][0].length = 512;
  engine.init(p, &sample, 8000, 1);
  engine.handle({Op::Transport});
  advance(81);
  assert(!engine.voice(0, 0).active && engine.triggerCount[0] == 1);
  app.engine = &engine;
  app.project = p;
  app.page = Pattern;
  app.bank = 0;
  tr.hits[0][0].length = 0;
  app.project = p;
  tap(lab::R1, 1000000);
  assert(app.bank == 1 && app.track == 0);
  tap(lab::L1, 1100000);
  assert(app.bank == 0);
  tap(lab::Y, 1200000);
  assert(app.bank == 0);
  gesture(lab::Y, 2000000);
  assert(app.project.tracks[0].hits[0][0].velocity == 91);
  gesture(lab::B, 3000000);
  assert(app.project.tracks[0].hits[0][0].fadeIn == 1);
  gesture(lab::A, 4000000);
  assert(app.project.tracks[0].hits[0][0].length == 4);
  app.project.tracks[0].hits[0][0].probability = 50;
  gesture(lab::X, 5000000);
  assert(app.project.tracks[0].hits[0][0].probability == 51);
  assert(app.step == 0 && app.bank == 0 && app.project.tracks[0].hits[0][0].on);
  gesture(lab::L1, 6000000);
  assert(app.key == 1 && app.bank == 0);
  tap(lab::L1, 7000000);
  assert(app.bank == 3);
  app.page = Sampler;
  app.project.tracks[0].instrument = MonoInstrument;
  tap(lab::R1, 8000000);
  assert(app.synthBank == 1);
  tap(lab::L1, 9000000);
  assert(app.synthBank == 0);
  app.page = Space;
  for (int type = 0; type < 4; ++type) {
    app.nextEffect(1);
    app.action(lab::X);
  }
  assert(app.project.tracks[0].fx.count() == 2);
  assert(!hasOption("ENABLE / BYPASS SELECTED"));
  app.page = Sampler;
  assert(!hasOption("EXIT MOSS") && hasOption("CHOOSE PRESET"));
  app.project.tracks[0].instrument = SampleInstrument;
  assert(!hasOption("ONE SHOT PLAYBACK") && !hasOption("REMOVE ASSIGNMENT"));
  app.page = Pattern;
  app.bank = 0;
  assert(!hasOption("CLEAR CURRENT LOCK BANK"));
  app.bank = 1;
  assert(!hasOption("CLEAR CURRENT LOCK BANK"));
  app.project.tracks[0].hits[app.key][app.step].locks[0] = 1;
  assert(hasOption("CLEAR CURRENT LOCK BANK"));
  app.beginName(2, "OLD NAME");
  app.nameKey = 2;
  tap(lab::A, 10000000);
  assert(!strcmp(app.nameEdit, "C") && app.naming);
  app.navigate(lab::Right);
  tap(lab::A, 11000000);
  assert(!strcmp(app.nameEdit, "CD"));
  tap(lab::L1, 12000000);
  app.nameKey = 0;
  tap(lab::A, 13000000);
  assert(!strcmp(app.nameEdit, "CAD"));
  tap(lab::X, 14000000);
  assert(!strcmp(app.nameEdit, "CD"));
  tap(lab::Start, 15000000);
  assert(!app.naming && !strcmp(app.project.tracks[0].name, "CD"));
  app.beginName(2, "CD");
  app.action(lab::X);
  app.action(lab::Start);
  assert(app.naming && !app.nameEdit[0]);
  for (int i = 0; i < 30; ++i)
    app.insertNameCharacter('Z');
  assert(strlen(app.nameEdit) == NameLimit);
  app.nameKey = NameDelete;
  app.action(lab::A);
  assert(strlen(app.nameEdit) == NameLimit - 1);
  app.action(lab::B);
  assert(!app.naming && !strcmp(app.project.tracks[0].name, "CD"));
  app.beginName(2, "KEEP");
  app.nameKey = NameSave;
  app.action(lab::A);
  assert(!app.naming && !strcmp(app.project.tracks[0].name, "KEEP"));
  app.beginName(2, "");
  app.nameKey = 0;
  app.navigate(lab::Left);
  assert(app.nameKey == 9);
  app.navigate(lab::Up);
  assert(app.nameKey == NameSave);
  app.action(lab::B);
  static Codec codec, again;
  assert(codec.encode(app.project));
  Project loaded;
  assert(codec.decode(loaded, codec.cursor));
  assert(loaded.tracks[0].hits[0][0].length == 4 && again.encode(loaded));
  assert(codec.cursor == again.cursor &&
         !memcmp(codec.bytes, again.bytes, codec.cursor));
  uint32_t random = 357;
  for (int i = 0; i < 100000; ++i) {
    random = random * 1664525u + 1013904223u;
    int value = int(random % 2000001) - 1000000, gain = int(random % 10001);
    assert(Engine::scaleTrack(value, gain) ==
           int(int64_t(value) * gain / 10000));
    int pcm = int(random % 65536) - 32768, vel = int(random % 128),
        level = int(random % 101), envelope = int(random % 129);
    int old = int(int64_t(pcm) * envelope * vel * level / (128 * 127 * 100));
    int optimized =
        int(float(pcm * envelope) * (float(vel * level) / (128 * 127 * 100)));
    assert(abs(old - optimized) <= 1);
  }
  char previous[MAX_PATH], folder[MAX_PATH];
  GetCurrentDirectoryA(sizeof(previous), previous);
  snprintf(folder, sizeof(folder), "build/v051-%lu", GetCurrentProcessId());
  makeDir(folder);
  assert(SetCurrentDirectoryA(folder));
  makeDir("captures");
  CaptureStore store;
  app.captureStore = &store;
  auto finishWrite = [&]() {
    for (int attempt = 0; attempt < 5000 && app.captureWritePending;
         ++attempt) {
      Sleep(1);
      app.flushCapture();
    }
    assert(!app.captureWritePending);
  };
  engine.capture.begin(8000, 256, -1);
  for (int n = 0; n < 256; ++n)
    engine.capture.append(n * 50, -n * 50);
  app.flushCapture();
  assert(app.captureWritePending && engine.capture.state == 2);
  engine.render(audio, 1024);
  finishWrite();
  assert(!app.captureFailed && engine.capture.state == 0 &&
         app.source.frames == 256);
  engine.capture.begin(8000, 256, -1);
  for (int n = 0; n < 256; ++n)
    engine.capture.append(n * 30, -n * 30);
  assert(rename("captures", "captures-off") == 0);
  app.flushCapture();
  finishWrite();
  assert(app.captureFailed && engine.capture.state == 2 &&
         engine.capture.frames == 256);
  assert(rename("captures-off", "captures") == 0);
  app.captureFailed = false;
  app.flushCapture();
  finishWrite();
  assert(!app.captureFailed && engine.capture.state == 0 &&
         exists("captures/take-0002.wav"));
  app.captureStore = nullptr;
  makeDir("projects");
  app.haveUndo = false;
  app.project = Project{};
  app.project.trackCount = 1;
  app.slot = 0;
  app.project.tracks[0].keys[0].asset = 31;
  assert(app.save() && app.assetProtected(31));
  app.project.tracks[0].keys[0].asset = -1;
  assert(app.assetProtected(31)); // still referenced by the saved version
  assert(app.save() && !app.assetProtected(31));
  app.beginName(1, "KEEP ON FAILURE");
  char oldName[25];
  snprintf(oldName, sizeof(oldName), "%s", app.project.name);
  assert(rename("projects", "projects-off") == 0);
  app.action(lab::Start);
  assert(app.naming && !strcmp(app.nameEdit, "KEEP ON FAILURE") &&
         !strcmp(app.project.name, oldName));
  assert(rename("projects-off", "projects") == 0);
  app.action(lab::Start);
  assert(!app.naming && !strcmp(app.project.name, "KEEP ON FAILURE"));
  app.beginName(3, "RETRY CREATION");
  assert(rename("projects", "projects-off") == 0);
  app.action(lab::Start);
  assert(app.naming && !strcmp(app.nameEdit, "RETRY CREATION"));
  assert(rename("projects-off", "projects") == 0);
  app.action(lab::Start);
  assert(!app.naming && !strcmp(app.project.name, "RETRY CREATION"));
  assert(SetCurrentDirectoryA(previous));
  puts("PASS v051: MOS5 sample-map migration, MOS6 LENGTH persistence, "
       "cap/one-shot, Y B A X gestures, shoulders/release order, 2-FX limit, "
       "contextual menus, exact mixer scaling and sampler <=1 LSB gain error");
  puts("PASS background capture: immutable buffer, playback while writing, "
       "failure keeps PCM, successful retry");
  puts("PASS names: character panel, replacement, insertion, deletion, cursor, "
       "limit, cancel, failed rename/create retry; saved sample protection "
       "updates");
}
