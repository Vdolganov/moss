#include "../src/app.h"
#undef NDEBUG
#include <assert.h>
using namespace moss;
static Engine engine;
static App app;
static moss::SampleBuffer samples[3];
static int16_t pcm[2048];
static void reset(Page page) {
  app = App{};
  app.engine = &engine;
  app.page = page;
  app.gridPageSize = 16; // This legacy test exercises the 16-cell view.
  app.step = 44;
  app.track = 1;
  engine.init(app.project, samples, 44100);
}
int main() {
  for (int i = 0; i < 3; i++)
    assert(samples[i].load(lab::sampleFiles[i]));
  const Page pages[] = {Grid, Mix, Tape, Pattern, Sampler, Space};
  for (auto page : pages) {
    reset(page);
    int bpm = app.project.bpm;
    auto step = app.project.tracks[1].steps[44];
    app.button(lab::Select, true, 1000000);
    app.button(lab::Right, true, 1020000);
    app.button(lab::R2, true, 1030000);
    app.button(lab::R2, false, 1040000);
    app.tick(3020000);
    assert(app.project.bpm == bpm + 6);
    assert(app.undoProject.bpm == bpm);
    app.button(lab::Right, false, 3030000);
    app.button(lab::Select, false, 3040000);
    assert(app.page == page && app.step == 44 && app.track == 1 &&
           !app.options);
    assert(!memcmp(&step, &app.project.tracks[1].steps[44], sizeof(step)));
    engine.render(pcm, 1);
    assert(engine.project.bpm == bpm + 6);
  }
  reset(Pattern);
  app.button(lab::Select, true, 1000000);
  app.button(lab::Left, true, 1010000);
  app.button(lab::Select, false, 1020000);
  app.tick(2000000);
  app.button(lab::Left, false, 2100000);
  assert(app.step == 44 && app.page == Pattern && app.project.bpm == 69);
  app.button(lab::Right, true, 2200000);
  app.button(lab::Right, false, 2210000);
  assert(app.step == 48);
  reset(Pattern);
  app.button(lab::Down, true, 1000000);
  app.button(lab::Down, false, 1010000);
  assert(app.step == 60);
  app.project.tracks[1].cycle = 256;
  app.step = 28;
  app.navigate(lab::Down);
  assert(app.step == 60);
  app.navigate(lab::Right);
  assert(app.step == 64);
  app.navigate(lab::Up);
  assert(app.step == 32);
  reset(Pattern);
  app.project.bpm = 240;
  app.button(lab::Select, true, 1000000);
  app.button(lab::Right, true, 1010000);
  assert(app.project.bpm == 240);
  app.clearInput();
  app.tick(3000000);
  assert(app.project.bpm == 240 && app.buttons.held == 0);
  reset(Sampler);
  app.options = true;
  app.button(lab::Select, true, 1000000);
  app.button(lab::Select, false, 1010000);
  assert(!app.options && app.page == Sampler);
  reset(Grid);
  app.button(lab::Select, true, 1000000);
  app.button(lab::Select, false, 1010000);
  assert(app.page == Atlas);
  reset(Grid);
  app.button(lab::Select, true, 1000000);
  app.button(lab::Select, false, 2000000);
  assert(app.page == Grid);
  reset(Grid);
  app.button(lab::Select, true, 1000000);
  app.button(lab::R1, true, 1010000);
  app.button(lab::Select, false, 1600000);
  app.button(lab::R1, false, 1610000);
  assert(app.track == 1 && app.page == Grid);
  reset(Pattern);
  app.project.tracks[1].keys[app.project.tracks[1].steps[44].key].mode = 0;
  int len = app.project.tracks[1].steps[44].length;
  app.edit(2, 1, 1);
  assert(app.project.tracks[1].steps[44].length == len);
  reset(Mix);
  int bpm = app.project.bpm;
  app.edit(3, 1, 1);
  assert(app.project.tracks[1].mute == 1 && app.project.bpm == bpm);
  reset(Pattern);
  app.bank = 3;
  bpm = app.project.bpm;
  app.edit(1, 1, 1);
  assert(app.project.bpm == bpm);
  reset(Pattern);
  engine.handle({Op::Transport});
  engine.render(pcm, 1024);
  auto clock = engine.clock;
  auto tick = engine.tick;
  engine.handle({Op::Global, 0, 0, 0, 130});
  assert(engine.clock == clock && engine.tick == tick && engine.playing);
  puts("PASS interaction: global tempo, acceleration, both release orders, no "
       "navigation or step edits, undo grouping, bounds, focus loss, short "
       "SELECT, 8-column grid, one-shot length and mixer mute");
}
