#pragma once
#include "../../../hardware lab/src/lab_ui.h"
#include "capture_store.h"
#include "engine.h"
#include "library.h"
#include "name_keyboard.h"
#include "parameter_ui.h"
#include "song_storage.h"
#include "stream_reader.h"
#include "sample_slice.h"
#include "sample_waveform.h"
#ifdef _WIN32
#include <new>
#else
inline void *operator new(size_t, void *ptr) noexcept { return ptr; }
#endif
namespace moss {
enum Page { Atlas, Grid, Mix, Tape, Pattern, Sampler, Space, SongPage };
struct App {
  Project project = makeProject(true), undoProject, editCandidate;
  Engine *engine = nullptr;
  lab::Buttons buttons;
  lab::HoldStats holds;
  lab::ParameterGesture repeat, tempoRepeat, pageRepeat;
  bool pageUsed = false, pageTail = false;
  bool selectUsed = false, tempoTail = false, parameterTail = false;
  Page page = Atlas;
  int slot = 0, track = 0, step = 0, key = 0, bank = 0, option = 0;
  int timingBank = 0;
  int gridPageSize = 64;
  struct TrackView { int step=0,key=0,bank=0,synth=0,timing=0; };
  TrackView trackViews[Tracks];
  int viewTrack=0;
  int effectBanks[Tracks+1][2][EffectTypes]={};
  bool menuConsumed=false;
  int lockMenu=-1, lockAction=0;
  int synthBank = 0, fxBank = 0;
  int effectPageMemory[EffectTypes]={};
  int fxCursor[Tracks+1]={-1,-1,-1,-1,-1},lockFx[Tracks]={-1,-1,-1,-1};
  bool fxDetail = false, nestedUsed = false, nestedTail = false;
  lab::ParameterGesture nestedRepeat;
  int voicePicker = 0, voiceChoice = 0; // 1 instrument, 2 synth preset, 3 add track, 4 REEL preset
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
  bool deletedProjectAvailable = false;
  int nameTarget = 0,
      confirmTarget = 0; // 0 sample, 1 project, 2 track, 3 new project
  int manageMenu = 0;    // 1 project, 2 track
  Library *library = nullptr;
  CaptureStore *captureStore = nullptr;
  bool captureWritePending = false;
  WavSource source;
  StreamReader streamReader;
  bool previewWhole = false;
  using Slice=SampleSlice;
  Slice slices[16];
  SampleWaveform waveform,detailWaveform;
  int sliceIndex = 0, sliceCount = 0;
  int tapeTab = 0, mixTab = 0, captureSource = 0, captureBars = 1, asset = 0,
      fileCursor = 0;
  bool importAssign = false, saveKitAssign=false;
  int importTrack = 0, importKey = 0;
  char saveLabel[25] = {};
  bool filePicker = false, assignMode = false, capturePending = false,
       captureFailed = false, discardPending = false;
  bool naming = false, confirmDelete = false;
  int nameCursor = 0, nameKey = 0;
  bool nameSelected = false;
  AssetMask savedAssetMasks[Projects + 1] = {};
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
    StopPreview,
    StartSong, LoadKit, AssignAsset
  };
#include "app_song.h"
  Job job = NoJob;
  char pendingPath[512] = {};
#include "app_sample_folders.h"
#include "app_slice_draft.h"
  unsigned captureState() const {
    return __atomic_load_n(&engine->capture.state, __ATOMIC_ACQUIRE);
  }
  bool captureBusy() const {
    return capturePending || discardPending || captureState() != 0;
  }
  bool fxPage() const { return page == Space || (page == Mix && mixTab == 1); }
  const char *assetName(int id) const {
    if(library && library->has(id)) return library->names[id];
    if (id < 0 || id >= engine->sampleCount)
      return "UNASSIGNED";
    if (!engine->samples[id].frames)
      return "MISSING FILE";
    return library  ? library->names[id]
           : id < 3 ? lab::sampleNames[id]
                    : "SAMPLE";
  }
  bool available(int id) const {
    return library ? library->has(id) : id >= 0 && id < engine->sampleCount && engine->samples[id].frames;
  }
  bool granularTrack() const { return project.tracks[track].instrument == GranularInstrument; }
  int voiceBanks() const { return granularTrack() ? GranularBankCount : synthBanks(project.tracks[track].instrument); }
  bool assignSample(int id) {
    return change(granularTrack() ? Command{Op::GranularSource, track, 0, 0, id}
                           : Command{Op::Key, track, key, 0, id});
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
    asset = -1; libraryFolder=-1; // IMPORT precedes prepared assets.
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
    return s.first(source.rate);
  }
  unsigned sliceEnd(const Slice &s) const {
    return s.last(source.rate,source.frames);
  }
  void limitSlice(Slice &s, int movedEdge) {
    if (!source.rate || !source.channels)
      return;
    unsigned maximum = SampleBudget / (source.channels * 2);
    if (sliceFrames(s) > maximum) {
      if (movedEdge == 1) {
        unsigned first = sliceEnd(s) - maximum;
        s.setFirst(first,source.rate);
      } else {
        uint64_t last = uint64_t(sliceStart(s)) + maximum;
        s.setLast(unsigned(sampleMin(last,uint64_t(source.frames))),source.rate,source.frames);
      }
    }
    int length=int(uint64_t(sliceFrames(s))*1000/source.rate);
    s.fadeIn = clamp(s.fadeIn, 0, length);
    s.fadeOut = clamp(s.fadeOut, 0, length - s.fadeIn);
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
    pumpWaveform();
  }
  bool assetUsed(int id) {
    auto uses = [&](const Project &p) {
      for (int i = 0; i < p.trackCount; i++) {
        if (p.tracks[i].granular.asset == id) return true;
        for (const auto &k : p.tracks[i].keys)
          if (k.asset == id)
            return true;
      }
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
        browseSources();
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
    } else if(tapeTab==2 && asset<=-2) {
      if(b==lab::A) {
        if(asset==-2){asset=-10-libraryFolder;libraryFolder=-1;}
        else {libraryFolder=-10-asset;asset=library->kits[libraryFolder].ids[0];}
      }
    } else if (tapeTab == 2 && asset == -1) {
      if (b == lab::A) {
        importAssign = assignMode;
        importTrack = track;
        importKey = key;
        browseSources();
      }
    } else if (tapeTab == 2 && library->has(asset)) {
      if (b == lab::X)
        job = PreviewAsset;
      if (b == lab::A) {
        if (assignMode) {
          job=AssignAsset;
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
    if (captureWritePending) {
      unsigned result = captureStore->poll();
      if (result == 1)
        return;
      captureWritePending = false;
      if (result != 2) {
        captureFailed = true;
        say("CAPTURE KEPT IN RAM / A RETRY SAVE");
        return;
      }
      source = captureStore->source;
      finishCaptureSave();
      return;
    }
    char p[128];
    int i = 1;
    for (; i < 10000; i++) {
      snprintf(p, sizeof(p), "captures/take-%04d.wav", i);
      if (!exists(p))
        break;
    }
    if (i < 10000 && captureStore) {
      captureStore->start(p, engine->capture.pcm, engine->capture.frames,
                          engine->capture.rate);
      captureWritePending = true;
      return;
    }
    if (i == 10000 || !writeWav(p, engine->capture.pcm, engine->capture.frames,
                                engine->capture.rate, 2)) {
      captureFailed = true;
      say("CAPTURE KEPT IN RAM / A RETRY SAVE");
      return;
    }
    if (source.open(p)) {
      source.overview();
    }
    finishCaptureSave();
  }
  void finishCaptureSave() {
    __atomic_store_n(&engine->capture.state, 0, __ATOMIC_RELEASE);
    fourSlices=false; sliceBank=0;
    sliceCount = 1;
    sliceIndex = 0;
    slices[0] = {0, durationMs(), 0, 0};
    limitSlice(slices[0], 0);
    if (library)
      library->rememberSource(source.path);
    if (page == Tape)
      tapeTab = 1;
    say("CAPTURE SAVED / TRIM THEN A SAVE SLICE");
  }
  // Only called with the audio worker paused. Never allocate/read files in
  // render().
  void service() {
    if (job == StartSong) {
      job = NoJob;
      streamReader.close();
      prepareSong();
      return;
    }
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
    resetSamples();
    if(current==LoadKit){loadKit();return;}
    if(current==AssignAsset) {
      AssetMask needed=projectAssets(project);needed.add(asset);
      if(!library->prepare(needed)){say(library->error);return;}
      if(!assignSample(asset)){resetSamples();return;}resetSamples();assignMode=false;page=Sampler;say("SAMPLE ASSIGNED");return;
    }
    if (current == PreviewSlice || current == PreviewSource ||
        current == PreviewFile) {
      bool whole = current != PreviewSlice;
      char filePath[512];
      if(current==PreviewFile) {
        if(!browser.count || browser.entries[fileCursor].folder || !browser.fullPath(fileCursor,filePath,sizeof(filePath)))return;
      } else snprintf(filePath,sizeof(filePath),"%s",source.path);
      const char *file=filePath;
      unsigned first = whole ? 0 : sliceStart(slices[sliceIndex]),
               last = whole ? 0 : sliceEnd(slices[sliceIndex]);
      int in = whole ? 0 : slices[sliceIndex].fadeIn,
          out = whole ? 0 : slices[sliceIndex].fadeOut;
      if (!streamReader.begin(engine->stream, file, engine->rate, first, last,
                              in, out))
        say(streamReader.source.error);
      else {
        previewWhole = whole;previewSliceIndex=whole?-1:sliceIndex;
        if(!whole && fourSlices) {
          char message[80];snprintf(message,sizeof(message),"PART %d PLAY / %s STOP",sliceIndex+1,lab::names[ParameterButtons[sliceIndex]]);say(message);
        } else say(whole ? "SOURCE AUDITION / X STOP" : "SLICE AUDITION / X STOP");
      }
      return;
    }
    if (current == OpenSource) {
      WavSource candidate;
      if (!candidate.open(pendingPath)) {
        say(candidate.error);
        return;
      }
      if(!saveSliceDraft())return;
      source = candidate;
      source.overview();
      fourSlices=false;sliceBank=0;
      sliceCount = 1;
      sliceIndex = 0;
      slices[0] = {0, durationMs(), 0, 0};
      limitSlice(slices[0], 0);
      restoreSliceDraft();
      page = Tape;
      tapeTab = 1;
      filePicker = false;
      assignMode = false;
      say("A SAVE SLICE / X AUDITION / L1/R1 BANK");
    } else if (current == PreviewAsset) {
      AssetMask needed=projectAssets(project);needed.add(asset);
      if(!library->prepare(needed)){say(library->error);return;}
      engine->init(project,library->views,engine->rate,CatalogAssets);
      engine->auditionBuffer(library->views[asset]);
    } else if (current == DeleteAsset) {
      if (assetUsed(asset)) {
        say("IN USE BY SEQUENCE OR UNDO / FILE KEPT");
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
    } else if(current==SaveAll && fourSlices) {
      int kit=library->saveKit(source,slices,saveLabel);
      if(kit<0){say(library->error);return;}
      libraryFolder=kit;asset=library->kits[kit].ids[0];
      if(saveKitAssign) {
        if(importAssign)track=importTrack;saveKitAssign=false;requestKit(kit);
      } else {tapeTab=2;say("FOLDER SAVED / FOUR NUMBERED SAMPLES");}
      return;
    } else if (sliceCount) {
      int first = current == SaveAll ? 0 : sliceIndex,
          last = current == SaveAll ? sliceCount - 1 : sliceIndex;
      if (current != PreviewSlice &&
          CatalogAssets - library->count() < last - first + 1) {
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
        if(!assignSample(asset)){say("SAMPLE SAVED / ASSIGNMENT FAILED / RETRY IN LIBRARY");return;}
        importAssign = assignMode = false;
        page = Sampler;
        resetSamples();
        say("NAMED SAMPLE SAVED AND ASSIGNED");
        return;
      }
      resetSamples();
      char m[80];
      snprintf(m, sizeof(m), "%d SLICE SAVED / R1 LIBRARY / VOICE ASSIGN",
               savedCount);
      say(m);
    }
  }

  void path(char *out, int n, int index) const {
    snprintf(out, n, "projects/project-%d.moss", index + 1);
  }
  void scan() {
    scanSongs();
    deletedProjectAvailable = exists("projects/deleted.moss");
    projectCount = 0;
    for(auto &mask:savedAssetMasks)mask=AssetMask{};
    if (deletedProjectAvailable) {
      Project deleted;
      if (loadProject("projects/deleted.moss", deleted))
        savedAssetMasks[Projects] = assetMask(deleted);
      else
        savedAssetMasks[Projects] = ~uint32_t(0);
    }
    for (int i = 0; i < Projects; i++) {
      char p[128];
      path(p, sizeof(p), i);
      saved[i] = exists(p);
      Project temp;
      broken[i] = saved[i] && !loadProject(p, temp);
      if (saved[i]) {
        if (broken[i])
          savedAssetMasks[i] = ~uint32_t(0);
        else
          savedAssetMasks[i] = assetMask(temp);
        projectRows[projectCount++] = i;
        snprintf(projectNames[i], 25, "%s",
                 broken[i] ? "DAMAGED SEQUENCE" : temp.name);
        projectTracks[i] = broken[i] ? 0 : temp.trackCount;
        projectTempos[i] = broken[i] ? 0 : temp.bpm;
      }
    }
    projectCursor = clamp(projectCursor, 0, projectCount);
  }
  const char *name() const {
    return page == SongPage ? song.name : project.name;
  }
  void seedProjects() {
    restoreLastDraft();
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
    savedAssetMasks[slot] = assetMask(project);
    say("SEQUENCE SAVED");
    return true;
  }
  bool open(int index) {
    char p[128];
    path(p, sizeof(p), index);
    Project temp;bool adaptedReel=false;
    if (exists(p)) {
      if (!loadProject(p, temp,&adaptedReel)) {
        say("SEQUENCE DAMAGED / ORIGINAL FILE KEPT");
        return false;
      }
    } else {
      say("SEQUENCE FILE MISSING / CREATE NEW");
      scan();
      return false;
    }
    project = temp;
    for(auto &v:trackViews)v=TrackView{};for(auto &v:fxCursor)v=-1;for(auto &v:lockFx)v=-1;memset(effectBanks,0,sizeof(effectBanks));viewTrack=0;
    slot = index;
    page = Grid;
    track = step = key = bank = 0;
    synthBank = 0;
    dirty = !exists(p);
    haveUndo = false;
    options = false;
    voicePicker = 0;
    resetSamples();
    say(adaptedReel?"LEGACY REEL ADAPTED / CHECK SOUND":"START PLAY / A OPEN TRACK / L1+R1 OPTIONS");
    return true;
  }
  bool send(Command c) {
    if (!engine->commands.push(c)) {
      say("INPUT QUEUE FULL / TRY AGAIN");
      return false;
    }
    return true;
  }
#include "app_mutation.h"
  bool change(Command c) {
    if((c.op==Op::Fx||c.op==Op::FxEnable)&&c.field==1&&c.value){const auto &f=c.track==Tracks?project.fx:project.tracks[c.track].fx;
      if(!(c.op==Op::FxEnable?f.active(c.index):f.enabled)&&f.count()>=ActiveEffects){say("2 FX MAX / BYPASS ONE TO ENABLE ANOTHER");return false;}}
    if(!commandChanges(c))return true;
    if(!send(c))return false;
    if(!editing){undoProject=project;haveUndo=true;editing=true;}
    apply(project,c);dirty=true;return true;
  }
#include "app_lock_controls.h"
#include "app_management.h"
  int effectType() const {
    int context=page==Mix?Tracks:track;const auto &f=context==Tracks?project.fx:project.tracks[track].fx;
    int selected=page==Pattern?lockFx[track]:fxCursor[context];return selected<0?f.type:selected;
  }
  Effect effectView() const {Effect f=page==Mix?project.fx:project.tracks[track].fx;f.select(effectType());return f;}
  void nextEffect(int direction) {
    auto f=effectView();int context=page==Mix?Tracks:track;
    effectBanks[context][0][f.type]=clamp(fxBank,0,fxBanks(f.type)-1);
    int at=0;while(at<EffectTypes-1&&f.order[at]!=f.type)++at;
    fxCursor[context]=f.order[clamp(at+direction,0,EffectTypes-1)];
    fxBank=effectBanks[context][0][effectType()];
  }
  int patternTabCount() const { return 3+project.tracks[track].fx.count(); }
  int patternTabIndex() const {
    if(bank<2)return bank;
    const auto &f=project.tracks[track].fx;
    if(bank==3)return patternTabCount()-1;
    int index=2;for(int id:f.order)if(f.active(id)){if(id==effectType())return index;++index;}
    return 0;
  }
  void choosePatternTab(int index) {
    auto &f=project.tracks[track].fx;
    if(bank==2)effectBanks[track][1][effectType()]=fxBank;
    if(index<2){bank=index;return;}
    if(index==patternTabCount()-1){bank=3;return;}
    int at=2;for(int id:f.order)if(f.active(id)&&at++==index){
      lockFx[track]=id;bank=2;fxBank=effectBanks[track][1][id];return;
    }
  }
  void rememberFxContext(){if(page==Pattern&&bank==2)effectBanks[track][1][effectType()]=clamp(fxBank,0,fxBanks(effectType())-1);else if(fxPage())effectBanks[page==Mix?Tracks:track][0][effectType()]=clamp(fxBank,0,fxBanks(effectType())-1);}
  void restoreFxContext(){
    if(page==Pattern&&bank==2){if(!project.tracks[track].fx.active(effectType())){bank=0;return;}fxBank=effectBanks[track][1][effectType()];}
    else if(fxPage())fxBank=effectBanks[page==Mix?Tracks:track][0][effectType()];
    fxBank=clamp(fxBank,0,fxBanks(effectType())-1);
  }
  void rememberTrackView() {
    rememberFxContext();
    if(viewTrack>=0&&viewTrack<Tracks)trackViews[viewTrack]={step,key,bank,synthBank,timingBank};
  }
  void restoreTrackView() {
    if(track<0||track>=project.trackCount)return;
    viewTrack=track;auto &v=trackViews[track];step=clamp(v.step,0,project.tracks[track].cycle-1);key=v.key;bank=v.bank;synthBank=clamp(v.synth,0,voiceBanks()-1);timingBank=v.timing;
    restoreFxContext();
  }
  void nested(int d) {
    bool vertical=buttons.held&(lab::bit(lab::Up)|lab::bit(lab::Down));
    if(!vertical){
      if((page==Pattern||page==Sampler)&&!project.tracks[track].instrument)key=clamp(key+d,0,Keys-1);
      else if(page==Tape&&tapeTab==1&&sliceCount){if(previewing())job=StopPreview;sliceIndex=clamp(sliceIndex+d,0,sliceCount-1);resetFineView();}
      return;
    }
    if(page==Tape&&tapeTab==1&&fourSlices){sliceBank=(sliceBank+3+d)%3;resetFineView();return;}
    if(page==Pattern&&bank==3)timingBank=(timingBank+2+d)%2;
    else if((page==Pattern&&bank==2)||(fxPage()&&fxDetail)){
      const auto f=effectView();
      fxBank=(fxBank+fxBanks(f.type)+d)%fxBanks(f.type);
    }else if((page==Sampler||(page==Pattern&&bank==1))&&project.tracks[track].instrument)synthBank=(synthBank+voiceBanks()+d)%voiceBanks();
  }
  int parameterDelta(int repeatAmount, int coarse = 10) const {
    return repeatAmount *
           ((buttons.held & (lab::bit(lab::Up) | lab::bit(lab::Down))) ? coarse
                                                                       : 1);
  }
  int editLock(int value,int base,int low,int high,int direction,int delta) const {
    return clamp((value==Inherit || value==ResetLock?base:value)+direction*delta,low,high);
  }
  void edit(int i, int d, int amount) {
    if (page == SongPage) {
      editSong(i, d, amount);
      return;
    }
    int repetitions = amount;
    amount = parameterDelta(repetitions);
    if (!project.trackCount && page != Tape && !fxPage())
      return;
    auto &t = project.tracks[track];
    auto &s = t.steps[step];
    auto &k = t.keys[key];
    if (page == Pattern) {
      auto &h = t.hits[key][step];
      if(bank==0 && selectedEvent(t,step,key)==LockEvent && i<3) return;
      if (bank == 0 && !t.instrument) {
        const int fields[] = {HitVelocity, HitFadeIn, HitLength,
                              HitProbability};
        bool trackOnly=h.on==EmptyEvent && s.on==LockEvent;
        if(i==3 && trackOnly) { change({Op::Step,track,step,4,clamp(s.probability+d*amount,0,100)}); return; }
        int vals[] = {h.velocity, h.fadeIn, h.length, h.probability};
        int hi[] = {127, 5000, Steps, 100};
        int increment = i == 2 ? t.stride() : 1;
        amount = parameterDelta(repetitions, i == 1 ? 100 : i == 2 ? 16 : 10);
        change({Op::Hit, track, step, hitField(key, fields[i]),
                clamp(vals[i] + d * amount * increment, 0, hi[i])});
        return;
      }
      if (bank == 0) {
        if (i == 2 && !t.instrument && t.keys[s.key].mode == 0)
          return;
        if (i == 0 && t.instrument) {
          change({Op::Step, track, step, 9,
                  clamp(s.note + d * parameterDelta(repetitions, 12), 24, 96)});
          return;
        }
        int vals[] = {s.key, s.velocity, s.length, s.probability};
        int lo[] = {0, 0, 1, 0}, hi[] = {Keys - 1, 127, Steps, 100};
        if (i == 2)
          amount = parameterDelta(repetitions, 16) * t.stride();
        int v = i == 0 ? assignedKey(s.key, d)
                       : clamp(vals[i] + d * amount, lo[i], hi[i]);
        change({Op::Step, track, step, i + 1, v});
      } else if (bank == 1) {
        if (granularTrack()) {
          i += synthBank * 4;
          const auto &spec=GranularSpecs[i];
          change({Op::GranularLock,track,step,i,editLock(s.grainLocks[i],t.granular.values[i],spec.low,spec.high,d,parameterDelta(repetitions,spec.coarse))});
          return;
        }
        if (t.instrument) {
          int type = t.instrument - 1;
          i += synthBank * 4;
          const auto &spec=SynthParameters[type][i];
          change({Op::SynthLock,track,step,type*SynthParameterCount+i,editLock(s.synthLocks[type][i],t.synthParams[type][i],synthMin(t.instrument,i),synthMax(t.instrument,i),d,parameterDelta(repetitions,spec.coarse))});
          return;
        }
        const auto &sk = t.keys[key];
        amount = parameterDelta(repetitions, i == 0 ? 12 : 10);
        int base[] = {sk.pitch, sk.start, sk.end, sk.level};
        int v=editLock(h.locks[i],base[i],i==0?-24:0,i==0?24:100,d,amount);
        if(!lockValue(v)) { change({Op::Hit,track,step,hitField(key,HitLock0+i),v}); return; }
        if (i == 1)
          v = clamp(v, 0, (!lockValue(h.locks[2]) ? sk.end : h.locks[2]) - 1);
        if (i == 2)
          v = clamp(v, (!lockValue(h.locks[1]) ? sk.start : h.locks[1]) + 1,
                    100);
        change({Op::Hit, track, step, hitField(key, HitLock0 + i), v});
      } else if (bank == 2) {
        int type = effectType();
        i += clamp(fxBank,0,fxBanks(type)-1)*4;
        amount = parameterDelta(repetitions, fxCoarseStep(type, i));
        change({Op::FxLock,track,step,type*EffectParameters+i,
                editLock(s.fxLocks[type][i],t.fx.values[type][i],fxLow(type,i),fxHigh(type,i),d,amount)});
      } else {
        if(timingBank && i<2) {
          if(i==0) change({Op::Track,track,0,TrackSwingField,
            clamp(t.swing+d*amount,SwingStraight,SwingMaximum)});
          else change({Op::Step,track,step,StepShiftField,
            clamp((s.shift==Inherit?0:s.shift)+d*amount,-ShiftLimit,ShiftLimit)});
          return;
        }
        if (i == 0)
          change(
              {Op::Track, track, 0, 0,
               clamp(t.cycle + d * parameterDelta(repetitions, 16) * t.stride(),
                     1, Steps)});
        if (i == 1) {
          int value = d > 0 ? t.division * 2 : t.division / 2;
          change({Op::Track, track, 0, 4, clamp(value, 8, 64)});
          step = step / t.stride() * t.stride();
        }
        if(i==2) {
          const int values[]={Inherit,0,8,16,32,64}; int index=0;
          while(index<5 && values[index]!=s.repeat) ++index;
          change({Op::Step,track,step,StepRepeatField,values[clamp(index+d,0,5)]});
        }
        if(i==3) change({Op::SetEventType,track,step,key,clamp(selectedEvent(t,step,key)+d,0,2)});
        step = clamp(step, 0, t.cycle - 1);
      }
    } else if (page == Sampler && granularTrack()) {
      i += synthBank * 4;
      int base = t.granular.values[i];
      if (i == 4 && !editing) {
        auto view = engine->grainView[track].read();
        if (view.running) base = int((view.head * 1000u + 32768u) / 65536u);
      }
      int v = granularEdit(i, base, d, repetitions);
      if (i == 5) v = clamp(v, 0, t.granular.values[6] - 1);
      if (i == 6) v = clamp(v, t.granular.values[5] + 1, 1000);
      if (i == 12 || i == 13) {
        int other = i == 12 ? 13 : 12;
        if (v + t.granular.values[other] > 100) change({Op::GranularParam, track, 0, other, 100-v});
      }
      change({Op::GranularParam, track, 0, i, v});
    } else if (page == Sampler && t.instrument) {
      i += synthBank * 4;
      int type = t.instrument - 1;
      change(
          {Op::SynthParam, track, type, i,
           synthEdit(t.instrument, i, t.synthParams[type][i], d, repetitions)});
    } else if (page == Sampler && k.asset >= 0) {
      amount = parameterDelta(repetitions, i == 0 ? 12 : 10);
      int vals[] = {k.pitch, k.start, k.end, k.level};
      int lo[] = {-24, 0, k.start + 1, 0}, hi[] = {24, k.end - 1, 100, 100};
      change({Op::Key, track, key, i + 1,
              clamp(vals[i] + d * amount, lo[i], hi[i])});
    } else if (fxPage()) {
      if (!fxDetail) {
        if (i == parameterSlot(lab::Y))
          change({Op::Fx, page == Mix ? Tracks : track,
                  effectType(), FxMoveField,
                  (buttons.held & (lab::bit(lab::Up) | lab::bit(lab::Down)))
                      ? -d
                      : d});
        return;
      }
      int target = page == Mix ? Tracks : track;
      auto f = effectView();
      i += clamp(fxBank,0,fxBanks(f.type)-1)*4;
      amount = parameterDelta(repetitions, fxCoarseStep(f.type, i));
      change({Op::Fx, target, f.type, i + 2,
              clamp(f.values[f.type][i] + d * amount, fxLow(f.type, i),
                    fxHigh(f.type, i))});
    } else if (page == Tape && !filePicker && !naming && !confirmDelete) {
      if (captureBusy())
        return;
      if (tapeTab == 0) {
        if (i == 0)
          captureSource =
              clamp(captureSource + d * repetitions, 0, project.trackCount);
        if (i == 1)
          captureBars =
              clamp(captureBars + d * parameterDelta(repetitions, 4), 1, 8);
      }
      if (tapeTab == 1 && sliceCount) {
        if(fourSlices && sliceBank==0){moveSlice(i,d,repetitions);return;}
        if(fourSlices && sliceBank==2){editFineSlice(i,d,repetitions);return;}
        auto &cut = slices[sliceIndex];
        if(i<2){editSliceEdge(i,d*parameterDelta(repetitions,100),false);return;}
        int *vals[] = {&cut.start, &cut.end, &cut.fadeIn, &cut.fadeOut};
        int lo[] = {0, cut.start + 1, 0, 0},
            hi[] = {cut.end - 1, durationMs(),
                    cut.end - cut.start - cut.fadeOut,
                    cut.end - cut.start - cut.fadeIn};
        int delta = parameterDelta(repetitions, 100);
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
    int delta = parameterDelta(amount, SynthParameters[kind - 1][i].coarse);
    return clamp(v + d * delta, synthMin(kind, i), synthMax(kind, i));
  }
  int granularEdit(int i, int value, int direction, int repetitions) const {
    const auto &s = GranularSpecs[i];
    return clamp(value + direction * parameterDelta(repetitions, s.coarse), s.low, s.high);
  }
#include "app_menu.h"
  void backToAtlas() {
    if (captureBusy()) {
      say("FINISH CAPTURE FIRST");
      return;
    }
    if(!saveSliceDraft())return;
    if (dirty && !save())
      return;
    if (songDirty && !saveArrangement())
      return;
    page = inSongSequence ? SongPage : Atlas;
    inSongSequence = false;
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
    if(!saveSliceDraft())return;
    if ((!dirty || save()) && (!songDirty || saveArrangement()))
      exit = true;
  }
  void action(int b) {
    if(lockMenu>=0){
      if(b==lab::B||b==lab::Select){lockMenu=-1;return;}
      if(b==lab::A){if(lockMenu==4){int field=lockAction;lockMenu=-1;openLockMenu(field);}else{applyFieldLock(lockMenu,lockAction);lockMenu=-1;}}return;
    }

    if (sequencePicker) {
      if (b == lab::B || b == lab::Select) {
        sequencePicker = 0;
        pendingSongInsert = 0;
      } else if (b == lab::A)
        chooseSequence();
      return;
    }
    if (voicePicker) {
      if (b == lab::B || b == lab::Select)
        voicePicker = 0;
      if (b == lab::A) {
        synthBank = 0;
        editing = false;
        if(voicePicker==4) {
          change({Op::ReelPreset,page==Mix?Tracks:track,0,0,voiceChoice});
          voicePicker=0;editing=false;fxDetail=true;fxBank=0;say("REEL PRESET APPLIED / UNDO IN MENU");return;
        }
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
        copySource = -1;
        pendingSongInsert = 0;
      } else if (b == lab::X)
        deleteNameCharacter();
      else if (b == lab::Start || (b == lab::A && nameKey == NameSave))
        commitName();
      else if (b == lab::A && nameKey == NameDelete)
        deleteNameCharacter();
      else if (b == lab::A)
        insertNameCharacter(NameCharacters[nameKey]);
      return;
    }
    if (confirmDelete) {
      if (b == lab::B || b == lab::Select)
        confirmDelete = false;
      if (b == lab::A) {
        confirmDelete = false;
        if (confirmTarget == 3)
          removeArrangement();
        else if (confirmTarget == 1)
          removeProject();
        else if(confirmTarget==7){editing=false;change({Op::ClearAllAtStep,track,step,0,0});editing=false;}
        else if(confirmTarget==8){editing=false;change({Op::ClearPattern,track,0,0,0});editing=false;}
        else if(confirmTarget==9){job=LoadKit;}
        else if(confirmTarget==10){deleteSelectedKit();}
        else if (confirmTarget == 2)
          removeTrack();
        else
          job = DeleteAsset;
      }
      return;
    }
    if (filePicker) {
      if(b==lab::B || b==lab::Select)browseBack();
      if(b==lab::A)browseOpen();
      if(b==lab::X && browser.count && !browser.entries[fileCursor].folder)previewToggle(PreviewFile);
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
    if(page==Tape && tapeTab==1 && fourSlices && b>=lab::A && b<=lab::Y) {
      auditionSlice(parameterSlot(b));return;
    }
    if(page==Tape && tapeTab==2 && b==lab::Y) {
      int kit=asset<=-10?-10-asset:libraryFolder;
      if(kit>=0)requestKit(kit);return;
    }
    if(page==Tape && tapeTab==2 && b==lab::B && libraryFolder>=0) {
      asset=-10-libraryFolder;libraryFolder=-1;return;
    }
    if (page == SongPage) {
      songAction(b);
      return;
    }
    if (b == lab::Select) {
      if (importAssign || assignMode) {
        if(fourSlices && tapeTab==1){importAssign=assignMode=false;page=Sampler;job=StopPreview;return;}
        action(lab::B);
        return;
      }
      if (page != Atlas)
        backToAtlas();
      return;
    }
    if (b == lab::B) {
      if(page==Tape)saveSliceDraft();
      if(page==Pattern||page==Sampler||page==Space)rememberTrackView();
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
    if (b == lab::Y)
      return; // Y is the fourth parameter, never navigation.
    if (page == Sampler && granularTrack() && b == lab::X) {
      if (!available(project.tracks[track].granular.asset)) {
        say("ASSIGN A SAMPLE FIRST");
        return;
      }
      change({Op::GranularFreeze, track, 0, 0, 1 - project.tracks[track].granular.freeze});
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
        auto f = effectView();
        change({Op::FxEnable, page == Mix ? Tracks : track, f.type, 1, !f.enabled});
      }
      return;
    }
    if (b == lab::A) {
      editing = false;
      if (page == Atlas) {
        if (atlasTab) {
          if (songCursor == songCount)
            beginName(6, "UNTITLED");
          else
            openArrangement(songRows[songCursor]);
          return;
        }
        if (projectCursor == projectCount)
          beginSequenceCreate();
        else
          requestedSlot = projectRows[projectCursor];
      }
      if (page == Grid) {
        if (track >= project.trackCount) {
          if (project.trackCount == Tracks) {
            say("4 TRACKS MAXIMUM");
            return;
          }
          voicePicker = 3;
          voiceChoice = OrganismInstrument;
          return;
        }
        page = Pattern;
        restoreTrackView();
      } else if (page == Pattern) {
        auto &t = project.tracks[track];
        if (!t.instrument && !available(t.keys[key].asset)) {
          say("ASSIGN A SAMPLE IN VOICE FIRST");
          return;
        }
        change(t.instrument
                   ? Command{Op::Step, track, step, 0, t.steps[step].on==NoteEvent?EmptyEvent:NoteEvent}
                   : Command{Op::Hit, track, step, hitField(key, HitOn),
                             t.hits[key][step].on==NoteEvent?EmptyEvent:NoteEvent});
      } else if (page == Sampler) {
        if (granularTrack() && !available(project.tracks[track].granular.asset))
          pickAsset();
        else if (project.tracks[track].instrument)
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
                browser.count ? browser.count - 1 : 0);
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
        libraryMove(direction);
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
      synthBank = clamp(synthBank, 0, voiceBanks() - 1);
    }
  }
  void parameterPage(int direction) {
    if (sequencePicker)
      return;
    if (naming) {
      int length = int(strlen(nameEdit));
      nameCursor = nameSelected ? (direction < 0 ? 0 : length)
                                : clamp(nameCursor + direction, 0, length);
      nameSelected = false;
      return;
    }
    if (voicePicker || filePicker || confirmDelete || page == SongPage)
      return;
    if (page == Atlas) return;
    if (fxPage() && fxDetail) {
      const auto f=effectView();
      fxBank=(fxBank+fxBanks(f.type)+direction)%fxBanks(f.type);
    } else if (page == Pattern)
      choosePatternTab((patternTabIndex()+patternTabCount()+direction)%patternTabCount());
    else if (page == Sampler && project.tracks[track].instrument)
      synthBank = (synthBank + voiceBanks() + direction) % voiceBanks();
    else if (page == Sampler)
      return;
    else if (page == Mix) {
      mixTab = 1 - mixTab;
      fxDetail = false;
    } else if (page == Space) {
      return;
    } else if (page == Tape) {
      if(fourSlices && (tapeTab==1 || (!importAssign && !assignMode))) {
        if(captureBusy()){say("CAPTURE ACTIVE / A FINISH");return;}
        if(importAssign){int tab=tapeTab==2?3:sliceBank;tab=(tab+4+direction)%4;tapeTab=tab==3?2:1;sliceBank=tab==3?0:tab;resetFineView();return;}
        int tab=tapeTab==0?0:tapeTab==2?4:1+sliceBank;
        tab=(tab+5+direction)%5;
        tapeTab=tab==0?0:tab==4?2:1;
        if(tapeTab==1)sliceBank=tab-1;
        else if(previewing())job=StopPreview;
        resetFineView();return;
      }
      if (importAssign || assignMode)
        return;
      if (captureBusy()) {
        say("CAPTURE ACTIVE / A FINISH");
        return;
      }
      if (previewing())
        job = StopPreview;
      tapeTab = (tapeTab + 3 + direction) % 3;
    }
  }
  void jumpPage(int direction) {
    if (sequencePicker) {
      sequenceChoice = clamp(sequenceChoice + direction * 6, 0, projectCount);
      return;
    }
    if (options) {
      option = clamp(option + direction * 7, 0, optionCount() - 1);
      return;
    }
    if (page == SongPage) {
      songRow = clamp(songRow + direction * 4, 0, song.count);
      return;
    }
    if (page == Atlas && atlasTab) {
      songCursor = clamp(songCursor + direction * 5, 0, songCount);
      return;
    }
    if (page == Pattern) {
      const auto &tr = project.tracks[track];
      int cell = step / tr.stride();
      int next = clamp(cell / gridPageSize + direction, 0, (tr.cells() - 1) / gridPageSize);
      step = clamp(next * gridPageSize + cell % gridPageSize, 0, tr.cells() - 1) * tr.stride();
    } else if (options)
      option = clamp(option + direction * 7, 0, optionCount() - 1);
    else if (filePicker)
      fileCursor =
          clamp(fileCursor + direction * SourcePageRows, 0,
                browser.count ? browser.count - 1 : 0);
    else if(page==Tape && tapeTab==2 && library) libraryMove(direction*5);
    else if (page == Atlas)
      projectCursor = clamp(projectCursor + direction * 5, 0, projectCount);
    else if (page == Mix && mixTab == 0)
      track = clamp(track + direction * 4, 0,
                    project.trackCount ? project.trackCount - 1 : 0);
  }
  void navigate(int b) {
    if(lockMenu>=0){lockAction=clamp(lockAction+((b==lab::Right||b==lab::Down)?1:-1),0,lockMenu==4?3:2);return;}
    int d = (b == lab::Right || b == lab::Down) ? 1 : -1;
    if (sequencePicker) {
      sequenceChoice = clamp(sequenceChoice + d, 0, projectCount);
      return;
    }
    if (!options && !naming && !confirmDelete && page == SongPage) {
      songRow = clamp(songRow + d, 0, song.count);
      return;
    }
    if (voicePicker) {
      voiceChoice = clamp(voiceChoice + d, 0, voicePicker==4?ReelPresetCount-1:voicePicker == 2 ? synthPresetCount(project.tracks[track].instrument)-1 : InstrumentCount - 1);
      return;
    }
    if (naming) {
      int row = nameKey / NameColumns, column = nameKey % NameColumns;
      if (b == lab::Left || b == lab::Right)
        column = (column + NameColumns + d) % NameColumns;
      else
        row = (row + NameKeyCount / NameColumns + d) %
              (NameKeyCount / NameColumns);
      nameKey = row * NameColumns + column;
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
    if (page == Atlas && atlasTab)
      songCursor = clamp(songCursor + d, 0, songCount);
    else if (page == Atlas)
      projectCursor = clamp(projectCursor + d, 0, projectCount);
    else if (fxPage()) {
      if (!fxDetail)
        nextEffect(d);
    } else if (page == Grid || page == Mix)
      object(d);
    else if (page == Pattern)
      step = clamp(step + (b == lab::Down ? (gridPageSize==64 && project.tracks[track].cells()>16?16:8)
                           : b == lab::Up ? -(gridPageSize==64 && project.tracks[track].cells()>16?16:8)
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
      key = clamp(key + d, 0, Keys - 1);
  }
  void tick(uint64_t now) {
    nowUs = now;
    if(lockMenu>=0){if(buttons.held&15){int d=-1;for(int i=0;i<4;++i)if(buttons.held&lab::bit(i)){d=i;break;}if(nav!=d){nav=d;navigate(d);}}else nav=-1;return;}
    holds.update(buttons.held, now);
    if (!menuConsumed && (buttons.held & lab::bit(lab::Menu)) &&
        lab::deadlineReached(now, pressed[lab::Menu], 2000000)) {
      if (captureBusy()) {
        say("FINISH CAPTURE BEFORE EXIT");
        return;
      }
      if(!saveSliceDraft()){buttons.held&=~lab::bit(lab::Menu);return;}
      if ((page == Atlas || !dirty || save()) &&
          (!songDirty || saveArrangement()))
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
    if (page != Atlas && !sequencePicker && !options && !voicePicker &&
        !naming && !confirmDelete && !filePicker &&
        (buttons.held & lab::bit(lab::Select))) {
      nav = -1;
      repeat = lab::ParameterGesture{};
      for (int i = 0; i < 4; i++)
        if (buttons.held & lab::bit(lab::A + i))
          gestures[i] = true;
      if (buttons.held & 15)
        selectUsed = tempoTail = true;
      if (tempoRepeat.tick((buttons.held & 15) | lab::bit(lab::A), now)) {
        int bpm =
            clamp((page == SongPage ? song.bpm : project.bpm) +
                      tempoRepeat.direction * parameterDelta(tempoRepeat.step),
                  30, 240);
        if (page == SongPage) {
          songCheckpoint();
          song.bpm = bpm;
          if (songPlaying())
            send({Op::Global, 0, 0, 0, bpm});
        } else
          change({Op::Global, 0, 0, 0, bpm});
      }
      return;
    }
    tempoRepeat = lab::ParameterGesture{};
    if (pageTail && !(buttons.held & lab::bit(lab::R1))) {
      nav = -1;
      pageRepeat = lab::ParameterGesture{};
      if (!(buttons.held & 15))
        pageTail = false;
      return;
    }
    if ((buttons.held & lab::bit(lab::R1)) &&
        !(buttons.held & (lab::bit(lab::L1) | 0xf0)) && !voicePicker &&
        !naming && !confirmDelete) {
      nav = -1;
      repeat = lab::ParameterGesture{};
      unsigned arrows =
          buttons.held & (lab::bit(lab::Left) | lab::bit(lab::Right));
      if (arrows)
        pageUsed = pageTail = true;
      if (pageRepeat.tick(arrows | lab::bit(lab::A), now))
        jumpPage(pageRepeat.direction);
      return;
    }
    pageRepeat = lab::ParameterGesture{};
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
        nested((buttons.held&(lab::bit(lab::Up)|lab::bit(lab::Down)))?-nestedRepeat.direction:nestedRepeat.direction);
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
    if (!sequencePicker && !options && !voicePicker && !filePicker && !naming &&
        !confirmDelete && page != Atlas && repeat.tick(buttons.held, now))
      for (int i = 0; i < 4; i++)
        if (repeat.faces & (1u << i))
          edit(parameterSlot(lab::A + i), repeat.direction, repeat.step);
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
    if(lockMenu>=0&&down){pressed[b]=now;if(b>=lab::A&&b<=lab::Y)gestures[b-lab::A]=false;tick(now);return;}
    if (down) {
      pressed[b] = now;
      if(b==lab::Menu)menuConsumed=false;
      if((buttons.held&lab::bit(lab::Menu))&&(buttons.held&0xf0)&&!options&&!naming&&!confirmDelete&&!voicePicker&&!filePicker){
        menuConsumed=true;for(int face=lab::A;face<=lab::Y;++face)if(buttons.held&lab::bit(face)){gestures[face-lab::A]=true;if(lockMenu<0)openLockMenu(parameterSlot(face));}
      }
      if (b == lab::Select) {
        selectUsed = false;
        tempoTail = false;
        editing = false;
      }
      if ((b == lab::L1 || b == lab::R1) &&
          (buttons.held & lab::bit(lab::Select)))
        chord = true;
      if (b >= lab::A && b <= lab::Y && !(menuConsumed && (buttons.held & lab::bit(lab::Menu))))
        gestures[b - lab::A] = false;
      if ((buttons.held & (lab::bit(lab::L1) | lab::bit(lab::R1))) ==
          (lab::bit(lab::L1) | lab::bit(lab::R1))) {
        chord = true;
        if (!(buttons.held & (0xf0 | lab::bit(lab::Select))) && !voicePicker &&
            !sequencePicker && !filePicker && !naming && !confirmDelete) {
          options = !options && optionCount() > 0;
          manageMenu = 0;
          option = 0;
        }
      }
      if ((b == lab::L2 || b == lab::R2) &&
          !(buttons.held & (0xf0 | lab::bit(lab::Select))) && !options &&
          page != SongPage && !sequencePicker &&
          !voicePicker && !filePicker && !naming && !confirmDelete &&
          !importAssign && !assignMode) {
        if (previewing())
          job = StopPreview;
        rememberFxContext();
        fxDetail = false;
        int dir = b == lab::R2 ? 1 : 2;
        if(page==Atlas)atlasTab=1-atlasTab;
        else if (page >= Pattern)
          page = Page(Pattern + (page - Pattern + dir) % 3);
        else
          page = Page(Grid + (page - Grid + dir) % 3);
        restoreFxContext();
      }
      if (b == lab::Start)
        action(b);
    }
    tick(now);
    if (!down) {
      if(lockMenu>=0&&(b==lab::L1||b==lab::R1))return;
      if (b == lab::L1 || b == lab::R1) {
        if (!chord && !nestedUsed && !pageUsed && now-pressed[b]<350000 &&
            !(buttons.held & (0xf0 | lab::bit(lab::Select))) && !options &&
            !tempoTail)
          parameterPage(b == lab::R1 ? 1 : -1);
        if (!(buttons.held & (lab::bit(lab::L1) | lab::bit(lab::R1))))
          chord = false;
        if (b == lab::L1)
          nestedUsed = false;
        if (b == lab::R1)
          pageUsed = false;
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
    pageUsed = pageTail = false;
    pageRepeat = lab::ParameterGesture{};
    nestedRepeat = lab::ParameterGesture{};
    nav = -1;
    chord = false;
    editing = false;
    memset(gestures, 0, sizeof(gestures));
  }
};
#include "app_ui.h"
} // namespace moss
