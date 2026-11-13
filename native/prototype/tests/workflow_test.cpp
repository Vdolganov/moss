#include "../src/app.h"
#undef NDEBUG
#include <assert.h>
#include <windows.h>
using namespace moss;
static App app;
static Engine engine;
static Library library;
static int16_t audio[2048];
static void finishName(const char *n) {
  memset(app.nameEdit, ' ', 24);
  memcpy(app.nameEdit, n, strlen(n));
  app.nameEdit[24] = 0;
  app.action(lab::Start);
}
static void projectWorkflow() {
  makeDir("projects");
  app.engine = &engine;
  app.library = &library;
  engine.init(app.project, library.pcm, 44100, Assets);
  app.seedProjects();
  assert(app.projectCount == 1 && !strcmp(app.projectNames[0], "SLOW SIGNAL"));
  app.projectCursor = app.projectCount;
  app.action(lab::A);
  assert(app.sequencePicker == 3);
  app.action(lab::A); // Empty sequence, rather than a copy.
  assert(app.naming && app.nameTarget == 3);
  finishName("NIGHT WINDOW");
  assert(app.page == Grid && app.project.trackCount == 0 &&
         !strcmp(app.project.name, "NIGHT WINDOW") && app.projectCount == 2);
  app.action(lab::A);
  assert(app.voicePicker == 3);
  app.voiceChoice = 1;
  app.action(lab::B);
  assert(!app.project.trackCount);
  app.page = Grid;
  app.action(lab::A);
  app.voiceChoice = MonoInstrument;
  app.action(lab::A);
  assert(app.page == Pattern && app.project.trackCount == 1);
  app.page = Grid;
  app.option = app.optionIndex(4);
  app.chooseOption();
  assert(app.manageMenu == 2);
  app.option = 0;
  app.chooseOption();
  finishName("LOW SIGNAL");
  assert(!strcmp(app.project.tracks[0].name, "LOW SIGNAL"));
  app.project.tracks[0].steps[0].on = 1;
  app.project.tracks[0].steps[0].note = 31;
  app.duplicateTrack();
  assert(app.project.trackCount == 2 && app.track == 1 &&
         app.project.tracks[1].steps[0].note == 31);
  assert(!strcmp(app.project.tracks[1].name, "LOW SIGNAL COPY"));
  app.removeTrack();
  assert(app.project.trackCount == 1);
  app.option = 1;
  app.chooseOption();
  assert(app.project.trackCount == 2);
  while (app.project.trackCount < Tracks)
    app.addTrack(FmInstrument);
  app.addTrack(1);
  assert(app.project.trackCount == Tracks);
  assert(app.save());
  Project loaded;
  char path[128];
  app.path(path, sizeof(path), app.slot);
  assert(loadProject(path, loaded));
  assert(loaded.trackCount == Tracks && !strcmp(loaded.name, "NIGHT WINDOW"));
  app.page = Grid;
  app.duplicateProject();
  assert(app.projectCount == 3 &&
         !strcmp(app.project.name, "NIGHT WINDOW COPY"));
  app.removeProject();
  assert(app.page == Atlas && app.projectCount == 2 &&
         exists("projects/deleted.moss"));
  app.restoreProject();
  assert(app.projectCount == 3 && !exists("projects/deleted.moss"));
  app.projectCursor = 0;
  assert(app.selectedProject());
  app.page = Grid;
  while (app.project.trackCount)
    app.removeTrack();
  assert(app.page == Grid && app.track == 0);
  engine.init(app.project, library.pcm, 44100, Assets);
  engine.handle({Op::Transport});
  engine.render(audio, 1024);
  for (int16_t n : audio)
    assert(n == 0);
  // More than the original four slots; list indexing does not reuse active
  // slot.
  for (int i = 0; i < 5; i++) {
    char n[25];
    snprintf(n, sizeof(n), "PROJECT %d", i);
    app.createProject(n);
  }
  assert(app.projectCount == 8);
  app.backToAtlas();
  app.projectCursor = 7;
  app.action(lab::A);
  assert(app.requestedSlot == app.projectRows[7]);
  assert(app.open(app.requestedSlot));
  assert(!strcmp(app.project.name, "PROJECT 4"));
  puts("PASS projects/tracks: named empty creation, picker cancellation, "
       "add/rename/duplicate/delete/undo, four tracks, >4 projects, "
       "save/reopen, project restore, empty playback");
}
static void wav(const char *path, unsigned rate, unsigned channels,
                unsigned frames) {
  FILE *f = fopen(path, "wb");
  assert(f);
  fwrite("RIFF", 1, 4, f);
  put32(f, 36 + frames * channels * 2);
  fwrite("WAVEfmt ", 1, 8, f);
  put32(f, 16);
  put16(f, 1);
  put16(f, channels);
  put32(f, rate);
  put32(f, rate * channels * 2);
  put16(f, channels * 2);
  put16(f, 16);
  fwrite("data", 1, 4, f);
  put32(f, frames * channels * 2);
  for (unsigned i = 0; i < frames; i++)
    for (unsigned c = 0; c < channels; c++)
      put16(f,
            unsigned((i % 128 < 64 ? 12000 : -12000) * (c ? -1 : 1)) & 65535);
  fclose(f);
}
static void tapeWorkflow() {
  makeDir("imports");
  wav("imports/long.wav", 44100, 2, 44100 * 20);
  app.openSource("imports/long.wav");
  app.service();
  assert(app.sliceCount == 1);
  assert(app.sliceFrames(app.slices[0]) * 4 <= SampleBudget);
  int originalEnd = app.slices[0].end;
  app.buttons.held = lab::bit(lab::Down);
  app.edit(1, 1, 100);
  app.buttons.held = 0;
  assert(app.slices[0].end > originalEnd && app.slices[0].start > 0);
  assert(app.sliceFrames(app.slices[0]) * 4 <= SampleBudget);
  int end = app.slices[0].end;
  app.edit(0, -1, 100);
  assert(app.slices[0].end < end);
  assert(app.sliceFrames(app.slices[0]) * 4 <= SampleBudget);
  for (int i = 0; i < 300; i++) {
    app.edit(i % 2, i % 3 ? 1 : -1, 1 + i % 1300);
    assert(app.slices[0].end > app.slices[0].start);
    assert(app.sliceFrames(app.slices[0]) * 4 <= SampleBudget);
  }
  app.slices[0] = {5000, 10000, 10, 20};
  app.limitSlice(app.slices[0], 0);
  unsigned selected = app.sliceFrames(app.slices[0]);
  app.job = App::PreviewSlice;
  app.service();
  assert(app.previewing());
  uint64_t energy = 0;
  unsigned blocks = 0;
  while (app.previewing()) {
    app.pumpPreview();
    engine.render(audio, 1024);
    for (int n : audio)
      energy += uint64_t(int64_t(n) * n);
    assert(++blocks < 1000);
  }
  assert(energy > 1000000 && engine.stream.readIndex == selected &&
         engine.stream.shownFrame == 441000 && engine.stream.underruns == 0);
  app.job = App::PreviewSource;
  app.service();
  assert(app.previewing() && engine.stream.total == 882000);
  blocks = 0;
  while (app.previewing()) {
    app.pumpPreview();
    engine.render(audio, 1024);
    assert(++blocks < 2000);
  }
  assert(engine.stream.readIndex == 882000 && engine.stream.underruns == 0);
  app.previewToggle(App::PreviewSlice);
  app.service();
  assert(app.previewing());
  app.previewToggle(App::PreviewSlice);
  app.service();
  assert(!app.previewing() && !app.streamReader.file);
  // Resampling, mono/stereo budgets, rounded EOF and immutable source file.
  for (unsigned rate : {8000u, 22050u, 48000u, 96000u})
    for (unsigned ch : {1u, 2u}) {
      wav("imports/rate.wav", rate, ch, rate * 4 + 7);
      assert(app.source.open("imports/rate.wav"));
      app.slices[0] = {0, app.durationMs(), 0, 0};
      app.limitSlice(app.slices[0], 0);
      assert(app.sliceFrames(app.slices[0]) * ch * 2 <= SampleBudget);
      app.job = App::PreviewSource;
      app.service();
      blocks = 0;
      while (app.previewing()) {
        app.pumpPreview();
        engine.render(audio, 1024);
        assert(++blocks < 1000);
      }
      assert(engine.stream.readIndex == rate * 4 + 7 &&
             engine.stream.underruns == 0);
    }
  puts("PASS TAPE: sliding 1 MiB trim in both directions, randomized bounds, "
       "selected/full 20-second WAV streaming, fades, playhead, toggle stop, "
       "mono/stereo 8-96 kHz, no underruns with regular producer");
}
static void maximumTracks() {
  Project p = makeSynthDemo();
  p.trackCount = Tracks;
  p.bpm = 120;
  p.master = 50;
  for (int t = 0; t < Tracks; t++) {
    auto &tr = p.tracks[t];
    tr.instrument = 1 + t % 3;
    tr.level = 60;
    tr.cycle = 7 + t;
    tr.fx.enabled = 1;
    tr.fx.type = t % 3;
    tr.steps[0].on = 1;
    tr.steps[0].length = tr.cycle;
    tr.steps[0].note = 36 + t * 4;
  }
  p.fx.enabled = 1;
  p.fx.type = 1;
  engine.init(p, nullptr, 44100, 0);
  engine.handle({Op::Transport});
  engine.handle({Op::CaptureStart, 0, 0, 0, 8});
  uint64_t energy = 0;
  int peak = 0;
  for (unsigned remaining = 44100 * 60; remaining;) {
    unsigned n = remaining > 1024 ? 1024 : remaining;
    engine.render(audio, n);
    remaining -= n;
    for (unsigned i = 0; i < n * 2; i++) {
      int v = abs(audio[i]);
      if (v > peak)
        peak = v;
      energy += uint64_t(v) * v;
    }
  }
  for (int t = 0; t < Tracks; t++)
    assert(engine.triggerCount[t] > 1 &&
           engine.shownStep[t] == engine.tick % unsigned(p.tracks[t].cycle));
  assert(energy > 10000000 && peak < 32767 && engine.capture.state == 2);
  printf("PASS four-track 60-second stress: synths, per-track FX + master "
         "POND, capture, independent cycles; peak %d (desktop only)\n",
         peak);
}
int main() {
  char original[MAX_PATH], dir[MAX_PATH];
  GetCurrentDirectoryA(sizeof(original), original);
  snprintf(dir, sizeof(dir), "build/workflow-%lu", GetCurrentProcessId());
  makeDir(dir);
  assert(SetCurrentDirectoryA(dir));
  projectWorkflow();
  tapeWorkflow();
  maximumTracks();
  assert(SetCurrentDirectoryA(original));
}
