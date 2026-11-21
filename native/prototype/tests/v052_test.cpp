#include "../src/app.h"
#undef NDEBUG
#include <assert.h>
#include <windows.h>
using namespace moss;
static App app;
static Engine engine, reference;
static Library library;
static SongBank bank;
static int16_t audio[2048], other[2048];
static uint64_t now = 1000000;
static void gesture(int face, int direction, bool releaseFaceFirst = true) {
  app.button(face, true, now);
  app.button(direction, true, now + 10000);
  app.button(releaseFaceFirst ? face : direction, false, now + 20000);
  app.tick(now + 600000);
  app.button(releaseFaceFirst ? direction : face, false, now + 610000);
  now += 1000000;
}
static void advance(Engine &e, unsigned n) {
  while (n) {
    unsigned block = n > 1024 ? 1024 : n;
    e.render(audio, block);
    n -= block;
  }
}
static void controls() {
  app.engine = &engine;
  app.library = &library;
  app.project = makeSynthDemo();
  app.gridPageSize = 16;
  engine.init(app.project, library.pcm, 8000, Assets);
  app.track = 1;
  app.page = Sampler;
  gesture(lab::Y, lab::Right);
  assert(app.project.tracks[1].synthParams[0][0] == 1201);
  gesture(lab::Y, lab::Up, false);
  assert(app.project.tracks[1].synthParams[0][0] == 1301);
  gesture(lab::Y, lab::Down);
  assert(app.project.tracks[1].synthParams[0][0] == 1201);
  app.page = Pattern;
  app.bank = 0;
  app.step = 0;
  int note = app.project.tracks[1].steps[0].note;
  gesture(lab::Y, lab::Up);
  assert(app.project.tracks[1].steps[0].note == note + 12);
  gesture(lab::B, lab::Right);
  int velocity = app.project.tracks[1].steps[0].velocity;
  gesture(lab::B, lab::Down);
  assert(app.project.tracks[1].steps[0].velocity == velocity - 10);
  app.bank = 1;
  gesture(lab::Y, lab::Right);
  assert(app.project.tracks[1].steps[0].synthLocks[0][0] == 1202);
  gesture(lab::Y, lab::Up);
  assert(app.project.tracks[1].steps[0].synthLocks[0][0] == 1302);
  app.project.tracks[1].cycle = 140;
  app.project.tracks[1].division = 16;
  app.step = 12;
  gesture(lab::R1, lab::Right);
  assert(app.step == 76 && app.bank == 1);
  gesture(lab::R1, lab::Right, false);
  assert(app.step == 136 && app.bank == 1);
  gesture(lab::R1, lab::Right);
  assert(app.step == 136);
  gesture(lab::R1, lab::Left);
  assert(app.step == 72 && app.bank == 1);
  app.options = true;
  app.option = 0;
  int beforeStep = app.step;
  gesture(lab::R1, lab::Right);
  assert(app.step == beforeStep);
  app.options = false;
  app.page = Mix;
  app.mixTab = 0;
  app.parameterPage(1);
  assert(app.mixTab == 1);
  app.fxDetail = true;
  app.project.fx.type = 2;
  app.project.fx.values[2][0] = 20;
  gesture(lab::Y, lab::Right);
  gesture(lab::Y, lab::Up);
  assert(app.project.fx.values[2][0] == 31);
  app.page = Tape;
  app.tapeTab = 1;
  app.sliceCount = 1;
  app.source.rate = 8000;
  app.source.frames = 80000;
  app.slices[0] = {100, 5000, 0, 0};
  gesture(lab::Y, lab::Right);
  gesture(lab::Y, lab::Up);
  assert(app.slices[0].start == 201);
  app.clearInput();
  app.dirty = false;
  app.songDirty = false;
  puts("PASS 052 controls: fine/coarse, notes/octaves, locks, FX, tape, page "
       "jumps, partial page, release orders, menu priority");
}
static void arrangement() {
  makeDir("projects");
  Project p = makeSynthDemo();
  p.trackCount = 2;
  p.tracks[0].cycle = 60;
  p.tracks[1].cycle = 64;
  strcpy(p.name, "ARRIVAL");
  assert(saveProject("projects/project-1.moss", p));
  assert(defaultBars(p) == 1);
  p.tracks[1].cycle = 65;
  assert(defaultBars(p) == 2);
  p.tracks[1].cycle = 64;
  app.page = Atlas;
  app.scan();
  app.atlasTab = 1;
  app.songCursor = app.songCount;
  app.action(lab::A);
  assert(app.naming && app.nameTarget == 6);
  app.navigate(lab::Right);
  assert(app.nameKey == 1);
  strcpy(app.nameEdit, "NIGHT TRAIN");
  app.action(lab::Start);
  assert(app.page == SongPage && !app.song.count && !app.naming);
  app.action(lab::A);
  assert(app.sequencePicker == 1);
  app.action(lab::A);
  assert(app.song.count == 1 && app.song.slots[0].bars == 1);
  gesture(lab::B, lab::Right);
  assert(app.song.slots[0].repeats == 2);
  gesture(lab::A, lab::Up);
  assert(app.song.slots[0].bars == 5);
  gesture(lab::A, lab::Down);
  assert(app.song.slots[0].bars == 1 && app.requestedSlot < 0);
  app.option = app.optionIndex(3);
  app.chooseOption();
  assert(app.song.count == 2 && app.songRow == 1);
  gesture(lab::Y, lab::Up);
  assert(app.songRow == 0);
  app.option = app.optionIndex(5);
  app.chooseOption();
  assert(app.naming && app.copySource == 0);
  strcpy(app.nameEdit, "BLOOM");
  app.action(lab::Start);
  assert(app.page == Grid && app.inSongSequence && app.slot == 1);
  assert(app.song.slots[0].sequence == 1 && app.song.slots[1].sequence == 0);
  Project copied;
  assert(loadProject("projects/project-2.moss", copied));
  assert(copied.tracks[0].cycle == 60 && !strcmp(copied.name, "BLOOM"));
  app.backToAtlas();
  assert(app.page == SongPage);
  assert(app.sequenceInSong(0));
  app.project = p;
  app.slot = 0;
  app.removeProject();
  assert(exists("projects/project-1.moss"));
  assert(app.saveArrangement());
  Song loaded;
  assert(loadSong("projects/song-1.moss", loaded));
  assert(loaded.count == 2 && loaded.slots[1].repeats == 2 &&
         loaded.bpm == 120);
  SongCodec codec;
  assert(codec.encode(loaded));
  codec.bytes[55] ^= 1;
  assert(!codec.decode(loaded));
  app.songRow = 0;
  app.action(lab::Start);
  assert(app.job == App::StartSong);
  app.service();
  assert(engine.playing && engine.project.bpm == 120 && engine.songBank);
  int count = app.song.count;
  app.action(lab::X);
  assert(!app.sequencePicker && count == app.song.count);
  engine.handle({Op::Stop});
  app.clearInput();
  // Cancel/failed writes keep the edit available, never discard a sequence.
  app.beginName(7, app.song.name);
  assert(rename("projects", "projects-away") == 0);
  strcpy(app.nameEdit, "RETRY");
  app.action(lab::Start);
  assert(app.naming);
  assert(rename("projects-away", "projects") == 0);
  app.action(lab::Start);
  assert(!app.naming);
  assert(remove("projects/project-1.moss") == 0);
  app.prepareSong();
  assert(!engine.playing && strstr(app.status, "MISSING"));
  puts("PASS SONG workflow: empty creation, slots, repeat/bars, reorder, "
       "independent copy, return focus, protection, codec, save retry, missing "
       "sequence");
}
static void playback() {
  bank.song = Song{};
  bank.song.count = 2;
  bank.song.bpm = 120;
  bank.song.slots[0] = {0, 2, 1};
  bank.song.slots[1] = {1, 1, 1};
  bank.count = 2;
  bank.map[0] = 0;
  bank.map[1] = 1;
  for (int i = 0; i < 2; ++i) {
    auto &p = bank.sequences[i];
    p = makeSynthDemo();
    p.trackCount = 2;
    p.bpm = 70 + i * 30;
    p.tracks[0].cycle = 60;
    p.tracks[1].cycle = 64;
    for (int t = 0; t < 2; ++t)
      for (auto &step : p.tracks[t].steps)
        step.on = 0;
    for (int t = 0; t < 2; ++t) {
      auto &s = p.tracks[t].steps[0];
      s.on = 1;
      s.length = 512;
    }
  }
  engine.init(bank.at(0), library.pcm, 8000, Assets);
  engine.startSong(bank, 0);
  advance(engine, 16000);
  assert(engine.songSlot == 0 && engine.songTicks == 63);
  advance(engine, 1);
  assert(engine.songTicks == 64 && engine.shownSongRepeat == 1 &&
         engine.tick == 64);
  assert(engine.shownStep[0] == 4 &&
         engine.shownStep[1] == 0); // no reset on repetition
  advance(engine, 16000);
  assert(engine.songSlot == 1 && engine.songTicks == 0 && engine.tick == 0);
  assert(engine.project.bpm == 120 && engine.shownStep[0] == 0);
  advance(engine, 16000);
  assert(!engine.playing); // 3 bars total
  bank.song.loop = 1;
  engine.startSong(bank, 0);
  reference.init(bank.at(0), library.pcm, 8000, Assets);
  reference.startSong(bank, 0);
  for (int i = 0; i < 55; ++i) {
    engine.render(audio, 1024);
    for (int j = 0; j < 1024; ++j)
      reference.render(other + 2 * j, 1);
    assert(!memcmp(audio, other, sizeof(audio)));
  }
  assert(engine.playing && engine.songSlot == 0);
  engine.handle({Op::Global, 0, 0, 0, 180});
  assert(engine.songBpm == 180);
  advance(engine, 32000);
  assert(engine.project.bpm == 180);
  puts("PASS SONG audio: sample-exact bars, continuous 15/16 cycles inside "
       "repeats, local restart on slot change, global BPM, stop/loop, block "
       "invariance across transitions");
}
int main() {
  char previous[MAX_PATH], dir[MAX_PATH];
  GetCurrentDirectoryA(sizeof(previous), previous);
  snprintf(dir, sizeof(dir), "build/v052-%lu", GetCurrentProcessId());
  makeDir(dir);
  assert(SetCurrentDirectoryA(dir));
  controls();
  arrangement();
  playback();
  assert(SetCurrentDirectoryA(previous));
}
