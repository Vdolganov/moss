#pragma once
#include "../../../hardware lab/src/lab_ui.h"
#include "engine.h"
#include "library.h"
#include "stream_reader.h"
namespace moss {
enum Page { Atlas, Grid, Mix, Tape, Pattern, Sampler, Space };
struct App {
  Project project = makeProject(true), undoProject;
  Engine *engine = nullptr;
  lab::Buttons buttons;
  lab::HoldStats holds;
  lab::ParameterGesture repeat, tempoRepeat;
  bool selectUsed = false, tempoTail = false, parameterTail = false;
  Page page = Atlas;
  int slot = 0, track = 0, step = 0, key = 0, bank = 0, option = 0;
  int synthBank = 0;
  bool fxDetail = false, nestedUsed = false, nestedTail = false;
  lab::ParameterGesture nestedRepeat;
  int voicePicker = 0, voiceChoice = 0; // 1 instrument, 2 preset
  bool dirty = false, haveUndo = false, options = false, chord = false,
       exit = false, reload = false;
  bool gestures[4] = {}, editing = false;
  uint64_t pressed[lab::ButtonCount] = {}, navAt = 0;
  int nav = -1;
  int requestedSlot = -1;
  char status[80] = {};
  uint64_t nowUs = 0, statusUntil = 0;
  bool saved[Projects] = {}, broken[Projects] = {};
  int projectRows[Projects] = {}, projectCount = 0, projectCursor = 0;
  char projectNames[Projects][25] = {};
  int projectTracks[Projects] = {}, projectTempos[Projects] = {};
  int nameTarget = 0,
      confirmTarget = 0; // 0 sample, 1 project, 2 track, 3 new project
  int manageMenu = 0;    // 1 project, 2 track
  Library *library = nullptr;
  WavSource source;
  StreamReader streamReader;
  bool previewWhole = false;
  struct Slice {
    int start = 0, end = 1, fadeIn = 0, fadeOut = 0;
  };
  Slice slices[16];
  int sliceIndex = 0, sliceCount = 0;
  int tapeTab = 0, mixTab = 0, captureSource = 0, captureBars = 1, asset = 0,
      fileCursor = 0;
  bool importAssign = false;
  int importTrack = 0, importKey = 0;
  char saveLabel[25] = {};
  bool filePicker = false, assignMode = false, capturePending = false,
       captureFailed = false, discardPending = false;
  bool naming = false, confirmDelete = false;
  int nameCursor = 0;
  char nameEdit[25] = {};
  enum Job {
    NoJob,
    OpenSource,
    SaveSlice,
    SaveAll,
    PreviewSlice,
    PreviewAsset,
    DeleteAsset,
    PreviewSource,
    PreviewFile,
    StopPreview
  };
  Job job = NoJob;
  char pendingPath[512] = {};
  unsigned captureState() const {
    return __atomic_load_n(&engine->capture.state, __ATOMIC_ACQUIRE);
  }
  bool captureBusy() const {
    return capturePending || discardPending || captureState() != 0;
  }
  bool fxPage() const { return page == Space || (page == Mix && mixTab == 1); }
  const char *assetName(int id) const {
    if (id < 0 || id >= engine->sampleCount)
      return "UNASSIGNED";
    if (!engine->samples[id].frames)
      return "MISSING FILE";
    return library  ? library->names[id]
           : id < 3 ? lab::sampleNames[id]
                    : "SAMPLE";
  }
  bool available(int id) const {
    return id >= 0 && id < engine->sampleCount && engine->samples[id].frames;
  }
  int assignedKey(int n, int d) const {
    for (int k = n + d; k >= 0 && k < Keys; k += d)
      if (available(project.tracks[track].keys[k].asset))
        return k;
    return n;
  }
  void pickAsset() {
    if (!library)
      return;
    assignMode = true;
    page = Tape;
    tapeTab = 2;
    importAssign = false;
    asset = -1; // IMPORT precedes prepared assets.
  }
  void nameSlice(bool all = false) {
    if (!sliceCount || !library)
      return;
    if (previewing())
      job = StopPreview;
    beginName(all ? 5 : 4, saveLabel[0] ? saveLabel : "SLICE");
  }
  void openSource(const char *path) {
    if (captureBusy()) {
      say("FINISH CAPTURE FIRST");
      return;
    }
    snprintf(pendingPath, sizeof(pendingPath), "%s", path);
    job = OpenSource;
    say("OPENING SOURCE / TRANSPORT STOPS");
  }
  int durationMs() const {
    return source.rate
               ? int((uint64_t(source.frames) * 1000 + source.rate - 1) /
                     source.rate)
               : 0;
  }
  unsigned sliceFrames(const Slice &s) const {
    return sliceEnd(s) - sliceStart(s);
  }
  unsigned sliceStart(const Slice &s) const {
    return unsigned(uint64_t(s.start) * source.rate / 1000);
  }
  unsigned sliceEnd(const Slice &s) const {
    return s.end == durationMs()
               ? source.frames
               : unsigned(uint64_t(s.end) * source.rate / 1000);
  }
  void limitSlice(Slice &s, int movedEdge) {
    if (!source.rate || !source.channels)
      return;
    unsigned maximum = SampleBudget / (source.channels * 2);
    if (sliceFrames(s) > maximum) {
      if (movedEdge == 1) {
        unsigned first = sliceEnd(s) - maximum;
        s.start = int((uint64_t(first) * 1000 + source.rate - 1) / source.rate);
      } else {
        uint64_t last = uint64_t(sliceStart(s)) + maximum;
        s.end = last >= source.frames ? durationMs()
                                      : int(last * 1000 / source.rate);
      }
    }
    s.fadeIn = clamp(s.fadeIn, 0, s.end - s.start);
    s.fadeOut = clamp(s.fadeOut, 0, s.end - s.start - s.fadeIn);
  }
  bool previewing() const {
    return __atomic_load_n(&engine->stream.state, __ATOMIC_ACQUIRE) == 1;
  }
  void previewToggle(Job which) {
    if (captureBusy()) {
      say("FINISH CAPTURE FIRST");
      return;
    }
    job = previewing() ? StopPreview : which;
  }
  void pumpPreview() {
    if (__atomic_load_n(&engine->stream.state, __ATOMIC_ACQUIRE) == 0)
      streamReader.close();
    else
      streamReader.pump(engine->stream);
    if (streamReader.failed) {
      say(streamReader.source.error);
      streamReader.failed = false;
    }
  }
  bool assetUsed(int id) {
    auto uses = [&](const Project &p) {
      for (int i = 0; i < p.trackCount; i++)
        for (const auto &k : p.tracks[i].keys)
          if (k.asset == id)
            return true;
      return false;
    };
    if (uses(project) || (haveUndo && uses(undoProject)))
      return true;
    for (int i = 0; i < Projects; i++) {
      char p[128];
      path(p, sizeof(p), i);
      if (!exists(p))
        continue;
      Project q;
      if (!loadProject(p, q) || uses(q))
        return true;
    }
    if (exists("projects/deleted.moss")) {
      Project q;
      if (!loadProject("projects/deleted.moss", q) || uses(q))
        return true;
    }
    return false;
  }
  void tapeAction(int b) {
    if (!library)
      return;
    if (b == lab::Y) {
      if (captureBusy()) {
        say("CAPTURE ACTIVE / A FINISH");
        return;
      }
      if (previewing())
        job = StopPreview;
      if (importAssign || assignMode)
        return;
      tapeTab = (tapeTab + 1) % 3;
      return;
    }
    if (tapeTab == 0) {
      if (b == lab::X) {
        if (captureBusy()) {
          say("FINISH CAPTURE FIRST");
          return;
        }
        library->refresh();
        fileCursor = 0;
        filePicker = true;
        return;
      }
      if (b == lab::A) {
        if (captureState() == 2) {
          captureFailed = false;
          return;
        }
        if (captureBusy()) {
          send({Op::CaptureStop});
          return;
        }
        if (!__atomic_load_n(&engine->shownPlaying, __ATOMIC_ACQUIRE))
          send({Op::Transport});
        capturePending =
            send({Op::CaptureStart, captureSource, 0, 0, captureBars});
        if (capturePending)
          say("CAPTURING / A FINISH EARLY");
      }
    } else if (tapeTab == 1 && sliceCount) {
      if (b == lab::A)
        nameSlice();
      if (b == lab::X)
        previewToggle(PreviewSlice);
    } else if (tapeTab == 2 && asset < 0) {
      if (b == lab::A) {
        importAssign = assignMode;
        importTrack = track;
        importKey = key;
        library->refresh();
        fileCursor = 0;
        filePicker = true;
      }
    } else if (tapeTab == 2 && library->pcm[asset].frames) {
      if (b == lab::X)
        job = PreviewAsset;
      if (b == lab::A) {
        if (assignMode) {
          change({Op::Key, track, key, 0, asset});
          assignMode = false;
          page = Sampler;
          say("SAMPLE ASSIGNED TO KEY");
        } else {
          char p[128];
          library->assetPath(p, sizeof(p), asset);
          openSource(p);
        }
      }
    }
  }
  // Called on the UI thread. Completed capture is immutable; playback may
  // continue.
  void flushCapture() {
    unsigned state = captureState();
    if (discardPending) {
      if (state == 0) {
        discardPending = false;
        capturePending = false;
      }
      return;
    }
    if (state)
      capturePending = false;
    if (state != 2 || captureFailed)
      return;
    if (!engine->capture.frames) {
      __atomic_store_n(&engine->capture.state, 0, __ATOMIC_RELEASE);
      return;
    }
    char p[128];
    int i = 1;
    for (; i < 10000; i++) {
      snprintf(p, sizeof(p), "captures/take-%04d.wav", i);
      if (!exists(p))
        break;
    }
    if (i == 10000 || !writeWav(p, engine->capture.pcm, engine->capture.frames,
                                engine->capture.rate, 2)) {
      captureFailed = true;
      say("CAPTURE KEPT IN RAM / A RETRY SAVE");
      return;
    }
    __atomic_store_n(&engine->capture.state, 0, __ATOMIC_RELEASE);
    if (source.open(p)) {
      source.overview();
      sliceCount = 1;
      sliceIndex = 0;
      slices[0] = {0, durationMs(), 0, 0};
      limitSlice(slices[0], 0);
    }
    if (library)
      library->refresh();
    if (page == Tape)
      tapeTab = 1;
    say("CAPTURE SAVED / TRIM THEN A SAVE SLICE");
  }
  // Only called with the audio worker paused. Never allocate/read files in
  // render().
  void service() {
    if (job == NoJob || !library)
      return;
    Job current = job;
    job = NoJob;
    streamReader.close();
    if (current == StopPreview) {
      __atomic_store_n(&engine->stream.state, 0, __ATOMIC_RELEASE);
      engine->preview.stop();
      return;
    }
    if (captureBusy()) {
      say("FINISH CAPTURE FIRST");
      return;
    }
    engine->init(project, library->pcm, engine->rate, Assets);
    if (current == PreviewSlice || current == PreviewSource ||
        current == PreviewFile) {
      bool whole = current != PreviewSlice;
      const char *file = current == PreviewFile
                             ? library->sourcePaths[fileCursor]
                             : source.path;
      unsigned first = whole ? 0 : sliceStart(slices[sliceIndex]),
               last = whole ? 0 : sliceEnd(slices[sliceIndex]);
      int in = whole ? 0 : slices[sliceIndex].fadeIn,
          out = whole ? 0 : slices[sliceIndex].fadeOut;
      if (!streamReader.begin(engine->stream, file, engine->rate, first, last,
                              in, out))
        say(streamReader.source.error);
      else {
        previewWhole = whole;
        say(whole ? "SOURCE AUDITION / X STOP" : "SLICE AUDITION / X STOP");
      }
      return;
    }
    if (current == OpenSource) {
      WavSource candidate;
      if (!candidate.open(pendingPath)) {
        say(candidate.error);
        return;
      }
      source = candidate;
      source.overview();
      sliceCount = 1;
      sliceIndex = 0;
      slices[0] = {0, durationMs(), 0, 0};
      limitSlice(slices[0], 0);
      page = Tape;
      tapeTab = 1;
      filePicker = false;
      assignMode = false;
      say("A SAVE SLICE / X AUDITION / Y BANK");
    } else if (current == PreviewAsset) {
      engine->auditionBuffer(library->pcm[asset]);
    } else if (current == DeleteAsset) {
      if (assetUsed(asset)) {
        say("IN USE BY PROJECT OR UNDO / FILE KEPT");
        return;
      }
      if (!library->removeAsset(asset)) {
        say(library->error);
        return;
      }
      int n = library->next(asset, 1);
      if (n == asset)
        n = library->next(asset, -1);
      asset = n;
      say("UNUSED SAMPLE DELETED");
    } else if (sliceCount) {
      int first = current == SaveAll ? 0 : sliceIndex,
          last = current == SaveAll ? sliceCount - 1 : sliceIndex;
      if (current != PreviewSlice &&
          Assets - library->count() < last - first + 1) {
        say("LIBRARY FULL / NOT ENOUGH EMPTY SLOTS");
        return;
      }
      for (int i = first; i <= last; i++)
        if (!sliceFrames(slices[i]) ||
            uint64_t(sliceFrames(slices[i])) * source.channels * 2 >
                SampleBudget) {
          say("SLICE EXCEEDS 1 MIB / TRIM OR CUT FIRST");
          return;
        }
      if (current == PreviewSlice) {
        auto &s = slices[sliceIndex];
        if (!source.extract(library->scratch, sliceStart(s), sliceEnd(s),
                            s.fadeIn, s.fadeOut)) {
          say(source.error);
          return;
        }
        engine->auditionBuffer(library->scratch);
        return;
      }
      int savedCount = 0;
      for (int i = first; i <= last; i++) {
        auto &s = slices[i];
        int id = library->saveSlice(source, sliceStart(s), sliceEnd(s),
                                    s.fadeIn, s.fadeOut, saveLabel,
                                    current == SaveAll ? i + 1 : 0);
        if (id < 0) {
          char m[80];
          snprintf(m, sizeof(m), "SAVED %d / %s", savedCount, library->error);
          say(m);
          return;
        }
        asset = id;
        savedCount++;
      }
      if (importAssign) {
        track = importTrack;
        key = importKey;
        editing = false;
        change({Op::Key, track, key, 0, asset});
        importAssign = assignMode = false;
        page = Sampler;
        engine->init(project, library->pcm, engine->rate, Assets);
        say("NAMED SAMPLE SAVED AND ASSIGNED");
        return;
      }
      engine->init(project, library->pcm, engine->rate, Assets);
      char m[80];
      snprintf(m, sizeof(m), "%d SLICE SAVED / Y LIBRARY / VOICE ASSIGN",
               savedCount);
      say(m);
    }
  }

  void path(char *out, int n, int index) const {
    snprintf(out, n, "projects/project-%d.moss", index + 1);
  }
  void scan() {
    projectCount = 0;
    for (int i = 0; i < Projects; i++) {
      char p[128];
      path(p, sizeof(p), i);
      saved[i] = exists(p);
      Project temp;
      broken[i] = saved[i] && !loadProject(p, temp);
      if (saved[i]) {
        projectRows[projectCount++] = i;
        snprintf(projectNames[i], 25, "%s",
                 broken[i] ? "DAMAGED PROJECT" : temp.name);
        projectTracks[i] = broken[i] ? 0 : temp.trackCount;
        projectTempos[i] = broken[i] ? 0 : temp.bpm;
      }
    }
    projectCursor = clamp(projectCursor, 0, projectCount);
  }
  const char *name() const { return project.name; }
  void seedProjects() {
    scan();
    if (!exists("projects/initialized")) {
      if (!projectCount &&
          !saveProject("projects/project-1.moss", makeSynthDemo()))
        return;
      FILE *f = fopen("projects/initialized", "wb");
      if (f) {
        fputs("MOSS projects 1\n", f);
        fclose(f);
      }
      scan();
    }
  }
  void say(const char *s) {
    snprintf(status, sizeof(status), "%s", s);
    statusUntil = nowUs + 5000000;
  }
  bool save() {
    if (slot < 0 || slot >= Projects)
      return false;
    char p[128];
    path(p, sizeof(p), slot);
    if (!saveProject(p, project)) {
      say("SAVE FAILED / CHECK SD / TRY OPTIONS SAVE");
      return false;
    }
    dirty = false;
    saved[slot] = true;
    broken[slot] = false;
    say("PROJECT SAVED");
    return true;
  }
  bool open(int index) {
    char p[128];
    path(p, sizeof(p), index);
    Project temp;
    if (exists(p)) {
      if (!loadProject(p, temp)) {
        say("PROJECT DAMAGED / ORIGINAL FILE KEPT");
        return false;
      }
    } else {
      say("PROJECT FILE MISSING / CREATE NEW");
      scan();
      return false;
    }
    project = temp;
    slot = index;
    page = Grid;
    track = step = key = bank = 0;
    dirty = !exists(p);
    haveUndo = false;
    options = false;
    voicePicker = 0;
    engine->init(project, engine->samples, engine->rate, engine->sampleCount);
    say("START PLAY / A OPEN TRACK / L1+R1 OPTIONS");
    return true;
  }
  bool send(Command c) {
    if (!engine->commands.push(c)) {
      say("INPUT QUEUE FULL / TRY AGAIN");
      return false;
    }
    return true;
  }
  void change(Command c) {
    if (!editing) {
      undoProject = project;
      haveUndo = true;
      editing = true;
    }
    if (send(c)) {
      apply(project, c);
      dirty = true;
    }
  }
#include "app_management.h"
  void nextEffect(int direction) {
    auto &f = page == Mix ? project.fx : project.tracks[track].fx;
    int at = 0;
    while (at < 3 && f.order[at] != f.type)
      at++;
    Command c{Op::Fx, page == Mix ? Tracks : track, 0, 0,
              f.order[clamp(at + direction, 0, 3)]};
    if (send(c))
      apply(project, c); // browsing a chain must not replace undo
  }
  void nested(int d) {
    if (page == Pattern && bank == 2)
      nextEffect(d);
    else if ((page == Sampler || (page == Pattern && bank == 1)) &&
             project.tracks[track].instrument)
      synthBank = clamp(synthBank + d, 0, 1);
    else if (page == Pattern && !project.tracks[track].instrument)
      key = assignedKey(key, d);
    else if (fxPage())
      nextEffect(d);
  }
  void edit(int i, int d, int amount) {
    if (!project.trackCount && page != Tape && !fxPage())
      return;
    auto &t = project.tracks[track];
    auto &s = t.steps[step];
    auto &k = t.keys[key];
    if (page == Pattern) {
      auto &h = t.hits[key][step];
      if (bank == 0 && !t.instrument) {
        int vals[] = {h.velocity, h.fadeIn, h.fadeOut, h.probability};
        int hi[] = {127, 5000, 5000, 100};
        change({Op::Hit, track, step, key * 9 + i + 1,
                clamp(vals[i] + d * amount * (i == 1 || i == 2 ? 10 : 1), 0,
                      hi[i])});
        return;
      }
      if (bank == 0) {
        if (i == 2 && !t.instrument && t.keys[s.key].mode == 0)
          return;
        if (i == 0 && t.instrument) {
          change(
              {Op::Step, track, step, 9, clamp(s.note + d * amount, 24, 96)});
          return;
        }
        int vals[] = {s.key, s.velocity, s.length, s.probability};
        int lo[] = {0, 0, 1, 0}, hi[] = {11, 127, Steps, 100};
        if (i == 2)
          amount *= t.stride();
        int v = i == 0 ? assignedKey(s.key, d)
                       : clamp(vals[i] + d * amount, lo[i], hi[i]);
        change({Op::Step, track, step, i + 1, v});
      } else if (bank == 1) {
        if (t.instrument) {
          int type = t.instrument - 1;
          i += synthBank * 4;
          int v = s.synthLocks[type][i];
          if (v == Inherit)
            v = t.synthParams[type][i];
          change({Op::SynthLock, track, step, type * 8 + i,
                  synthEdit(t.instrument, i, v, d, amount)});
          return;
        }
        const auto &sk = t.keys[key];
        int base[] = {sk.pitch, sk.start, sk.end, sk.level};
        int v = h.locks[i] == Inherit ? base[i] : h.locks[i];
        v = clamp(v + d * amount, i == 0 ? -24 : 0, i == 0 ? 24 : 100);
        if (i == 1)
          v = clamp(v, 0, (h.locks[2] == Inherit ? sk.end : h.locks[2]) - 1);
        if (i == 2)
          v = clamp(v, (h.locks[1] == Inherit ? sk.start : h.locks[1]) + 1,
                    100);
        change({Op::Hit, track, step, key * 9 + i + 5, v});
      } else if (bank == 2) {
        int type = t.fx.type;
        int v = s.fxLocks[type][i];
        if (v == Inherit)
          v = t.fx.values[type][i];
        change({Op::FxLock, track, step, type * 4 + i,
                clamp(v + d * amount, fxLow(type, i), fxHigh(type, i))});
      } else {
        if (i == 0)
          change({Op::Track, track, 0, 0,
                  clamp(t.cycle + d * amount * t.stride(), 1, Steps)});
        if (i == 1) {
          int value = d > 0 ? t.division * 2 : t.division / 2;
          change({Op::Track, track, 0, 4, clamp(value, 8, 64)});
          step = step / t.stride() * t.stride();
        }
        step = clamp(step, 0, t.cycle - 1);
      }
    } else if (page == Sampler && t.instrument) {
      i += synthBank * 4;
      int type = t.instrument - 1;
      change({Op::SynthParam, track, type, i,
              synthEdit(t.instrument, i, t.synthParams[type][i], d, amount)});
    } else if (page == Sampler && k.asset >= 0) {
      int vals[] = {k.pitch, k.start, k.end, k.level};
      int lo[] = {-24, 0, k.start + 1, 0}, hi[] = {24, k.end - 1, 100, 100};
      change({Op::Key, track, key, i + 1,
              clamp(vals[i] + d * amount, lo[i], hi[i])});
    } else if (fxPage()) {
      if (!fxDetail) {
        if (i == 3)
          change({Op::Fx, page == Mix ? Tracks : track,
                  (page == Mix ? project.fx : t.fx).type, 6,
                  (buttons.held & (lab::bit(lab::Up) | lab::bit(lab::Down)))
                      ? -d
                      : d});
        return;
      }
      int target = page == Mix ? Tracks : track;
      auto &f = page == Mix ? project.fx : t.fx;
      change({Op::Fx, target, f.type, i + 2,
              clamp(f.values[f.type][i] + d * amount, fxLow(f.type, i),
                    fxHigh(f.type, i))});
    } else if (page == Tape && !filePicker && !naming && !confirmDelete) {
      if (captureBusy())
        return;
      if (tapeTab == 0) {
        if (i == 0)
          captureSource =
              clamp(captureSource + d * amount, 0, project.trackCount);
        if (i == 1)
          captureBars = clamp(captureBars + d * amount, 1, 8);
      }
      if (tapeTab == 1 && sliceCount) {
        auto &cut = slices[sliceIndex];
        int *vals[] = {&cut.start, &cut.end, &cut.fadeIn, &cut.fadeOut};
        int lo[] = {0, cut.start + 1, 0, 0},
            hi[] = {cut.end - 1, durationMs(),
                    cut.end - cut.start - cut.fadeOut,
                    cut.end - cut.start - cut.fadeIn};
        int delta = amount;
        if (i < 2) {
          if (buttons.held & (lab::bit(lab::Up) | lab::bit(lab::Down)))
            delta = 100 * amount;
          if (repeat.previous && nowUs - repeat.since > 1600000)
            delta = 100;
          if (repeat.previous && nowUs - repeat.since > 3500000)
            delta = 1000;
        }
        *vals[i] = clamp(*vals[i] + d * delta, lo[i], hi[i]);
        if (i < 2)
          limitSlice(cut, i);
        if (previewing())
          job = StopPreview;
        cut.fadeIn = clamp(cut.fadeIn, 0, cut.end - cut.start);
        cut.fadeOut = clamp(cut.fadeOut, 0, cut.end - cut.start - cut.fadeIn);
      }
    } else if (page == Mix) {
      int vals[] = {t.level, t.pan, project.master, t.mute};
      int lo[] = {0, -100, 0, 0}, hi[] = {100, 100, 100, 1};
      int v = clamp(vals[i] + d * amount, lo[i], hi[i]);
      change(i < 2    ? Command{Op::Track, track, 0, i + 1, v}
             : i == 2 ? Command{Op::Global, 0, 0, 1, v}
                      : Command{Op::Track, track, 0, 3, v});
    }
  }
  int synthEdit(int kind, int i, int v, int d, int amount) const {
    int delta = amount;
    if (synthMax(kind, i) > 100) {
      delta *= 10;
      if (buttons.held & (lab::bit(lab::Up) | lab::bit(lab::Down)))
        delta *= 10;
    }
    return clamp(v + d * delta, synthMin(kind, i), synthMax(kind, i));
  }
  int optionCount() const {
    if (importAssign)
      return 3;
    if (assignMode)
      return 2;
    if (manageMenu)
      return 3;
    if (page == Atlas)
      return 7;
    if (page == Grid)
      return 8;
    if (fxPage())
      return 4;
    if (page == Tape)
      return tapeTab == 0 ? 6 : tapeTab == 1 ? 8 : 6;
    return page == Sampler ? 7 : page == Pattern ? 7 : 5;
  }
  const char *optionName(int i) const {
    if (importAssign) {
      const char *n[] = {"AUDITION FULL SOURCE", "NAME + ASSIGN SLICE",
                         "CANCEL IMPORT"};
      return n[i];
    }
    if (assignMode)
      return i == 0 ? "IMPORT FROM DEVICE" : "CANCEL ASSIGNMENT";
    if (manageMenu) {
      const char *n[] = {"RENAME", "DUPLICATE", "DELETE"};
      return n[i];
    }
    if (page == Atlas) {
      const char *n[] = {"NEW PROJECT",    "DEMO / SLOW SIGNAL",
                         "RENAME PROJECT", "DUPLICATE PROJECT",
                         "DELETE PROJECT", "RESTORE DELETED PROJECT",
                         "EXIT MOSS"};
      return n[i];
    }
    if (page == Grid && i >= 3) {
      const char *n[] = {"ADD TRACK", "TRACK OPTIONS", "PROJECT OPTIONS",
                         "BACK TO PROJECTS", "EXIT MOSS"};
      return n[i - 3];
    }
    if (i == 0)
      return "SAVE PROJECT";
    if (i == 1)
      return "UNDO LAST EDIT";
    if (i == 2)
      return page == Tape && tapeTab == 1 ? "AUDITION FULL SOURCE"
                                          : "RESTART TRANSPORT";
    if (fxPage())
      return "ENABLE / BYPASS SELECTED";
    if (page == Tape) {
      const char *n[3][6] = {
          {"IMPORT WAV", "FROM LIBRARY", "DISCARD CAPTURE"},
          {"SAVE SELECTED SLICE", "SAVE ALL SLICES", "CUT AT MIDPOINT",
           "ADD NEXT RANGE", "REMOVE SELECTED SLICE", "AUDITION FULL SOURCE"},
          {"EDIT COPY", "RENAME SAMPLE", "DELETE UNUSED SAMPLE"}};
      return n[tapeTab][i - 3];
    }
    if (page == Sampler) {
      if (i == 3)
        return "CHOOSE INSTRUMENT";
      if (project.tracks[track].instrument) {
        const char *n[] = {"CHOOSE PRESET", "CHANGE MODE", "EXIT MOSS"};
        return n[i - 4];
      }
      const char *n[] = {"ASSIGN FROM LIBRARY", "REMOVE ASSIGNMENT",
                         "ONE SHOT PLAYBACK"};
      return n[i - 4];
    }
    if (page == Pattern) {
      const char *n[] = {"NOTE TO CYCLE END", "CLEAR CURRENT LOCK BANK",
                         "CYCLE 8 / 16 / 32 / 64", "CLEAR SELECTED STEP"};
      return n[i - 3];
    }
    return i == 3 ? "BACK TO PROJECTS" : "EXIT MOSS";
  }
  void chooseOption() {
    int i = option;
    options = false;
    if (importAssign) {
      if (i == 0 && sliceCount)
        previewToggle(PreviewSource);
      if (i == 1)
        nameSlice();
      if (i == 2)
        action(lab::B);
      return;
    }
    if (assignMode) {
      if (i == 0) {
        asset = -1;
        tapeAction(lab::A);
      } else
        action(lab::B);
      return;
    }
    editing = false;
    if (manageMenu) {
      int target = manageMenu;
      manageMenu = 0;
      if (!structureAllowed())
        return;
      if (i == 0)
        beginName(target,
                  target == 1 ? project.name : project.tracks[track].name);
      if (i == 1) {
        if (target == 1)
          duplicateProject();
        else
          duplicateTrack();
      }
      if (i == 2 && structureAllowed()) {
        confirmDelete = true;
        confirmTarget = target;
      }
      return;
    }
    if (page == Atlas) {
      if (i == 0)
        beginName(3, "UNTITLED");
      if (i == 1)
        createProject("SLOW SIGNAL", true);
      if (i >= 2 && i <= 4 && selectedProject()) {
        if (i == 2)
          beginName(1, project.name);
        if (i == 3)
          duplicateProject();
        if (i == 4) {
          confirmDelete = true;
          confirmTarget = 1;
        }
      }
      if (i == 5)
        restoreProject();
      if (i == 6)
        requestExit();
      return;
    }
    if (i == 0) {
      save();
      return;
    }
    if (i == 1) {
      if (captureBusy()) {
        say("FINISH CAPTURE FIRST");
        return;
      }
      if (haveUndo) {
        Project temp = project;
        project = undoProject;
        undoProject = temp;
        dirty = true;
        reload = true;
        normalizeSelection();
        say("UNDO / TRANSPORT STOPPED");
      } else
        say("NOTHING TO UNDO");
      return;
    }
    if (i == 2) {
      if (page == Tape && tapeTab == 1) {
        if (sliceCount)
          previewToggle(PreviewSource);
        else
          say("IMPORT A SOURCE FIRST");
        return;
      }
      send({Op::Rewind});
      return;
    }
    if (page == Grid) {
      if (i == 3 && structureAllowed()) {
        if (project.trackCount == Tracks)
          say("8 TRACKS MAXIMUM");
        else {
          voicePicker = 3;
          voiceChoice = OrganismInstrument;
        }
      }
      if (i == 4) {
        if (track < project.trackCount) {
          manageMenu = 2;
          options = true;
          option = 0;
        } else
          say("SELECT A TRACK FIRST");
      }
      if (i == 5) {
        manageMenu = 1;
        options = true;
        option = 0;
      }
      if (i == 6)
        backToAtlas();
      if (i == 7)
        requestExit();
      return;
    }
    if (fxPage()) {
      auto &f = page == Mix ? project.fx : project.tracks[track].fx;
      change({Op::Fx, page == Mix ? Tracks : track, 0, 1, !f.enabled});
    } else if (page == Tape) {
      if (captureBusy()) {
        if (tapeTab == 0 && i == 5) {
          discardPending = send({Op::CaptureDiscard});
          capturePending = false;
          captureFailed = false;
          say("CAPTURE DISCARDED");
        } else
          say("FINISH CAPTURE FIRST");
        return;
      }
      if (tapeTab == 0) {
        if (i == 3)
          tapeAction(lab::X);
        if (i == 4) {
          nameTarget = 0;
          tapeTab = 2;
          assignMode = false;
        }
        return;
      }
      if (tapeTab == 1 && sliceCount) {
        auto cut = slices[sliceIndex];
        if (i == 3)
          nameSlice();
        if (i == 4)
          nameSlice(true);
        if (i == 5 || i == 6) {
          if (sliceCount == 16) {
            say("16 SLICES MAXIMUM");
            return;
          }
          int at = i == 5 ? (cut.start + cut.end) / 2 : cut.end;
          if ((i == 5 && cut.end - cut.start < 2) ||
              (i == 6 && at >= durationMs())) {
            say("NO SOURCE LEFT FOR ANOTHER SLICE");
            return;
          }
          for (int j = sliceCount; j > sliceIndex + 1; j--)
            slices[j] = slices[j - 1];
          slices[sliceIndex + 1] = {at, i == 5 ? cut.end : durationMs(), 0, 0};
          limitSlice(slices[sliceIndex + 1], 0);
          if (i == 5) {
            slices[sliceIndex].end = at;
            slices[sliceIndex].fadeIn = slices[sliceIndex].fadeOut = 0;
          }
          sliceCount++;
          sliceIndex++;
        }
        if (i == 7) {
          if (sliceCount == 1) {
            say("KEEP ONE SLICE / EDIT ITS BOUNDS");
            return;
          }
          for (int j = sliceIndex; j < sliceCount - 1; j++)
            slices[j] = slices[j + 1];
          sliceCount--;
          sliceIndex = clamp(sliceIndex, 0, sliceCount - 1);
        }
      } else if (tapeTab == 2 && library && asset >= 0 &&
                 library->pcm[asset].frames) {
        if (i == 3) {
          char p[128];
          library->assetPath(p, sizeof(p), asset);
          openSource(p);
        }
        if (i == 4) {
          memset(nameEdit, ' ', 24);
          nameEdit[24] = 0;
          size_t n = strlen(library->names[asset]);
          if (n > 24)
            n = 24;
          memcpy(nameEdit, library->names[asset], n);
          nameCursor = 0;
          nameTarget = 0;
          naming = true;
        }
        if (i == 5) {
          confirmTarget = 0;
          if (assetUsed(asset))
            say("IN USE BY PROJECT OR UNDO / FILE KEPT");
          else
            confirmDelete = true;
        }
      }
    } else if (page == Sampler) {
      auto &t = project.tracks[track];
      if (i == 3) {
        voicePicker = 1;
        voiceChoice = t.instrument;
        return;
      }
      if (t.instrument) {
        if (i == 4) {
          voicePicker = 2;
          voiceChoice = t.synthPreset[t.instrument - 1] < 0
                            ? 0
                            : t.synthPreset[t.instrument - 1];
        }
        if (i == 5)
          change({Op::SynthMode, track, t.instrument - 1, 0,
                  1 - t.synthMode[t.instrument - 1]});
        if (i == 6)
          requestExit();
        return;
      }
      if (i == 4)
        pickAsset();
      if (i == 5)
        change({Op::Key, track, key, 0, -1});
      if (i == 6)
        say("ONE SHOT / SAMPLE PLAYS TO ITS END");
    } else if (page == Pattern) {
      auto &t = project.tracks[track];
      if (i == 3 && t.instrument) {
        change({Op::Step, track, step, 0, 1});
        change({Op::Step, track, step, 3, t.cycle - step});
      }
      if (i == 4)
        for (int j = 0; j < 4; j++)
          change(
              bank == 2
                  ? Command{Op::FxLock, track, step, t.fx.type * 4 + j, Inherit}
              : t.instrument
                  ? Command{Op::SynthLock, track, step,
                            (t.instrument - 1) * 8 + synthBank * 4 + j, Inherit}
                  : Command{Op::Hit, track, step, key * 9 + j + 5, Inherit});
      if (i == 5) {
        int cells = t.cells();
        int count = cells < 8    ? 8
                    : cells < 16 ? 16
                    : cells < 32 ? 32
                    : cells < 64 ? 64
                                 : 8;
        change({Op::Track, track, 0, 0, count * t.stride()});
        step = clamp(step, 0, t.cycle - 1);
      }
      if (i == 6)
        change(t.instrument ? Command{Op::Step, track, step, 0, 0}
                            : Command{Op::Hit, track, step, key * 9, 0});
    } else if (i == 4)
      requestExit();
    else
      backToAtlas();
    editing = false;
  }
  void backToAtlas() {
    if (captureBusy()) {
      say("FINISH CAPTURE FIRST");
      return;
    }
    if (dirty && !save())
      return;
    page = Atlas;
    options = false;
    sendStop();
    scan();
  }
  void sendStop() { send({Op::Stop}); }
  void requestExit() {
    if (captureBusy()) {
      say("FINISH CAPTURE BEFORE EXIT");
      return;
    }
    if (!dirty || save())
      exit = true;
  }
  void action(int b) {
    if (voicePicker) {
      if (b == lab::B || b == lab::Select)
        voicePicker = 0;
      if (b == lab::A) {
        editing = false;
        if (voicePicker == 3) {
          voicePicker = 0;
          addTrack(voiceChoice);
          return;
        }
        change(voicePicker == 1
                   ? Command{Op::Instrument, track, 0, 0, voiceChoice}
                   : Command{Op::SynthPreset, track,
                             project.tracks[track].instrument - 1, 0,
                             voiceChoice});
        voicePicker = 0;
        editing = false;
      }
      return;
    }
    if (naming) {
      if (b == lab::B || b == lab::Select) {
        naming = false;
        return;
      }
      if (b == lab::A) {
        int n = 24;
        while (n > 0 && nameEdit[n - 1] == ' ')
          n--;
        nameEdit[n] = 0;
        if (!n) {
          say("NAME CANNOT BE EMPTY");
          return;
        }
        if (nameTarget == 4 || nameTarget == 5) {
          snprintf(saveLabel, sizeof(saveLabel), "%s", nameEdit);
          naming = false;
          job = nameTarget == 5 ? SaveAll : SaveSlice;
          return;
        }
        if (nameTarget) {
          if (nameTarget == 3) {
            naming = false;
            createProject(nameEdit);
            return;
          }
          if (nameTarget == 1) {
            char before[25];
            memcpy(before, project.name, 25);
            bool wasDirty = dirty;
            snprintf(project.name, 25, "%s", nameEdit);
            dirty = true;
            if (!save()) {
              memcpy(project.name, before, 25);
              dirty = wasDirty;
              return;
            }
            scan();
          } else {
            checkpoint();
            snprintf(project.tracks[track].name, 25, "%s", nameEdit);
          }
          naming = false;
          say("NAME SAVED");
          return;
        }
        char old[32];
        memcpy(old, library->names[asset], 32);
        snprintf(library->names[asset], 32, "%s", nameEdit);
        if (!library->saveName(asset)) {
          memcpy(library->names[asset], old, 32);
          say("NAME SAVE FAILED");
          return;
        }
        naming = false;
        say("SAMPLE RENAMED");
      }
      return;
    }
    if (confirmDelete) {
      if (b == lab::B || b == lab::Select)
        confirmDelete = false;
      if (b == lab::A) {
        confirmDelete = false;
        if (confirmTarget == 1)
          removeProject();
        else if (confirmTarget == 2)
          removeTrack();
        else
          job = DeleteAsset;
      }
      return;
    }
    if (filePicker) {
      if (b == lab::B || b == lab::Select) {
        filePicker = false;
        if (previewing())
          job = StopPreview;
      }
      if (b == lab::A && library && library->sourceCount)
        openSource(library->sourcePaths[fileCursor]);
      if (b == lab::X && library && library->sourceCount)
        previewToggle(PreviewFile);
      return;
    }

    if (options) {
      if (b == lab::A)
        chooseOption();
      if (b == lab::B || b == lab::Select) {
        options = false;
        manageMenu = 0;
      }
      return;
    }
    if (b == lab::Select) {
      if (importAssign || assignMode) {
        action(lab::B);
        return;
      }
      if (page != Atlas)
        backToAtlas();
      return;
    }
    if (b == lab::B) {
      if (fxPage() && fxDetail) {
        fxDetail = false;
        return;
      }
      if (previewing())
        job = StopPreview;
      if (page == Tape && (assignMode || importAssign)) {
        if (importAssign) {
          track = importTrack;
          key = importKey;
        }
        assignMode = importAssign = false;
        page = Sampler;
        return;
      }
      if (page == Pattern || page == Sampler || page == Space || page == Mix ||
          page == Tape)
        page = Grid;
      else if (page != Atlas)
        backToAtlas();
      return;
    }
    if (b == lab::Start) {
      if (page != Atlas)
        send({Op::Transport});
      return;
    }
    if (b == lab::Y) {
      if (page == Pattern)
        bank = (bank + 1) % 4;
      if (page == Sampler && project.tracks[track].instrument)
        synthBank = 1 - synthBank;
      if (page == Mix)
        mixTab = (mixTab + 1) % 2;
      if (page == Tape)
        tapeAction(b);
      return;
    }
    if (page == Tape && (b == lab::A || b == lab::X)) {
      tapeAction(b);
      return;
    }
    if (fxPage() && (b == lab::A || b == lab::X)) {
      if (b == lab::A)
        fxDetail = true;
      else {
        auto &f = page == Mix ? project.fx : project.tracks[track].fx;
        change({Op::Fx, page == Mix ? Tracks : track, 0, 1, !f.enabled});
      }
      return;
    }
    if (b == lab::A) {
      editing = false;
      if (page == Atlas) {
        if (projectCursor == projectCount)
          beginName(3, "UNTITLED");
        else
          requestedSlot = projectRows[projectCursor];
      }
      if (page == Grid) {
        if (track >= project.trackCount) {
          if (project.trackCount == Tracks) {
            say("8 TRACKS MAXIMUM");
            return;
          }
          voicePicker = 3;
          voiceChoice = OrganismInstrument;
          return;
        }
        page = Pattern;
        step = 0;
        bank = 0;
      } else if (page == Pattern) {
        auto &t = project.tracks[track];
        if (!t.instrument && !available(t.keys[key].asset)) {
          say("ASSIGN A SAMPLE IN VOICE FIRST");
          return;
        }
        change(t.instrument
                   ? Command{Op::Step, track, step, 0, !t.steps[step].on}
                   : Command{Op::Hit, track, step, key * 9,
                             !t.hits[key][step].on});
      } else if (page == Sampler) {
        if (project.tracks[track].instrument)
          send({Op::Audition, track, key});
        else if (project.tracks[track].keys[key].asset < 0) {
          pickAsset();
        } else
          send({Op::Audition, track, key});
      } else if (page == Mix && project.trackCount)
        change({Op::Track, track, 0, 3, !project.tracks[track].mute});
      editing = false;
    }
  }
  void object(int direction) {
    if (voicePicker)
      return;
    if (naming || confirmDelete)
      return;
    if (filePicker) {
      if (previewing())
        job = StopPreview;
      fileCursor =
          clamp(fileCursor + direction, 0,
                library && library->sourceCount ? library->sourceCount - 1 : 0);
      return;
    }
    if (page == Tape) {
      if (tapeTab == 1) {
        if (previewing())
          job = StopPreview;
        sliceIndex =
            clamp(sliceIndex + direction, 0, sliceCount ? sliceCount - 1 : 0);
      }
      if (tapeTab == 2 && library) {
        int next = library->next(asset, direction);
        if (direction < 0 && next == asset)
          next = -1;
        asset = next;
      }
      return;
    }

    if (page == Atlas)
      projectCursor = clamp(projectCursor + direction, 0, projectCount);
    else {
      track = clamp(track + direction, 0,
                    page == Grid         ? project.trackCount
                    : project.trackCount ? project.trackCount - 1
                                         : 0);
      if (track >= Tracks)
        track = Tracks - 1;
      step = clamp(step, 0, project.tracks[track].cycle - 1);
    }
  }
  void navigate(int b) {
    int d = (b == lab::Right || b == lab::Down) ? 1 : -1;
    if (voicePicker) {
      voiceChoice = clamp(voiceChoice + d, 0, 3);
      return;
    }
    if (naming) {
      const char *chars = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.";
      if (b == lab::Left || b == lab::Right)
        nameCursor = clamp(nameCursor + d, 0, 23);
      else {
        const char *p = strchr(chars, nameEdit[nameCursor]);
        int at = p ? int(p - chars) : 0;
        int n = int(strlen(chars));
        nameEdit[nameCursor] = chars[(at + (b == lab::Up ? 1 : n - 1)) % n];
      }
      return;
    }
    if (confirmDelete)
      return;
    if (!options && (filePicker || page == Tape)) {
      object(d);
      return;
    }
    if (options) {
      option = clamp(option + d, 0, optionCount() - 1);
      return;
    }
    if (page == Atlas)
      projectCursor = clamp(projectCursor + d, 0, projectCount);
    else if (fxPage()) {
      if (!fxDetail)
        nextEffect(d);
    } else if (page == Grid || page == Mix)
      object(d);
    else if (page == Pattern)
      step = clamp(step + (b == lab::Down ? 8
                           : b == lab::Up ? -8
                                          : d) *
                              project.tracks[track].stride(),
                   0,
                   (project.tracks[track].cells() - 1) *
                       project.tracks[track].stride());
    else if (page == Sampler && project.tracks[track].instrument) {
      change({Op::AuditionNote, track, 0, 0,
              clamp(project.tracks[track].audition + (b == lab::Up     ? 12
                                                      : b == lab::Down ? -12
                                                                       : d),
                    24, 96)});
    } else if (page == Sampler)
      key = clamp(key + (b == lab::Down ? 6
                         : b == lab::Up ? -6
                                        : d),
                  0, Keys - 1);
  }
  void tick(uint64_t now) {
    nowUs = now;
    holds.update(buttons.held, now);
    if ((buttons.held & lab::bit(lab::Menu)) &&
        lab::deadlineReached(now, pressed[lab::Menu], 2000000)) {
      if (captureBusy()) {
        say("FINISH CAPTURE BEFORE EXIT");
        return;
      }
      if (page == Atlas || !dirty || save())
        exit = true;
      else
        buttons.held &= ~lab::bit(lab::Menu);
    }
    if (tempoTail && !(buttons.held & lab::bit(lab::Select))) {
      repeat = lab::ParameterGesture{};
      tempoRepeat = lab::ParameterGesture{};
      nav = -1;
      if (!(buttons.held & 15))
        tempoTail = false;
      return;
    }
    if (page != Atlas && !options && !voicePicker && !naming &&
        !confirmDelete && !filePicker &&
        (buttons.held & lab::bit(lab::Select))) {
      nav = -1;
      repeat = lab::ParameterGesture{};
      for (int i = 0; i < 4; i++)
        if (buttons.held & lab::bit(lab::A + i))
          gestures[i] = true;
      if (buttons.held & 15)
        selectUsed = tempoTail = true;
      if (tempoRepeat.tick((buttons.held & 15) | lab::bit(lab::A), now))
        change({Op::Global, 0, 0, 0,
                clamp(project.bpm + tempoRepeat.direction * tempoRepeat.step,
                      30, 240)});
      return;
    }
    tempoRepeat = lab::ParameterGesture{};
    if (nestedTail && !(buttons.held & lab::bit(lab::L1))) {
      nav = -1;
      nestedRepeat = lab::ParameterGesture{};
      if (!(buttons.held & 15))
        nestedTail = false;
      return;
    }
    if ((buttons.held & lab::bit(lab::L1)) &&
        !(buttons.held & (lab::bit(lab::R1) | 0xf0)) && !options &&
        !voicePicker && !filePicker && !naming && !confirmDelete &&
        page != Atlas) {
      nav = -1;
      repeat = lab::ParameterGesture{};
      if (buttons.held & 15)
        nestedUsed = nestedTail = true;
      if (nestedRepeat.tick((buttons.held & 15) | lab::bit(lab::A), now))
        nested(nestedRepeat.direction);
      return;
    }
    nestedRepeat = lab::ParameterGesture{};
    bool faces = (buttons.held & 0xf0) != 0;
    if (parameterTail && !faces) {
      repeat = lab::ParameterGesture{};
      nav = -1;
      if (!(buttons.held & 15))
        parameterTail = false;
      return;
    }
    if (faces && (buttons.held & 15)) {
      parameterTail = true;
      for (int i = 0; i < 4; i++)
        if (buttons.held & lab::bit(lab::A + i))
          gestures[i] = true;
    }
    if (!options && !voicePicker && !filePicker && !naming && !confirmDelete &&
        page != Atlas && repeat.tick(buttons.held, now))
      for (int i = 0; i < 4; i++)
        if (repeat.faces & (1u << i))
          edit(i, repeat.direction, repeat.step);
    if (!faces) {
      int direction = -1;
      for (int i = 0; i < 4; i++)
        if (buttons.held & lab::bit(i)) {
          direction = i;
          break;
        }
      if (direction < 0)
        nav = -1;
      else if (nav != direction) {
        nav = direction;
        navigate(nav);
        navAt = now + 350000;
      } else if (now >= navAt) {
        navigate(nav);
        navAt = now + 110000;
      }
    } else
      nav = -1;
  }
  void button(int b, bool down, uint64_t now) {
    if (b < 0 || b >= lab::ButtonCount ||
        bool(buttons.held & lab::bit(b)) == down)
      return;
    buttons.event(b, down);
    if (down) {
      pressed[b] = now;
      if (b == lab::Select) {
        selectUsed = false;
        tempoTail = false;
        editing = false;
      }
      if ((b == lab::L1 || b == lab::R1) &&
          (buttons.held & lab::bit(lab::Select)))
        chord = true;
      if (b >= lab::A && b <= lab::Y)
        gestures[b - lab::A] = false;
      if ((buttons.held & (lab::bit(lab::L1) | lab::bit(lab::R1))) ==
          (lab::bit(lab::L1) | lab::bit(lab::R1))) {
        chord = true;
        if (!(buttons.held & (0xf0 | lab::bit(lab::Select))) && !voicePicker &&
            !filePicker && !naming && !confirmDelete) {
          options = !options;
          manageMenu = 0;
          option = 0;
        }
      }
      if ((b == lab::L2 || b == lab::R2) &&
          !(buttons.held & (0xf0 | lab::bit(lab::Select))) && !options &&
          page != Atlas && !voicePicker && !filePicker && !naming &&
          !confirmDelete && !importAssign && !assignMode) {
        if (previewing())
          job = StopPreview;
        fxDetail = false;
        int dir = b == lab::R2 ? 1 : 2;
        if (page >= Pattern)
          page = Page(Pattern + (page - Pattern + dir) % 3);
        else
          page = Page(Grid + (page - Grid + dir) % 3);
      }
      if (b == lab::Start)
        action(b);
    }
    tick(now);
    if (!down) {
      if (b == lab::L1 || b == lab::R1) {
        if (!chord && !nestedUsed &&
            !(buttons.held & (0xf0 | lab::bit(lab::Select))) && !options &&
            !tempoTail)
          object(b == lab::R1 ? 1 : -1);
        if (!(buttons.held & (lab::bit(lab::L1) | lab::bit(lab::R1))))
          chord = false;
        if (b == lab::L1)
          nestedUsed = false;
      }
      if (b >= lab::A && b <= lab::Y && !gestures[b - lab::A] &&
          now - pressed[b] < 500000)
        action(b);
      if (b == lab::Select && !selectUsed && now - pressed[b] < 500000)
        action(b);
      if (!(buttons.held & (0xf0 | lab::bit(lab::Select))))
        editing = false;
    }
  }
  void clearInput() {
    buttons.clearHeld();
    repeat = lab::ParameterGesture{};
    tempoRepeat = lab::ParameterGesture{};
    selectUsed = false;
    tempoTail = false;
    parameterTail = false;
    nestedUsed = nestedTail = false;
    nestedRepeat = lab::ParameterGesture{};
    nav = -1;
    chord = false;
    editing = false;
    memset(gestures, 0, sizeof(gestures));
  }
};
inline const char *note(int n) {
  static const char *names[] = {"C3",  "C#3", "D3",  "D#3", "E3",  "F3",
                                "F#3", "G3",  "G#3", "A3",  "A#3", "B3"};
  return names[clamp(n, 0, 11)];
}
inline const char *midiNote(int n) {
  static char names[4][8];
  static unsigned slot = 0;
  const char *pitch[] = {"C",  "C#", "D",  "D#", "E",  "F",
                         "F#", "G",  "G#", "A",  "A#", "B"};
  char *out = names[slot++ % 4];
  snprintf(out, 8, "%s%d", pitch[n % 12], n / 12 - 1);
  return out;
}
#include "app_synth_ui.h"
#include "app_tools_ui.h"
inline void draw(lab::Canvas &c, const App &a) {
  using namespace lab;
  char b[128];
  c.clear(BG);
  bool playing =
      __atomic_load_n(&a.engine->shownPlaying, __ATOMIC_ACQUIRE) != 0;

  if (a.page == Atlas) {
    text(c, 18, 18, "MOSS / PROJECTS", TEXT, 2);
    text(c, 460, 18, "0.5.0", DIM);
    c.rect(0, 48, 640, 1, LINE);
    int first = a.projectCursor / 5 * 5;
    for (int row = first; row <= a.projectCount && row < first + 5; row++) {
      int y = 85 + (row - first) * 65;
      if (row == a.projectCursor)
        c.box(18, y - 10, 604, 57, TEXT);
      if (row == a.projectCount) {
        text(c, 30, y, "+ NEW PROJECT", TEXT);
        text(c, 30, y + 25, "NAME / CREATE EMPTY PROJECT", DIM);
      } else {
        int id = a.projectRows[row];
        text(c, 30, y, a.projectNames[id], TEXT, 2, 38);
        if (a.broken[id])
          strcpy(b, "DAMAGED / FILE PRESERVED");
        else
          snprintf(b, sizeof(b), "%d TRACKS / %d BPM", a.projectTracks[id],
                   a.projectTempos[id]);
        text(c, 30, y + 25, b, DIM);
      }
    }
    snprintf(b, sizeof(b), "%d / %d PROJECTS / L1+R1 OPTIONS", a.projectCount,
             Projects);
    text(c, 18, 416, b, DIM);
  } else {
    snprintf(b, sizeof(b), "%s  %s%s",
             a.captureBusy() ? "REC"
             : playing       ? ">"
                             : "-",
             a.name(), a.dirty ? " +" : "");
    text(c, 18, 18, b, TEXT, 2, 30);
    bool tempoFocus = !a.options && (a.buttons.held & bit(Select));
    if (tempoFocus)
      c.box(468, 9, 154, 30, TEXT);
    snprintf(b, sizeof(b), "SEL %d BPM", a.project.bpm);
    text(c, 478, 18, b, tempoFocus ? TEXT : DIM);
    c.rect(0, 48, 640, 1, LINE);
    const char *tabs[3];
    int active = 0;
    if (a.page >= Pattern) {
      tabs[0] = "PATTERN";
      tabs[1] = "VOICE";
      tabs[2] = "SPACE";
      active = a.page - Pattern;
    } else {
      tabs[0] = "GRID";
      tabs[1] = "MIX";
      tabs[2] = "TAPE";
      active = a.page - Grid;
    }
    if (!a.assignMode && !a.importAssign) {
      text(c, 18, 65, "[", DIM);
      text(c, 610, 65, "]", DIM);
    }
    for (int i = 0; i < 3; i++) {
      text(c, 60 + i * 180, 65, tabs[i], i == active ? TEXT : DIM);
      if (i == active)
        c.rect(56 + i * 180, 88, 140, 2, TEXT);
    }
    c.rect(0, 91, 640, 1, LINE);
    if (a.page == Grid) {
      text(c, 30, 111, "TRACK", DIM);
      text(c, 278, 111, "VOICE", DIM);
      text(c, 482, 111, "SPACE", DIM);
      int first = a.track / 5 * 5;
      for (int t = first; t <= a.project.trackCount && t < first + 5; t++) {
        int y = 141 + (t - first) * 53;
        if (t == a.track)
          c.rect(18, y - 10, 604, 49, PANEL);
        if (t == a.project.trackCount) {
          text(c, 30, y + 5, t == Tracks ? "8 TRACKS / FULL" : "+ ADD TRACK",
               t == Tracks ? DIM : TEXT);
          continue;
        }
        snprintf(b, sizeof(b), "%02d  %.13s", t + 1, a.project.tracks[t].name);
        text(c, 30, y + 5, b, TEXT, 2, 17);
        text(c, 278, y + 5, instrumentName(a.project.tracks[t].instrument),
             DIM);
        int effects = 0;
        for (int id = 0; id < 4; id++)
          effects += a.project.tracks[t].fx.active(id);
        snprintf(b, sizeof(b), "%d / 4 FX", effects);
        text(c, 482, y + 5, b, DIM);
        c.rect(18, y + 39, 604, 1, t == a.track ? TEXT : LINE);
      }
      snprintf(b, sizeof(b), "%d / 8 TRACKS / A OPEN / L1+R1 OPTIONS",
               a.project.trackCount);
      text(c, 18, 417, b, DIM);
    }
    if (a.page == Mix && a.mixTab == 0) {
      text(c, 18, 102, "Y  LEVELS / MASTER FX", DIM);
      for (int t = a.track / 3 * 3;
           t < a.project.trackCount && t < (a.track / 3 + 1) * 3; t++) {
        const auto &tr = a.project.tracks[t];
        int y = 134 + t % 3 * 66;
        if (t == a.track)
          c.box(18, y - 10, 604, 61, TEXT);
        snprintf(b, sizeof(b), "%02d  %.18s", t + 1, tr.name);
        text(c, 30, y, b, TEXT, 2, 23);
        snprintf(b, sizeof(b), "%03d STEPS / %s", tr.cells(),
                 tr.mute ? "MUTED" : instrumentName(tr.instrument));
        text(c, 30, y + 26, b, DIM);
        unsigned peak =
            __atomic_load_n(&a.engine->shownPeak[t], __ATOMIC_ACQUIRE);
        c.rect(360, y + 10, 240, 4, LINE);
        c.rect(360, y + 10, clamp(int(peak) * 240 / 16384, 0, 240), 4, TEXT);
        snprintf(b, sizeof(b), "LEVEL %d / PAN %+d", tr.level, tr.pan);
        text(c, 360, y + 26, b, DIM, 2, 20);
      }
      if (!a.project.trackCount)
        text(c, 18, 190, "NO TRACKS / B GRID / A ADD TRACK", DIM);
    }
    if (a.page == Pattern) {
      const auto &tr = a.project.tracks[a.track];
      const auto &selected = tr.steps[a.step];
      int stride = tr.stride(), selectedCell = a.step / stride;
      int first = selectedCell / 16 * 16;
      if (tr.instrument)
        snprintf(b, sizeof(b), "%02d %.18s / 1:%d", a.track + 1, tr.name,
                 tr.division);
      else
        snprintf(b, sizeof(b), "%s / %.20s / 1:%d", note(a.key),
                 a.assetName(tr.keys[a.key].asset), tr.division);
      text(c, 18, 108, b, DIM);
      snprintf(b, sizeof(b), "%02d-%02d", first + 1,
               clamp(first + 16, 0, tr.cells()));
      text(c, 524, 108, b, DIM);
      for (int i = 0; i < 16; i++) {
        int cell = first + i, s = cell * stride, x = 18 + i % 8 * 76,
            y = 132 + i / 8 * 65;
        bool enabled = s < tr.cycle;
        const auto &st = tr.steps[s < Steps ? s : 0];
        const auto &hit = tr.hits[a.key][s < Steps ? s : 0];
        bool on = enabled && (tr.instrument ? st.on : hit.on);
        bool between = false;
        for (int off = 1; off < stride && s + off < tr.cycle; off++)
          between |= tr.instrument ? tr.steps[s + off].on != 0
                                   : tr.hits[a.key][s + off].on != 0;
        uint32_t ink = on ? BG : DIM;
        c.rect(x, y, 69, 53, on ? 0xd5d6d3 : BG);
        c.box(x, y, 69, 53, enabled ? (on ? 0xd5d6d3 : 0x56585e) : LINE);
        snprintf(b, sizeof(b), "%02d", cell + 1);
        text(c, x + 7, y + 7, b, enabled ? ink : LINE);
        if (enabled) {
          text(c, x + 7, y + 29,
               on ? (tr.instrument ? midiNote(st.note) : note(a.key)) : "--",
               ink);
          bool locked = false;
          if (tr.instrument) {
            for (int n : st.synthLocks[tr.instrument - 1])
              locked |= n != Inherit;
          } else {
            for (int n : hit.locks)
              locked |= n != Inherit;
          }
          for (const auto &row : st.fxLocks)
            for (int n : row)
              locked |= n != Inherit;
          if (between)
            c.rect(x + 7, y + 46, 52, 1, ink);
          if (locked)
            c.rect(x + 59, y + 31, 4, 4, ink);
          bool sustained =
              selected.on && tr.instrument &&
              ((s - a.step + tr.cycle) % tr.cycle) < selected.length;
          if (sustained)
            c.rect(x + 4, y + 48, 61, 2, on ? BG : DIM);
          if (playing &&
              unsigned(cell) == __atomic_load_n(&a.engine->shownStep[a.track],
                                                __ATOMIC_ACQUIRE) /
                                    unsigned(stride))
            c.rect(x, y - 6, 69, 2, TEXT);
        }
        if (s == a.step) {
          c.box(x - 3, y - 3, 75, 59, TEXT);
          c.box(x - 2, y - 2, 73, 57, TEXT);
        }
      }
      snprintf(b, sizeof(b), "PAGE %d/%d", selectedCell / 16 + 1,
               (tr.cells() + 15) / 16);
      text(c, 18, 269, b, DIM);
      snprintf(
          b, sizeof(b), "PLAY %02u/%02d",
          __atomic_load_n(&a.engine->shownStep[a.track], __ATOMIC_ACQUIRE) /
                  unsigned(stride) +
              1,
          tr.cells());
      text(c, 186, 269, b, DIM);
      text(c, 478, 269, "Y BANK", DIM);
      c.rect(18, 293, 604, 1, LINE);
      snprintf(b, sizeof(b), "STEP %03d / %s", selectedCell + 1,
               (tr.instrument ? selected.on : tr.hits[a.key][a.step].on)
                   ? "ON"
                   : "OFF");
      text(c, 18, 305, b, TEXT);
      int asset = tr.keys[a.key].asset;
      snprintf(b, sizeof(b), "%s / %s",
               tr.instrument ? midiNote(selected.note) : note(a.key),
               tr.instrument ? instrumentName(tr.instrument)
                             : a.assetName(asset));
      text(c, 350, 305, b, DIM, 2, 22);
      const char *banks[] = {"STEP", "VOICE LOCKS", "FX LOCKS", "CYCLE"};
      const int x[] = {18, 100, 280, 430}, width[] = {56, 140, 100, 68};
      for (int i = 0; i < 4; i++) {
        text(c, x[i], 326, banks[i], a.bank == i ? TEXT : DIM);
        if (a.bank == i)
          c.rect(x[i], 343, width[i], 1, TEXT);
      }
      text(c, 520, 326,
           a.bank == 2                    ? fxName(tr.fx.type)
           : a.bank == 1 && tr.instrument ? (a.synthBank ? "CHAR" : "CORE")
                                          : "Y NEXT",
           DIM, 2, 8);
    }
    if (a.page == Sampler && !a.project.tracks[a.track].instrument) {
      auto &tr = a.project.tracks[a.track];
      auto &k = tr.keys[a.key];
      snprintf(b, sizeof(b), "TRACK %02d / KEY %s / %s", a.track + 1,
               note(a.key), a.assetName(k.asset));
      text(c, 18, 108, b, DIM);
      for (int i = 0; i < Keys; i++) {
        int x = 18 + (i % 6) * 102, y = 140 + (i / 6) * 43;
        c.box(x, y, 92, 34, i == a.key ? TEXT : LINE);
        text(c, x + 8, y + 11, note(i), tr.keys[i].asset >= 0 ? TEXT : DIM);
        if (tr.keys[i].asset >= 0)
          c.rect(x + 78, y + 13, 4, 4, TEXT);
      }
      if (a.available(k.asset)) {
        auto &sample = a.engine->samples[k.asset];
        int left = 18 + 604 * k.start / 100, right = 18 + 604 * k.end / 100;
        c.rect(left, 235, right - left, 62, PANEL);
        for (int i = 0; i < 64; i++) {
          int h = clamp(sample.peaks[i] * 28 / 8192, 0, 28);
          c.rect(18 + i * 604 / 64, 266 - h, 5, h * 2 + 1, DIM);
        }
        c.rect(left, 235, 1, 62, TEXT);
        c.rect(right - 1, 235, 1, 62, TEXT);
        snprintf(b, sizeof(b), "ONE SHOT / A AUDITION / OPTIONS ASSIGN");
        text(c, 18, 321, b, DIM);
      } else
        text(c, 18, 260, "A ASSIGN SAMPLE / L1+R1 OPTIONS", TEXT);
    }
    drawTools(c, a);
    drawSynth(c, a);
    if (a.page == Pattern || a.page == Sampler ||
        (a.page == Mix && (a.project.trackCount || a.mixTab == 1)) ||
        a.page == Space || (a.page == Tape && a.tapeTab < 2)) {
      View v;
      v.buttons = a.buttons;
      v.holds = a.holds;
      auto &t = a.project.tracks[a.track];
      auto &s = t.steps[a.step];
      auto &k = t.keys[a.key];
      bool cycleOnly = a.page == Pattern && a.bank == 3;
      if (cycleOnly) {
        text(c, 330, 365, "RESOLUTION KEEPS TIME", DIM);
        text(c, 330, 394, "OFF-GRID EVENTS KEPT", DIM);
        if (t.cycle % t.stride()) {
          snprintf(b, sizeof(b), "LAST CELL %d/%d", t.cycle % t.stride(),
                   t.stride());
          text(c, 330, 423, b, DIM);
        } else
          text(c, 330, 423, "MAX 64 EIGHTH NOTES", DIM);
      }
      int fieldCount = cycleOnly                          ? 2
                       : a.fxPage() && !a.fxDetail        ? 0
                       : a.page == Tape && a.tapeTab == 0 ? 2
                                                          : 4;
      for (int i = 0; i < fieldCount; i++) {
        const char *label = "";
        const char *unit = "";
        bool disabled = false;
        if (a.page == Pattern && a.bank == 0 && !t.instrument) {
          const auto &h = t.hits[a.key][a.step];
          const char *l[] = {"VELOCITY", "FADE IN", "FADE OUT", "PROB"};
          int vals[] = {h.velocity, h.fadeIn, h.fadeOut, h.probability};
          label = l[i];
          snprintf(b, sizeof(b), "%d", vals[i]);
          unit = i == 1 || i == 2 ? "MS" : i == 3 ? "PERCENT" : "";
        } else if (a.page == Pattern && a.bank == 0) {
          const char *l[] = {"KEY", "VEL", "LENGTH", "PROB"};
          label = l[i];
          if (i == 0 && t.instrument)
            label = "NOTE";
          int val[] = {s.key, s.velocity,
                       (s.length + t.stride() - 1) / t.stride(), s.probability};
          if (i == 0)
            snprintf(b, sizeof(b), "%s",
                     t.instrument ? midiNote(s.note) : note(val[i]));
          else
            snprintf(b, sizeof(b), "%d", val[i]);
          unit = i == 2 ? "STEPS" : i == 3 ? "PERCENT" : "";
          if (i == 2 && !t.instrument && t.keys[s.key].mode == 0) {
            strcpy(b, "OFF");
            unit = "ONE SHOT";
          }
        } else if (a.page == Pattern && a.bank == 3) {
          label = i == 0 ? "CYCLE" : "STEP";
          if (i == 0)
            snprintf(b, sizeof(b), "%d", t.cells());
          else
            snprintf(b, sizeof(b), "1/%d", t.division);
          unit = i == 0 ? "CELLS" : "RESOLUTION";
        } else if (a.fxPage() || (a.page == Pattern && a.bank == 2)) {
          const auto &f = a.page == Mix ? a.project.fx : t.fx;
          label = fxLabel(f.type, i);
          int value = f.values[f.type][i];
          if (a.page == Pattern) {
            value = s.fxLocks[f.type][i];
            unit = "STEP LOCK";
          }
          if (value == Inherit)
            strcpy(b, "--");
          else if (f.type == 0 && i == 0) {
            const char *n[] = {"1/16", "1/8", "3/16", "1/4",
                               "3/8",  "1/2", "3/4",  "1 BAR"};
            snprintf(b, sizeof(b), "%s", n[value]);
          } else
            snprintf(b, sizeof(b), "%d", value);
        } else if (a.page == Tape) {
          if (a.tapeTab == 0) {
            const char *l[] = {"SOURCE", "LENGTH", "--", "--"};
            label = l[i];
            if (i == 0) {
              if (a.captureSource == 0)
                strcpy(b, "MASTER");
              else
                snprintf(b, sizeof(b), "TR %02d", a.captureSource);
            } else if (i == 1)
              snprintf(b, sizeof(b), "%d", a.captureBars);
            else
              strcpy(b, "--");
            unit = i == 1 ? "BARS" : "";
          } else {
            const char *l[] = {"START", "END", "FADE IN", "FADE OUT"};
            label = l[i];
            auto &cut = a.slices[a.sliceIndex];
            int val[] = {cut.start, cut.end, cut.fadeIn, cut.fadeOut};
            if (a.sliceCount)
              snprintf(b, sizeof(b), "%d", val[i]);
            else
              strcpy(b, "--");
            unit = "MS";
          }
        } else if (a.page == Mix) {
          const char *l[] = {"LEVEL", "PAN", "MASTER", "MUTE"};
          label = l[i];
          int vals[] = {t.level, t.pan, a.project.master, t.mute};
          snprintf(b, sizeof(b), "%d", vals[i]);
          unit = i == 3 ? "0 OFF 1 ON" : "";
        } else if (t.instrument &&
                   (a.page == Sampler || (a.page == Pattern && a.bank == 1))) {
          int type = t.instrument - 1, parameter = i + a.synthBank * 4;
          label = synthLabel(t.instrument, parameter);
          int value = a.page == Sampler ? t.synthParams[type][parameter]
                                        : s.synthLocks[type][parameter];
          unit = a.page == Pattern ? "STEP LOCK"
                                   : synthUnit(t.instrument, parameter);
          if (value == Inherit)
            strcpy(b, "--");
          else if (t.instrument == FmInstrument && parameter == 1)
            snprintf(b, sizeof(b), "%s", ratioName(value));
          else
            snprintf(b, sizeof(b), "%d", value);
        } else {
          const char *l[] = {"PITCH", "START", "END", "LEVEL"};
          label = l[i];
          int val[] = {k.pitch, k.start, k.end, k.level};
          if (a.page == Pattern) {
            const auto &hit = t.hits[a.key][a.step];
            if (hit.locks[i] == Inherit) {
              strcpy(b, "--");
              disabled = true;
            } else
              val[i] = hit.locks[i];
            unit = "STEP LOCK";
          } else
            unit = i == 0 ? "SEMITONES" : "PERCENT";
          if (!disabled)
            snprintf(b, sizeof(b), "%d", val[i]);
          if (a.page == Sampler && k.asset < 0)
            strcpy(b, "--");
        }
        field(c, v, i, label, b, unit);
      }
    }
  }
  c.rect(0, 445, 640, 1, LINE);
  const char *hint =
      a.importAssign             ? "A NAME + ASSIGN / X PLAY / B VOICE"
      : a.assignMode             ? "A SELECT / X AUDITION / B VOICE"
      : a.fxPage() && a.fxDetail ? "X ON-OFF / L1+ARROWS FX / B CHAIN"
      : a.page == Atlas          ? "A OPEN / L1 R1 PROJECT / L1+R1 OPTIONS"
      : a.page == Grid           ? "A OPEN / L1 R1 TRACK / L1+R1 OPTIONS"
      : a.page == Mix            ? "Y BANK / A MUTE OR BYPASS / L1+R1 OPTIONS"
      : a.page == Pattern        ? "L1+ARROWS ITEM / Y BANK / L1+R1 OPTIONS"
      : a.page == Sampler        ? "A AUDITION / L1+R1 OPTIONS / B GRID"
      : a.page == Tape           ? "Y BANK / L1 R1 ITEM / L1+R1 OPTIONS"
                                 : "A EDIT / X BYPASS / Y+ARROWS ORDER";
  bool tempoFocus =
      a.page != Atlas && !a.options && (a.buttons.held & bit(Select));
  text(c, 18, 457,
       tempoFocus ? "TEMPO / SELECT + ARROWS / RELEASE TO KEEP"
       : a.status[0] && a.nowUs < a.statusUntil ? a.status
                                                : hint,
       DIM, 2, 50);
  if (a.options) {
    c.rect(54, 104, 532, 320, PANEL);
    c.box(54, 104, 532, 320, LINE);
    text(c, 72, 120,
         a.manageMenu == 1   ? "PROJECT / B CLOSE"
         : a.manageMenu == 2 ? "TRACK / B CLOSE"
                             : "OPTIONS / B CLOSE",
         DIM);
    if (a.optionCount() > 8) {
      snprintf(b, sizeof(b), "%d/%d", a.option / 8 + 1,
               (a.optionCount() + 7) / 8);
      text(c, 510, 120, b, DIM);
    }
    int first = a.option / 8 * 8;
    for (int i = first; i < a.optionCount() && i < first + 8; i++) {
      int y = 155 + (i - first) * 31;
      if (i == a.option)
        c.box(68, y - 6, 504, 27, TEXT);
      text(c, 80, y, a.optionName(i), i == a.option ? TEXT : DIM);
    }
  }
  drawOverlay(c, a);
  if (a.voicePicker) {
    c.rect(54, 104, 532, 320, PANEL);
    c.box(54, 104, 532, 320, LINE);
    text(c, 72, 120,
         a.voicePicker == 3   ? "ADD TRACK / B CANCEL"
         : a.voicePicker == 1 ? "INSTRUMENT / B CANCEL"
                              : "PRESET / B CANCEL",
         DIM);
    for (int i = 0; i < 4; i++) {
      int y = 166 + i * 45;
      if (i == a.voiceChoice)
        c.box(68, y - 8, 504, 33, TEXT);
      text(c, 80, y,
           a.voicePicker == 1 || a.voicePicker == 3
               ? instrumentName(i)
               : presetName(a.project.tracks[a.track].instrument, i),
           i == a.voiceChoice ? TEXT : DIM);
    }
    text(c, 72, 380, "D-PAD SELECT / A APPLY", DIM);
  }
  c.box(0, 0, 640, 480, LINE);
}
} // namespace moss
