#include "../src/app.h"
#undef NDEBUG
#include <assert.h>
#include <time.h>
using namespace moss;
static Library library;
static Engine engine, reference;
static App app;
static int16_t output[2048], other[2048];
static lab::Canvas canvas;
static void advance(Engine &e, unsigned count) {
  while (count) {
    unsigned n = count > 1024 ? 1024 : count;
    e.render(output, n);
    count -= n;
  }
}
static void gesture(int modifier, int arrow, uint64_t time,
                    bool modifierFirst = true) {
  app.button(modifier, true, time);
  app.button(arrow, true, time + 10000);
  app.button(modifierFirst ? modifier : arrow, false, time + 20000);
  app.tick(time + 600000);
  app.button(modifierFirst ? arrow : modifier, false, time + 610000);
}
static void timelineAndGestures() {
  app = App{};
  app.engine = &engine;
  app.library = &library;
  app.project = makeProject(false);
  app.page = Pattern;
  app.bank = 3;
  engine.init(app.project, library.pcm, 8000, Assets);
  auto &t = app.project.tracks[0];
  t.division = 8;
  t.cycle = 15 * 8;
  t.hits[0][0].on = t.hits[3][9].on = 1;
  t.steps[9].note = 61;
  t.steps[9].length = 17;
  for (int division : {16, 32, 64}) {
    app.edit(1, 1, 1);
    assert(t.division == division && t.cycle == 120 &&
           t.cells() == 15 * division / 8);
    assert(t.hits[3][9].on && t.steps[9].length == 17);
  }
  for (int division : {32, 16, 8}) {
    app.edit(1, -1, 1);
    assert(t.division == division && t.cycle == 120);
  }
  app.bank = 0;
  app.key = 0;
  app.step = 0;
  gesture(lab::L1, lab::Right, 1000000);
  assert(app.key == 1 && app.track == 0 && app.step == 0);
  gesture(lab::L1, lab::Right, 2000000, false);
  assert(app.key == 2 && app.track == 0 && app.step == 0);
  app.key = 3;
  t.keys[3].asset = 0;
  app.step = 8;
  app.action(lab::A);
  assert(t.hits[3][8].on && !t.hits[0][8].on);
  gesture(lab::B, lab::Right, 3000000);
  assert(t.hits[3][8].fadeIn == 1 && app.page == Pattern && app.step == 8);
  app.bank = 1;
  app.edit(0, 1, 2);
  assert(t.hits[3][8].locks[0] == t.keys[3].pitch + 2 &&
         t.hits[0][8].locks[0] == Inherit);
  t.instrument = MonoInstrument;
  app.synthBank = 0;
  gesture(lab::L1, lab::Right, 4000000);
  assert(app.synthBank == 1 && app.track == 0);
  app.edit(2, 1, 12);
  assert(t.steps[8].synthLocks[0][6] == t.synthParams[0][6] + 12);
  assert(t.steps[8].synthLocks[0][2] == Inherit);
  app.page = Sampler;
  app.parameterPage(1);
  assert(app.synthBank == 0);
  app.parameterPage(1);
  app.edit(3, 1, 5);
  assert(t.synthParams[0][7] == 25);
  app.page = Space;
  app.fxDetail = false;
  for (int i = 0; i < 4; i++) {
    app.action(lab::X);
    if (i < 3)
      app.navigate(lab::Down);
  }
  for (int i = 0; i < 4; i++)
    assert(t.fx.active(i) == (i < ActiveEffects));
  t.steps[8].fxLocks[3][1] = 67;
  gesture(lab::Y, lab::Up, 5000000);
  assert(t.fx.order[2] == 3 && t.fx.order[3] == 2 &&
         t.steps[8].fxLocks[3][1] == 67);
  app.action(lab::A);
  assert(app.fxDetail);
  app.action(lab::B);
  assert(!app.fxDetail && app.page == Space);
  app.page = Pattern;
  app.bank = 2;
  app.nested(-1);
  assert(t.fx.type == 1);
  app.edit(1, 1, 2);
  assert(t.steps[8].fxLocks[1][1] != Inherit && t.steps[8].fxLocks[3][1] == 67);
  t.division = 8;
  t.cycle = 15;
  app.bank = 3;
  draw(canvas, app);
  assert(valid(app.project));
  puts("PASS v05 interaction: resolution roundtrip/off-grid preservation, "
       "independent drum lanes, nested release orders, synth banks/locks, "
       "chain toggles/order/identity locks");
}
static void sound() {
  Project p = makeProject(false);
  p.trackCount = 1;
  p.bpm = 120;
  auto &t = p.tracks[0];
  t.cycle = 512;
  for (int k = 0; k < Keys; k++) {
    t.keys[k].asset = 0;
    t.keys[k].pitch = 0;
    t.keys[k].mode = 2;
    t.hits[k][0].on = 1;
  }
  engine.init(p, library.pcm, 8000, Assets);
  engine.handle({Op::Transport});
  advance(engine, 1);
  assert(engine.triggerCount[0] == Keys);
  for (int k = 0; k < Keys; k++)
    assert(engine.voice(0, k).active && engine.voice(0, k).mode == 0);
  advance(engine, 8100);
  for (int k = 0; k < Keys; k++)
    assert(!engine.voice(0, k).active);
  assert(engine.triggerCount[0] == Keys); // no looping or additional triggers
  for (int k = 1; k < Keys; k++)
    t.hits[k][0].on = 0;
  t.fx.enabled = 1;
  t.fx.values[0][0] = 0;
  t.fx.values[0][1] = 80;
  t.fx.values[0][2] = 0;
  t.steps[0].fxLocks[0][2] = 75;
  engine.init(p, library.pcm, 8000, Assets);
  engine.handle({Op::Transport});
  advance(engine, 8100);
  long long tailEnergy = 0;
  for (int n = 0; n < 8000; n += 1000) {
    engine.render(output, 1000);
    for (int i = 0; i < 2000; i++)
      tailEnergy += abs(output[i]);
  }
  assert(tailEnergy > 10000 && engine.activeFxStep[0] == 0 &&
         !engine.voices[0].active);
  assert(Engine::monitor(10000) == 18000 && Engine::monitor(-10000) == -18000);
  assert(Engine::monitor(50000) < 32767 && Engine::monitor(-50000) > -32767);
  t.division = 8;
  engine.init(p, library.pcm, 8000, Assets);
  t.division = 64;
  reference.init(p, library.pcm, 8000, Assets);
  engine.handle({Op::Transport});
  reference.handle({Op::Transport});
  for (int n = 0; n < 160; n++) {
    engine.render(output, 1024);
    reference.render(other, 1024);
    assert(!memcmp(output, other, sizeof(output)));
  }
  assert(engine.triggerCount[0] == reference.triggerCount[0]);
  puts("PASS v05 audio: four simultaneous keys, forced one-shots, delay lock "
       "tails over rests, 180% monitor gain/soft peak protection, resolution leaves "
       "audio duration unchanged");
}
static void namedImport() {
  app = App{};
  app.engine = &engine;
  app.library = &library;
  app.project = makeProject(false);
  app.page = Sampler;
  app.key = 3;
  engine.init(app.project, library.pcm, 8000, Assets);
  app.pickAsset();
  assert(app.asset == -1 && app.assignMode);
  app.action(lab::A);
  assert(app.filePicker && app.importAssign);
  app.action(lab::B);
  assert(!app.filePicker && app.assignMode);
  app.action(lab::B);
  assert(app.page == Sampler && app.project.tracks[0].keys[3].asset == -1);
  app.pickAsset();
  app.action(lab::A);
  app.openSource("imports/source.wav");
  app.service();
  assert(app.importAssign && app.page == Tape && app.tapeTab == 1);
  int before = library.count();
  app.action(lab::A);
  assert(app.naming && app.nameTarget == 4);
  app.action(lab::B);
  assert(!app.naming && library.count() == before && app.importAssign);
  app.action(lab::A);
  snprintf(app.nameEdit, 25, "KICK DUST");
  app.action(lab::Start);
  app.service();
  int id = app.project.tracks[0].keys[3].asset;
  assert(id >= 0 && app.page == Sampler && app.key == 3 && !app.importAssign &&
         !strcmp(library.names[id], "KICK DUST"));
  assert(engine.project.tracks[0].keys[3].asset == id &&
         library.count() == before + 1);
  library.init();
  assert(!strcmp(library.names[id], "KICK DUST"));
  app.key = 2;
  app.project.tracks[0].keys[2].asset = -1;
  app.pickAsset();
  app.action(lab::A);
  app.openSource("imports/source.wav");
  app.service();
  assert(rename("library", "library-off") == 0);
  app.action(lab::A);
  snprintf(app.nameEdit, 25, "SNARE");
  app.action(lab::Start);
  app.service();
  assert(app.importAssign && app.project.tracks[0].keys[2].asset == -1 &&
         !strcmp(app.saveLabel, "SNARE"));
  assert(rename("library-off", "library") == 0);
  app.action(lab::A);
  assert(app.naming);
  app.action(lab::Start);
  app.service();
  assert(app.page == Sampler && app.project.tracks[0].keys[2].asset >= 0);
  puts("PASS v05 import: picker entry, cancel at browser/trim/name, named "
       "persistence, automatic assignment/return, failed disk save keeps "
       "destination/name and retries");
}
static void chainStress() {
  Project p = makeSynthDemo();
  p.trackCount = Tracks;
  p.bpm = 70;
  for (int n = 0; n < Tracks; n++) {
    auto &t = p.tracks[n];
    t.instrument = n % 3 + 1;
    t.cycle = (15 + n) * 4;
    t.steps[0].on = 1;
    t.steps[0].note = 36 + n * 3;
    t.steps[0].length = t.cycle;
    t.fx.enabled = 1;
    for (int &v : t.fx.enabledTypes)
      v = 1;
    if (n & 1)
      t.fx.move(3, -1);
  }
  p.fx.enabled = 1;
  for (int &v : p.fx.enabledTypes)
    v = 1;
  engine.init(p, nullptr, 44100, 0);
  engine.handle({Op::Transport});
  engine.handle({Op::CaptureStart, 0, 0, 0, 1});
  clock_t start = clock();
  long long energy = 0;
  int peak = 0;
  for (int frames = 0; frames < 441000; frames += 1000) {
    engine.render(output, 1000);
    for (int i = 0; i < 2000; i++) {
      energy += abs(output[i]);
      if (abs(output[i]) > peak)
        peak = abs(output[i]);
    }
  }
  assert(energy > 1000000 && peak < 32767 && engine.seenFxMask == 15 &&
         engine.capture.state == 2);
  printf("PASS v05 10-second stress: 4 synth tracks, transient full-chain load, "
         "capture; peak %d; desktop CPU %.3fs (ARM pending)\n",
         peak, double(clock() - start) / CLOCKS_PER_SEC);
  printf("SIZES Project=%zu Engine=%zu App=%zu FxState=%zu Codec=%zu\n",
         sizeof(Project), sizeof(Engine), sizeof(App), sizeof(FxState),
         sizeof(Codec));
}
static void characterResponse() {
  SynthTables table;
  table.init();
  for (int kind = 1; kind <= 3; kind++)
    for (int parameter = 0; parameter < 8; parameter++) {
      int low[8], high[8];
      for (int i = 0; i < 8; i++)
        low[i] = high[i] = presetValues(kind, 0)[i];
      low[parameter] = synthMin(kind, parameter);
      high[parameter] = synthMax(kind, parameter);
      SynthVoice a, b;
      a.on(kind, 43, table.frequency[43], 110, 64, low, 0, 8000, 3);
      b.on(kind, 43, table.frequency[43], 110, 64, high, 0, 8000, 3);
      long long difference = 0;
      for (int frame = 0; frame < 16000; frame++) {
        if (frame == 8000) {
          a.off();
          b.off();
        }
        int l = 0, r = 0, x = 0, y = 0;
        a.render(l, r, table);
        b.render(x, y, table);
        difference += abs(l - x);
      }
      assert(difference > 10000);
    }
  puts("PASS v05 character: each of 24 synth controls measurably changes "
       "rendered attack, held tone or release");
}
int main() {
  char original[512], dir[128];
  GetCurrentDirectoryA(512, original);
  snprintf(dir, sizeof(dir), "build/v05-%lu", GetCurrentProcessId());
  makeDir(dir);
  assert(SetCurrentDirectoryA(dir));
  makeDir("library");
  makeDir("imports");
  makeDir("captures");
  makeDir("projects");
  for (int n = 0; n < 8000; n++)
    library.scratch.pcm[n] = int16_t(sin(n * .173) * 10000);
  assert(writeWav("imports/source.wav", library.scratch.pcm, 8000, 8000, 1));
  assert(library.pcm[0].load("imports/source.wav"));
  strcpy(library.names[0], "SOURCE");
  assert(writeWav("library/asset-00.wav", library.pcm[0].pcm, 8000, 8000, 1));
  assert(library.saveName(0));
  timelineAndGestures();
  sound();
  namedImport();
  characterResponse();
  chainStress();
  assert(SetCurrentDirectoryA(original));
}
