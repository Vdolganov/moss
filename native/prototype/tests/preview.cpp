#include "../src/app.h"
#include <stdlib.h>
using namespace moss;
static moss::SampleBuffer samples[3];
static Engine engine;
static App app;
static Library library;
static lab::Canvas canvas;
static void little(FILE *f, unsigned n, int bytes) {
  for (int i = 0; i < bytes; i++)
    fputc((n >> (i * 8)) & 255, f);
}
int main(int argc, char **argv) {
  if (argc != 3 && argc != 4)
    return 1;
  for (int i = 0; i < 3; i++)
    if (!samples[i].load(lab::sampleFiles[i]))
      return 2;
  engine.init(app.project, samples, 44100);
  app.engine = &engine;
  app.page = Page(atoi(argv[1]));
  app.step = 28;
  app.track = 1;
  app.key = 3;
  for (int i = 0; i < 3; i++) {
    library.pcm[i] = samples[i];
    snprintf(library.names[i], 32, "%s", lab::sampleNames[i]);
  }
  app.library = &library;
  app.projectCount = 1;
  app.projectRows[0] = 0;
  snprintf(app.projectNames[0], 25, "SLOW SIGNAL");
  app.projectTracks[0] = 3;
  app.projectTempos[0] = 70;
  if (argc == 4) {
    if (!strcmp(argv[3], "drums") || !strcmp(argv[3], "drumlocks")) {
      app.page = Pattern;
      app.track = 0;
      app.key = 3;
      app.step = 16;
      auto &tr = app.project.tracks[0];
      tr = Track{};
      snprintf(tr.name, 25, "DUST KIT");
      for (int k = 0; k < Keys; k++) {
        tr.keys[k].asset = k % 3;
        tr.hits[k][0].on = 1;
      }
      for (int i : {0, 16, 32, 48})
        tr.hits[3][i].on = 1;
      tr.hits[3][16].fadeIn = 5;
      tr.hits[3][16].length = 16;
      if (!strcmp(argv[3], "drumlocks")) {
        app.bank = 1;
        tr.hits[3][16].locks[0] = -5;
        tr.hits[3][16].locks[2] = 70;
      }
    }
    if (!strcmp(argv[3], "resolution")) {
      app.page = Pattern;
      app.track = 0;
      app.bank = 3;
      auto &tr = app.project.tracks[0];
      tr.division = 64;
      tr.cycle = 120;
      app.step = 96;
    }
    if (!strcmp(argv[3], "fxchain") || !strcmp(argv[3], "reel")) {
      app.page = Space;
      app.fxDetail = !strcmp(argv[3], "reel");
      auto &f = app.project.tracks[app.track].fx;
      f.type = 3;
      f.enabled = 1;
      f.enabledTypes[1] = f.enabledTypes[3] = 1;
      f.move(3, -1);
    }
    if (!strcmp(argv[3], "monochar") || !strcmp(argv[3], "organismchar") ||
        !strcmp(argv[3], "fmchar") || !strcmp(argv[3], "charlocks")) {
      app.project = makeSynthDemo();
      app.page = Sampler;
      app.synthBank = 1;
      app.track = !strcmp(argv[3], "organismchar") ? 0
                  : !strcmp(argv[3], "fmchar")     ? 2
                                                   : 1;
      if (!strcmp(argv[3], "charlocks")) {
        app.page = Pattern;
        app.bank = 1;
        app.project.tracks[1].steps[28].synthLocks[0][6] = 35;
      }
    }
    if (!strcmp(argv[3], "importassign")) {
      app.page = Tape;
      app.tapeTab = 2;
      app.assignMode = true;
      app.asset = -1;
    }
    if (!strcmp(argv[3], "nameslice")) {
      app.importAssign = true;
      app.importTrack = app.track;
      app.importKey = app.key;
      app.openSource(lab::sampleFiles[0]);
      app.service();
      app.slices[0] = {100, 400, 5, 20};
      app.nameSlice();
      snprintf(app.nameEdit, sizeof(app.nameEdit), "KICK DUST");
    }

    if (!strcmp(argv[3], "emptygrid")) {
      app.page = Grid;
      app.project.trackCount = 0;
      snprintf(app.project.name, 25, "NIGHT WINDOW");
      app.track = 0;
    }
    if (!strcmp(argv[3], "name")) {
      app.page = Atlas;
      app.beginName(3, "NIGHT WINDOW");
    }
    if (!strcmp(argv[3], "tracks4")) {
      app.page = Grid;
      app.project = makeSynthDemo();
      app.project.trackCount = Tracks;
      for (int t = 3; t < Tracks; t++) {
        app.project.tracks[t].instrument = 1 + t % 3;
        snprintf(app.project.tracks[t].name, 25, "LAYER %d", t + 1);
      }
      app.track = Tracks - 1;
    }
    if (!strcmp(argv[3], "catalog")) {
      app.page = Atlas;
      app.projectCount = 8;
      app.projectCursor = 6;
      for (int i = 0; i < 8; i++) {
        app.projectRows[i] = i;
        snprintf(app.projectNames[i], 25, "NIGHT WINDOW %d", i + 1);
        app.projectTracks[i] = i;
        app.projectTempos[i] = 70;
      }
    }
    if (!strcmp(argv[3], "trim")) {
      app.page = Tape;
      app.tapeTab = 1;
      app.source.open(lab::sampleFiles[0]);
      app.source.overview();
      app.source.rate = 44100;
      app.source.channels = 2;
      app.source.frames = 44100 * 60;
      snprintf(app.source.path, 512,
               "imports/MA_HeatTransfer_80_A#_TapeStrings.wav");
      app.sliceCount = 1;
      app.sliceIndex = 0;
      app.slices[0] = {12000, 22000, 10, 20};
      app.limitSlice(app.slices[0], 1);
      engine.stream.state = 1;
      engine.stream.shownFrame = 44100 * 20;
      app.streamReader.source = app.source;
    }
    if (!strcmp(argv[3], "mono") || !strcmp(argv[3], "organism") ||
        !strcmp(argv[3], "fm") || !strcmp(argv[3], "synthlocks") ||
        !strcmp(argv[3], "synthgrid") || !strcmp(argv[3], "instrument")) {
      app.project = makeSynthDemo();
      app.track = !strcmp(argv[3], "organism") ? 0
                  : !strcmp(argv[3], "fm")     ? 2
                                               : 1;
      app.page = Sampler;
      if (!strcmp(argv[3], "synthlocks")) {
        app.page = Pattern;
        app.bank = 1;
        app.project.tracks[1].steps[28].synthLocks[0][0] = 400;
      }
      if (!strcmp(argv[3], "synthgrid"))
        app.page = Grid;
      if (!strcmp(argv[3], "instrument")) {
        app.voicePicker = 1;
        app.voiceChoice = 3;
      }
    }
    if (!strcmp(argv[3], "tempo")) {
      app.button(lab::Select, true, 1000000);
    }
    if (!strcmp(argv[3], "tail")) {
      app.project.tracks[1].cycle = 19;
      app.step = 18;
    }
    if (!strcmp(argv[3], "sustain")) {
      app.track = 0;
      app.step = 0;
    }
    if (!strcmp(argv[3], "locks")) {
      app.bank = 1;
      app.project.tracks[1].steps[28].locks[0] = 7;
    }
    if (!strcmp(argv[3], "cycle")) {
      app.bank = 3;
    }
    if (!strcmp(argv[3], "orbit")) {
      app.page = Space;
      app.fxDetail = true;
      app.project.tracks[1].fx.enabled = 1;
    }
    if (!strcmp(argv[3], "pond")) {
      app.page = Space;
      app.fxDetail = true;
      app.project.tracks[1].fx.type = 1;
      app.project.tracks[1].fx.enabled = 1;
    }
    if (!strcmp(argv[3], "crush")) {
      app.page = Space;
      app.fxDetail = true;
      app.project.tracks[1].fx.type = 2;
      app.project.tracks[1].fx.enabled = 1;
    }
    if (!strcmp(argv[3], "master")) {
      app.page = Mix;
      app.mixTab = 1;
      app.project.fx.type = 1;
      app.project.fx.enabled = 1;
    }
    if (!strcmp(argv[3], "edit")) {
      app.page = Tape;
      app.tapeTab = 1;
      app.source.open(lab::sampleFiles[0]);
      app.source.overview();
      app.sliceCount = 1;
      app.slices[0] = {100, 400, 10, 25};
    }
    if (!strcmp(argv[3], "library")) {
      app.page = Tape;
      app.tapeTab = 2;
      app.asset = 1;
    }
    if (!strcmp(argv[3], "files")) {
      app.page = Tape;
      app.filePicker = true;
      library.addSource("imports/MA_Aerofoil_80_A#_TapeGuitar.wav");
      library.addSource("imports/MA_Helios_80_A#_TapeSynth.wav");
    }
    if (!strcmp(argv[3], "fxlocks")) {
      app.bank = 2;
      app.project.tracks[1].fx.type = 2;
      app.project.tracks[1].steps[28].fxLocks[2][1] = 8;
    }
    if (!strcmp(argv[3], "poly15")) {
      app.track = 2;
      app.step = 14;
      engine.tick = 15;
      engine.boundary();
      engine.shownPlaying = 1;
    }
  }
  if (argc == 4 && !strncmp(argv[3], "menu-", 5)) {
    app.haveUndo = true;
    if (!strcmp(argv[3], "menu-sampler")) {
      app.page = Sampler;
      app.key = 0;
    } else if (!strcmp(argv[3], "menu-synth")) {
      app.page = Sampler;
      app.project = makeSynthDemo();
    } else if (!strcmp(argv[3], "menu-step")) {
      app.page = Pattern;
      app.bank = 0;
      app.key = 0;
    } else if (!strcmp(argv[3], "menu-locks")) {
      app.page = Pattern;
      app.bank = 2;
    } else if (!strcmp(argv[3], "menu-tape")) {
      app.openSource(lab::sampleFiles[0]);
      app.service();
      app.slices[0].end = 100;
      app.page = Tape;
      app.tapeTab = 1;
    } else
      app.page = Grid;
    app.options = true;
    app.option = 0;
  }
  if (argc == 4) {
    const char *mode = argv[3];
    if (!strcmp(mode, "mixlevels")) {
      app.page = Mix;
      app.mixTab = 0;
    }
    if (!strcmp(mode, "masterchain")) {
      app.page = Mix;
      app.mixTab = 1;
      app.fxDetail = false;
    }
    if (!strcmp(mode, "masterdetail")) {
      app.page = Mix;
      app.mixTab = 1;
      app.fxDetail = true;
    }
    if (!strcmp(mode, "song") || !strcmp(mode, "songempty") ||
        !strcmp(mode, "songplaying") || !strcmp(mode, "songmenu")) {
      app.page = SongPage;
      strcpy(app.song.name, "NIGHT TRAIN");
      app.song.bpm = 70;
      app.song.count = !strcmp(mode, "songempty") ? 0 : 6;
      app.songRow = 0;
      const char *names[] = {"ARRIVAL", "BLOOM", "STATIC FIELD", "RETURN"};
      for (int i = 0; i < 4; ++i) {
        app.saved[i] = true;
        snprintf(app.projectNames[i], 25, "%s", names[i]);
      }
      for (int i = 0; i < 6; ++i)
        app.song.slots[i] = {i % 4, i % 3 + 1, i % 2 + 1};
      if (!strcmp(mode, "songplaying")) {
        engine.shownPlaying = 1;
        engine.shownSongSlot = 2;
        engine.shownSongRepeat = 1;
        app.song.slots[2].repeats = 3;
      }
      if (!strcmp(mode, "songmenu"))
        app.options = true;
    }
    if (!strcmp(mode, "songlibrary")) {
      app.page = Atlas;
      app.atlasTab = 1;
      app.songCount = 1;
      app.songRows[0] = 0;
      strcpy(app.songNames[0], "NIGHT TRAIN");
      app.songSizes[0] = 6;
      app.songTempos[0] = 70;
    }
    if (!strcmp(mode, "sequencecopy")) {
      app.page = Atlas;
      app.sequencePicker = 3;
      app.sequenceChoice = 1;
    }
  }
  draw(canvas, app);
  FILE *f = fopen(argv[2], "wb");
  if (!f)
    return 3;
  fwrite("BM", 1, 2, f);
  little(f, 54 + 640 * 480 * 3, 4);
  little(f, 0, 4);
  little(f, 54, 4);
  little(f, 40, 4);
  little(f, 640, 4);
  little(f, 480, 4);
  little(f, 1, 2);
  little(f, 24, 2);
  little(f, 0, 4);
  little(f, 640 * 480 * 3, 4);
  for (int i = 0; i < 4; i++)
    little(f, 0, 4);
  for (int y = 479; y >= 0; y--)
    for (int x = 0; x < 640; x++)
      little(f, canvas.pixels[y * 640 + x], 3);
  fclose(f);
  return 0;
}
