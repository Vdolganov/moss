#include "../../../hardware lab/src/lab_input.h"
#include "app.h"
#include "diagnostics/stress_scenario.h"
#include <SDL/SDL.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <sched.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#ifndef __UCLIBC__
#error "Use pinned uClibc SDK"
#endif
static_assert(sizeof(input_event) == 16 && sizeof(void *) == 4,
              "Garlic ARM ABI required");
using namespace moss;
static App app;
static Engine engine;
static Library library;
static CaptureStore captureStore;
static lab::Canvas canvas;
static lab::Timing timing;
static uint32_t callbackHistogram[200] = {};
static bool profiling = false;
static int profileKind = 0;
static unsigned profileMask = 0;
static unsigned profileSeconds = 0;
static uint64_t profileStart = 0;
static uint64_t uiFrameMaxUs = 0;
static unsigned uiSlowFrames = 0;
static bool audioSchedulingSet = false;
static int audioSchedulingError = 0;
static constexpr int AudioPriority = 10;
static uint64_t micros() {
  timespec t = {};
  clock_gettime(CLOCK_MONOTONIC, &t);
  return uint64_t(t.tv_sec) * 1000000 + t.tv_nsec / 1000;
}
struct Device {
  int fd = -1;
  lab::RawInputState input;
  bool lost = false;
};
static Device devices[8];
static int deviceCount = 0;
static void openInput() {
  for (int i = 0; i < 8; i++) {
    char path[40], name[128] = {};
    snprintf(path, sizeof(path), "/dev/input/event%d", i);
    int fd = open(path, O_RDONLY | O_NONBLOCK);
    if (fd < 0)
      continue;
    ioctl(fd, EVIOCGNAME(sizeof(name)), name);
    if (strcmp(name, "RG35XX Gamepad")) {
      close(fd);
      continue;
    }
    auto &d = devices[deviceCount++];
    d.fd = fd;
    d.input.profile = lab::InputProfile::GarlicGamepad;
    for (int j = 0; j < 2; j++) {
      input_absinfo info = {};
      if (ioctl(fd, EVIOCGABS(j == 0 ? ABS_Z : ABS_RZ), &info) >= 0)
        d.input.triggers[j].configure(info.minimum, info.maximum);
    }
  }
}
static void reconcile() {
  uint32_t held = 0;
  for (int i = 0; i < deviceCount; i++)
    held |= devices[i].input.held();
  for (int b = 0; b < lab::ButtonCount; b++)
    app.button(b, (held & lab::bit(b)) != 0, micros());
}
static void pollInput() {
  for (int i = 0; i < deviceCount; i++) {
    auto &d = devices[i];
    input_event ev;
    while (read(d.fd, &ev, sizeof(ev)) == sizeof(ev)) {
      if (ev.type == EV_SYN && ev.code == SYN_DROPPED) {
        d.lost = true;
        d.input = lab::RawInputState{};
        d.input.profile = lab::InputProfile::GarlicGamepad;
        app.clearInput();
        continue;
      }
      if (d.lost) {
        if (ev.type == EV_SYN && ev.code == SYN_REPORT) {
          d.lost = false;
          unsigned char keys[(KEY_MAX + 8) / 8] = {};
          if (ioctl(d.fd, EVIOCGKEY(sizeof(keys)), keys) >= 0)
            for (int code = 0; code <= KEY_MAX; code++)
              d.input.key(code, (keys[code / 8] >> (code % 8)) & 1);
          const int axes[] = {ABS_HAT0X, ABS_HAT0Y, ABS_Z, ABS_RZ};
          for (int code : axes) {
            input_absinfo info = {};
            if (ioctl(d.fd, EVIOCGABS(code), &info) >= 0)
              d.input.axis(code, info.value);
          }
          reconcile();
        }
        continue;
      }
      if (ev.type == EV_KEY)
        d.input.key(ev.code, ev.value);
      else if (ev.type == EV_ABS)
        d.input.axis(ev.code, ev.value);
      else
        continue;
      reconcile();
    }
  }
}
static void callback(void *, Uint8 *stream, int bytes) {
  if (!audioSchedulingSet) {
    // Only the audio worker is real-time. UI and SD workers stay normal.
    // ALSA blocks between buffers; this worker has bounded DSP and no file I/O.
    sched_param parameters = {};
    parameters.sched_priority = AudioPriority;
    if (sched_setscheduler(0, SCHED_FIFO, &parameters))
      audioSchedulingError = errno;
    audioSchedulingSet = true;
  }
  uint64_t start = micros();
  engine.render((int16_t *)stream, unsigned(bytes) / 4);
  uint64_t finish = micros(), elapsed = finish - start;
  timing.record(start, finish, uint64_t(bytes / 4) * 1000000 / engine.rate);
  callbackHistogram[elapsed / 250 < 200 ? elapsed / 250 : 199]++;
  // Diagnostic renders the full signal graph but keeps the loudspeaker quiet.
  if (profiling)
    memset(stream, 0, unsigned(bytes));
}
static char reportPath[160];
struct ReportRequest {
  unsigned audioFrames;
  bool completed, dirty;
  unsigned bytes;
  int assets;
  uint64_t uiMax;
  unsigned uiSlow;
  unsigned elapsed;
};
static SDL_Thread *reportThread = nullptr;
static uint32_t reportDone = 0;
static ReportRequest pendingReport;
static void writeReport(const ReportRequest &request) {
  unsigned audioFrames = request.audioFrames;
  bool completed = request.completed;
  // Snapshot only under the audio lock. Formatting and SD writes never hold it.
  lab::Timing snapshot;
  uint32_t histogram[200];
  unsigned timingEvents[Tracks], scheduleOverflows;
  unsigned triggers[Tracks], grainPeaks[Tracks], locks[Tracks], repeats[Tracks], mask, instruments, started, finished, hz,
      trackCount, streamUnderruns;
  uint64_t frames;
  SDL_LockAudio();
  snapshot = timing;
  memcpy(histogram, callbackHistogram, sizeof(histogram));
  frames = engine.frame;
  hz = engine.rate;
  trackCount = engine.project.trackCount;
  streamUnderruns = engine.stream.underruns;
  for (int t = 0; t < Tracks; t++) {
    triggers[t] = engine.triggerCount[t];
    locks[t]=engine.lockEventCount[t];repeats[t]=engine.repeatEventCount[t];
    grainPeaks[t] = engine.granular[t].stats.peak;
  }
  scheduleOverflows=engine.scheduleOverflows;
  for(int t=0;t<Tracks;++t) timingEvents[t]=engine.timingEventCount[t];
  mask = engine.seenFxMask;
  instruments = engine.seenInstrumentMask;
  started = engine.capture.startedCount;
  finished = engine.capture.finishedCount;
  SDL_UnlockAudio();
  char temporary[180];
  snprintf(temporary, sizeof(temporary), "%s.tmp", reportPath);
  FILE *f = fopen(temporary, "wb");
  if (!f)
    return;
  unsigned p99 = 0, accumulated = 0;
  for (unsigned i = 0; i < 200; ++i) {
    accumulated += histogram[i];
    if (accumulated >= (snapshot.count * 99 + 99) / 100) {
      p99 = (i + 1) * 250;
      break;
    }
  }
  fprintf(
      f,
      "{\"build\":\"MOSS %s\",\"completed\":%s,\"rate\":%u,\"buffer\":%u,\"callbacks\":%"
      "u,\"callback_max_us\":%llu,\"over_budget\":%u,\"late_intervals\":%u,"
      "\"frames\":%llu,\"triggers\":[%u,%u,%u,%u],\"grain_peaks\":[%u,%u,%u,%u],\"lock_events\":[%u,%u,%u,%u],\"repeat_events\":[%u,%u,%u,%u],\"track_count\":%"
      "u,\"stream_underruns\":%u,\"project_saved\":%s,\"pcm_"
      "bytes\":%u,\"assets\":%d,\"fx_seen_mask\":%u,\"capture_started\":%u,"
      "\"capture_finished\":%u,\"instruments_seen_mask\":%u,\"callback_mean_"
      "us\":%llu,\"reserved_objects_bytes\":%u,\"callback_p99_upper_us\":%u,"
      "\"ui_frame_max_us\":%llu,\"ui_slow_frames\":%u,\"profile_kind\":%d,"
      "\"profile_effect_mask\":%u,\"profile_target_seconds\":%u,\"elapsed_"
      "seconds\":%u,\"audio_realtime_error\":%d,\"audio_priority\":%d,"
      "\"timing_events\":[%u,%u,%u,%u],\"schedule_overflows\":%u,\"profile_passed\":%s}\n",
      Version, completed ? "true" : "false", hz, audioFrames, snapshot.count,
      (unsigned long long)snapshot.maxUs, snapshot.overBudget,
      snapshot.lateIntervals, (unsigned long long)frames, triggers[0],
      triggers[1], triggers[2], triggers[3],grainPeaks[0],grainPeaks[1],grainPeaks[2],grainPeaks[3], locks[0],locks[1],locks[2],locks[3],repeats[0],repeats[1],repeats[2],repeats[3], trackCount, streamUnderruns,
      request.dirty ? "false" : "true", request.bytes, request.assets, mask,
      started, finished, instruments,
      (unsigned long long)(snapshot.count ? snapshot.totalUs / snapshot.count
                                          : 0),
      unsigned(sizeof(engine) + sizeof(library) + sizeof(app) + sizeof(canvas)),
      p99, (unsigned long long)request.uiMax, request.uiSlow,
      profiling ? profileKind : -2, profileMask, profileSeconds,
      request.elapsed, audioSchedulingError,
      audioSchedulingError ? 0 : AudioPriority,
      timingEvents[0],timingEvents[1],timingEvents[2],timingEvents[3],scheduleOverflows,
      profiling && completed && request.elapsed >= profileSeconds &&
              !snapshot.overBudget && !snapshot.lateIntervals &&
              !streamUnderruns && !scheduleOverflows
          ? "true"
          : "false");
  bool ok = !ferror(f);
  if (fflush(f) || fsync(fileno(f)))
    ok = false;
  if (fclose(f))
    ok = false;
  if (ok)
    rename(temporary, reportPath);
}
static int reportWorker(void *) {
  writeReport(pendingReport);
  __atomic_store_n(&reportDone, 1u, __ATOMIC_RELEASE);
  return 0;
}
static void report(unsigned audioFrames, bool completed) {
  if (reportThread) {
    if (!completed && !__atomic_load_n(&reportDone, __ATOMIC_ACQUIRE))
      return;
    SDL_WaitThread(reportThread, nullptr);
    reportThread = nullptr;
  }
  pendingReport = {
      audioFrames,     completed,
      app.dirty,       library.bytes(),
      library.count(), uiFrameMaxUs,
      uiSlowFrames,    unsigned((micros() - profileStart) / 1000000)};
  if (completed)
    writeReport(pendingReport);
  else {
    __atomic_store_n(&reportDone, 0u, __ATOMIC_RELEASE);
    reportThread = SDL_CreateThread(reportWorker, nullptr);
  }
}
int main(int argc, char **argv) {
  profiling = argc == 5 && !strcmp(argv[1], "--profile");
  profileKind = profiling ? atoi(argv[2]) : 0;
  profileMask = profiling ? unsigned(atoi(argv[3])) : 0;
  profileSeconds = profiling ? unsigned(atoi(argv[4])) : 0;
  if (profiling) {
    if (profileKind < -1 || profileKind > 20 || profileMask >= (1u << EffectTypes) ||
        profileSeconds < 10 || profileSeconds > 1800)
      return 2;
    mkdir("perf", 0755);
    mkdir("perf/run", 0755);
    if (chdir("perf/run"))
      return 2;
  }
  mkdir("projects", 0755);
  mkdir("reports", 0755);
  snprintf(reportPath, sizeof(reportPath), "reports/music-%ld-%ld.json",
           (long)time(nullptr), (long)getpid());
  library.init();
  app.library = &library;
  app.captureStore = &captureStore;
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0) {
    fprintf(stderr, "SDL: %s\n", SDL_GetError());
    return 3;
  }
  SDL_Surface *screen = SDL_SetVideoMode(640, 480, 16, SDL_HWSURFACE);
  if (!screen)
    screen = SDL_SetVideoMode(640, 480, 16, SDL_SWSURFACE);
  SDL_Surface *source =
      SDL_CreateRGBSurfaceFrom(canvas.pixels, 640, 480, 32, 640 * 4, 0x00ff0000,
                               0x0000ff00, 0x000000ff, 0);
  if (!screen || !source) {
    fprintf(stderr, "Video: %s\n", SDL_GetError());
    SDL_Quit();
    return 4;
  }
  SDL_AudioSpec want = {}, got = {};
  want.freq = AudioRate;
  want.format = AUDIO_S16SYS;
  want.channels = 2;
  want.samples = AudioBlockFrames;
  want.callback = callback;
  if (SDL_OpenAudio(&want, &got) < 0) {
    fprintf(stderr, "Audio: %s\n", SDL_GetError());
    SDL_Quit();
    return 5;
  }
  if (got.format != AUDIO_S16SYS || got.channels != 2 ||
      got.freq != int(AudioRate) || got.samples != AudioBlockFrames) {
    fprintf(stderr, "Unsupported audio configuration: %d Hz, %u frames\n",
            got.freq, got.samples);
    SDL_CloseAudio();
    SDL_Quit();
    return 6;
  }
  if (profiling) {
    for (auto &sample : library.pcm) {
      stressSample(sample);
      if (profileKind >= 5) sample.rate = 96000;
    }
    if(profileKind>=17) stressSwing(app.project,profileMask,profileKind==18?1:profileKind==19?2:0);
    else if(profileKind>=13) stressEvents(app.project,profileMask,profileKind==14?1:profileKind==15?2:0);
    else if (profileKind >= 5)
      stressGranular(app.project, profileMask, profileKind==6?1:profileKind==8?2:profileKind==9?3:profileKind==10?4:profileKind==11?5:profileKind==12?6:0);
    else
      stressProject(app.project, Tracks, profileKind == 4 ? 2 : profileKind, profileMask);
    app.page = Pattern;
  }
  engine.init(app.project, library.pcm, got.freq, Assets);
  app.engine = &engine;
  if (!profiling)
    app.seedProjects();
  else {
    if (profileKind == 4 || profileKind == 7 || (profileKind==16 || profileKind==20)) {
      void *memory = malloc(sizeof(SongBank));
      if (!memory)
        return 7;
      app.songPlayback = new (memory) SongBank;
      auto &b = *app.songPlayback;
      b.song.count = SongSlots;
      b.song.bpm = 240;
      b.song.loop = 1;
      b.count = Projects;
      strcpy(b.song.name, "TRANSITION STRESS");
      const unsigned masks[] = {3,5,9,17,6,10,18,12,20,24};
      for (int i = 0; i < Projects; ++i) {
        b.map[i] = i;
        if(profileKind==20) stressSwing(b.sequences[i],masks[i%10],i%3);
        else if(profileKind==16) stressEvents(b.sequences[i],masks[i%10],i%3);
        else if(profileKind==7) {
          stressGranular(b.sequences[i],masks[i%10],i%2);
          if(i%3==0) b.sequences[i].tracks[3].instrument=SampleInstrument;
        } else
          stressProject(b.sequences[i], Tracks, i % 2 ? 2 : (i / 2) % 4, masks[i % 10]);
        snprintf(b.sequences[i].name, 25, "MAX LOAD %02d", i + 1);
        app.saved[i] = true;
        snprintf(app.projectNames[i], 25, "MAX LOAD %02d", i + 1);
      }
      for (int i = 0; i < SongSlots; ++i)
        b.song.slots[i] = {i % Projects, 1, 1};
      app.song = b.song;
      app.page = SongPage;
      engine.startSong(b, 0);
    } else
      engine.handle({Op::Transport});
    engine.handle({Op::CaptureStart, 0, 0, 0, 4});
  }
  openInput();
  SDL_ShowCursor(SDL_DISABLE);
  SDL_PauseAudio(0);
  uint64_t start = micros();
  profileStart = start;
  uint64_t nextReport = start + 5000000;
  while (!app.exit) {
    uint64_t before = micros();
    if (!profiling)
      pollInput();
    SDL_Event ev;
    while (SDL_PollEvent(&ev))
      if (ev.type == SDL_QUIT) {
        if (!app.captureBusy() && (!app.dirty || app.save()) &&
            (!app.songDirty || app.saveArrangement()))
          app.exit = true;
      }
    app.tick(micros());
    if (profiling) {
      unsigned seconds = unsigned((micros() - start) / 1000000);
      const Page pages[] = {Pattern, Sampler, Space, Mix, Grid, Tape};
      app.page = (profileKind == 4 || profileKind == 7 || (profileKind==16 || profileKind==20)) ? SongPage : pages[(seconds / 2) % 6];
      if (profileKind==11 || profileKind==13 || profileKind==17) app.page=Sampler; // Keep the full live lens visible.
      if (profileKind == 4 || profileKind == 7 || (profileKind==16 || profileKind==20))
        app.songRow = int(seconds % SongSlots);
      app.track = int((seconds / 3) % Tracks);
      app.bank = int((seconds / 2) % 4);
      app.synthBank = int((seconds / 2) % app.voiceBanks());
      app.fxDetail = (seconds / 3) % 2;
      app.step = int(seconds * 7 % Steps);
      // Repeated capture writeback and real UI command dispatch under dense DSP load.
      if (profileKind>=5 && !app.captureBusy() && seconds % 10 == 0)
        app.capturePending=app.send({Op::CaptureStart,0,0,0,4});
      if ((profileKind==9 || profileKind==12) && app.granularTrack())
        app.change({Op::GranularFreeze,app.track,0,0,int(seconds%2)});
      if (seconds >= profileSeconds)
        app.exit = true;
    }
    app.pumpPreview();
    app.flushCapture();
    if (app.requestedSlot >= 0 || app.reload || app.job != App::NoJob) {
      SDL_PauseAudio(1);
      SDL_LockAudio(); // reporter snapshots the same engine under this lock
      app.streamReader.close();
      if (app.job != App::NoJob)
        app.service();
      else if (app.requestedSlot >= 0) {
        app.open(app.requestedSlot);
        app.requestedSlot = -1;
      } else
        engine.init(app.project, library.pcm, got.freq, Assets);
      app.reload = false;
      SDL_UnlockAudio();
      SDL_PauseAudio(0);
    }
    draw(canvas, app);
    SDL_BlitSurface(source, nullptr, screen, nullptr);
    SDL_Flip(screen);
    if (micros() >= nextReport) {
      report(got.samples, false);
      nextReport = micros() + 15000000;
    }
    if (!deviceCount && micros() - start > 60000000)
      app.exit = true;
    uint64_t elapsed = micros() - before;
    if (elapsed > uiFrameMaxUs)
      uiFrameMaxUs = elapsed;
    if (elapsed > 33333)
      ++uiSlowFrames;
    if (elapsed < 33333)
      SDL_Delay(unsigned((33333 - elapsed) / 1000));
  }
  SDL_PauseAudio(1);
  report(got.samples, true);
  SDL_CloseAudio();
  captureStore.join();
  for (int i = 0; i < deviceCount; i++)
    close(devices[i].fd);
  SDL_FreeSurface(source);
  SDL_Quit();
  return 0;
}
