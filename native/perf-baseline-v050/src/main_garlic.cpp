#include "../../../hardware lab/src/lab_input.h"
#include "app.h"
#include <SDL/SDL.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
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
static lab::Canvas canvas;
static lab::Timing timing;
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
  uint64_t start = micros();
  engine.render((int16_t *)stream, unsigned(bytes) / 4);
  timing.record(start, micros(), uint64_t(bytes / 4) * 1000000 / engine.rate);
}
static char reportPath[160];
static void report(unsigned audioFrames, bool completed) {
  // Snapshot only under the audio lock. Formatting and SD writes never hold it.
  lab::Timing snapshot;
  unsigned triggers[Tracks], mask, instruments, started, finished, hz,
      trackCount, streamUnderruns;
  uint64_t frames;
  SDL_LockAudio();
  snapshot = timing;
  frames = engine.frame;
  hz = engine.rate;
  trackCount = engine.project.trackCount;
  streamUnderruns = engine.stream.underruns;
  for (int t = 0; t < Tracks; t++)
    triggers[t] = engine.triggerCount[t];
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
  fprintf(
      f,
      "{\"build\":\"MOSS "
      "0.5.0\",\"completed\":%s,\"rate\":%u,\"buffer\":%u,\"callbacks\":%"
      "u,\"callback_max_us\":%llu,\"over_budget\":%u,\"late_intervals\":%u,"
      "\"frames\":%llu,\"triggers\":[%u,%u,%u,%u,%u,%u,%u,%u],\"track_count\":%"
      "u,\"stream_underruns\":%u,\"project_saved\":%s,\"pcm_"
      "bytes\":%u,\"assets\":%d,\"fx_seen_mask\":%u,\"capture_started\":%u,"
      "\"capture_finished\":%u,\"instruments_seen_mask\":%u,\"callback_mean_"
      "us\":%llu,\"reserved_objects_bytes\":%u}\n",
      completed ? "true" : "false", hz, audioFrames, snapshot.count,
      (unsigned long long)snapshot.maxUs, snapshot.overBudget,
      snapshot.lateIntervals, (unsigned long long)frames, triggers[0],
      triggers[1], triggers[2], triggers[3], triggers[4], triggers[5],
      triggers[6], triggers[7], trackCount, streamUnderruns,
      app.dirty ? "false" : "true", library.bytes(), library.count(), mask,
      started, finished, instruments,
      (unsigned long long)(snapshot.count ? snapshot.totalUs / snapshot.count
                                          : 0),
      unsigned(sizeof(engine) + sizeof(library) + sizeof(app) +
               sizeof(canvas)));
  bool ok = !ferror(f);
  if (fflush(f) || fsync(fileno(f)))
    ok = false;
  if (fclose(f))
    ok = false;
  if (ok)
    rename(temporary, reportPath);
}
int main() {
  mkdir("projects", 0755);
  mkdir("reports", 0755);
  snprintf(reportPath, sizeof(reportPath), "reports/music-%ld-%ld.json",
           (long)time(nullptr), (long)getpid());
  library.init();
  app.library = &library;
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
  want.freq = 44100;
  want.format = AUDIO_S16SYS;
  want.channels = 2;
  want.samples = 1024;
  want.callback = callback;
  if (SDL_OpenAudio(&want, &got) < 0) {
    fprintf(stderr, "Audio: %s\n", SDL_GetError());
    SDL_Quit();
    return 5;
  }
  if (got.format != AUDIO_S16SYS || got.channels != 2 || got.freq < 8000 ||
      got.freq > 96000) {
    SDL_CloseAudio();
    SDL_Quit();
    return 6;
  }
  engine.init(app.project, library.pcm, got.freq, Assets);
  app.engine = &engine;
  app.seedProjects();
  openInput();
  SDL_ShowCursor(SDL_DISABLE);
  SDL_PauseAudio(0);
  uint64_t start = micros();
  uint64_t nextReport = start + 5000000;
  while (!app.exit) {
    uint64_t before = micros();
    pollInput();
    SDL_Event ev;
    while (SDL_PollEvent(&ev))
      if (ev.type == SDL_QUIT) {
        if (!app.dirty || app.save())
          app.exit = true;
      }
    app.tick(micros());
    app.pumpPreview();
    app.flushCapture();
    if (app.requestedSlot >= 0 || app.reload || app.job != App::NoJob) {
      SDL_PauseAudio(1);
      app.streamReader.close();
      if (app.job != App::NoJob)
        app.service();
      else if (app.requestedSlot >= 0) {
        app.open(app.requestedSlot);
        app.requestedSlot = -1;
      } else
        engine.init(app.project, library.pcm, got.freq, Assets);
      app.reload = false;
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
    if (elapsed < 33333)
      SDL_Delay(unsigned((33333 - elapsed) / 1000));
  }
  SDL_PauseAudio(1);
  report(got.samples, true);
  SDL_CloseAudio();
  for (int i = 0; i < deviceCount; i++)
    close(devices[i].fd);
  SDL_FreeSurface(source);
  SDL_Quit();
  return 0;
}
