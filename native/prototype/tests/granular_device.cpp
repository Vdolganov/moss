#include "granular_harness.h"
#include "../src/capture_store.h"
#include "../../../hardware lab/src/lab_ui.h"
#include <SDL/SDL.h>
#include <sched.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
using namespace moss;
static GranularHarness harness;
static CaptureStore writer;
static lab::Canvas canvas;
static lab::Timing timing;
static volatile unsigned char songMemory[sizeof(SongBank)];
static unsigned histogram[256] = {};
static uint32_t shownFrames = 0, requestCapture = 0;
static bool prioritySet = false;
static int priorityError = 0;
static uint64_t nowUs() {
  timespec t = {};
  clock_gettime(CLOCK_MONOTONIC, &t);
  return uint64_t(t.tv_sec) * 1000000 + t.tv_nsec / 1000;
}
static void callback(void *, Uint8 *buffer, int bytes) {
  if (!prioritySet) {
    sched_param p = {};
    p.sched_priority = 10;
    priorityError = sched_setscheduler(0, SCHED_FIFO, &p) ? errno : 0;
    prioritySet = true;
  }
  unsigned frames = unsigned(bytes) / 4;
  uint64_t begin = nowUs();
  if (__atomic_exchange_n(&requestCapture, 0u, __ATOMIC_ACQ_REL))
    harness.capture.begin(AudioRate, AudioRate * 8, -1);
  harness.render(reinterpret_cast<int16_t *>(buffer), frames);
  uint64_t finish = nowUs();
  if (harness.frame >= AudioRate * 2) {
    timing.record(begin, finish, uint64_t(frames) * 1000000 / AudioRate);
    unsigned bucket = unsigned((finish - begin) / 250);
    ++histogram[bucket < 256 ? bucket : 255];
  }
  // The DSP and real audio backend still run; stress noise is not sent to the speaker.
  memset(buffer, 0, unsigned(bytes));
  __atomic_store_n(&shownFrames, uint32_t(harness.frame), __ATOMIC_RELEASE);
}
int main(int argc, char **argv) {
  if (argc != 6)
    return 2;
  unsigned cap = unsigned(atoi(argv[1])), mask = unsigned(atoi(argv[2]));
  unsigned mode = unsigned(atoi(argv[3])), seconds = unsigned(atoi(argv[4]));
  unsigned secondBit = mask & (mask - 1);
  if (!cap || cap > 32 || mask > 15 || (secondBit & (secondBit - 1)) || mode > 3 || seconds < 10 ||
      seconds > 1800)
    return 2;
  const char *report = argv[5];
  harness.init(4, cap, mask, mode);
  for (unsigned i = 0; i < sizeof(songMemory); ++i)
    songMemory[i] = unsigned(i);
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0) {
    fprintf(stderr, "SDL init: %s\n", SDL_GetError());
    return 3;
  }
  SDL_Surface *screen = SDL_SetVideoMode(640, 480, 16, SDL_HWSURFACE);
  if (!screen)
    screen = SDL_SetVideoMode(640, 480, 16, SDL_SWSURFACE);
  SDL_Surface *source = SDL_CreateRGBSurfaceFrom(canvas.pixels, 640, 480, 32, 640 * 4, 0x00ff0000,
                                                 0x0000ff00, 0x000000ff, 0);
  if (!screen || !source) {
    fprintf(stderr, "SDL video: %s\n", SDL_GetError());
    return 4;
  }
  SDL_AudioSpec want = {}, got = {};
  want.freq = AudioRate;
  want.format = AUDIO_S16SYS;
  want.channels = 2;
  want.samples = AudioBlockFrames;
  want.callback = callback;
  if (SDL_OpenAudio(&want, &got) || got.freq != int(AudioRate) || got.format != AUDIO_S16SYS ||
      got.channels != 2 || got.samples != AudioBlockFrames) {
    fprintf(stderr, "SDL audio: %s\n", SDL_GetError());
    return 5;
  }
  unsigned captureSaved = 0, captureErrors = 0, nextCapture = AudioRate * 16;
  uint64_t uiMax = 0;
  unsigned uiSlow = 0, uiFrames = 0;
  uint64_t start = nowUs();
  SDL_ShowCursor(SDL_DISABLE);
  SDL_PauseAudio(0);
  bool quit = false;
  while (__atomic_load_n(&shownFrames, __ATOMIC_ACQUIRE) < (seconds + 2) * AudioRate &&
         nowUs() - start < uint64_t(seconds + 15) * 1000000 && !quit) {
    uint64_t begin = nowUs();
    unsigned current = __atomic_load_n(&shownFrames, __ATOMIC_ACQUIRE);
    unsigned result = writer.poll();
    if (result == 2)
      ++captureSaved;
    if (result == 3)
      ++captureErrors;
    if (!writer.thread && __atomic_load_n(&harness.capture.state, __ATOMIC_ACQUIRE) == 2) {
      if (captureSaved + captureErrors < harness.capture.finishedCount) {
        writer.start("lab-capture.wav", harness.capture.pcm, harness.capture.frames, AudioRate);
      } else if (current >= nextCapture) {
        nextCapture = current + AudioRate * 16;
        __atomic_store_n(&requestCapture, 1u, __ATOMIC_RELEASE);
      }
    }
    SDL_Event e;
    while (SDL_PollEvent(&e))
      if (e.type == SDL_QUIT)
        quit = true;
    canvas.clear(lab::BG);
    lab::text(canvas, 18, 18, "MOSS / GRANULAR LAB", lab::TEXT, 3);
    lab::text(canvas, 18, 62, "AUTOMATIC HARDWARE TEST / MUTED", lab::DIM);
    char line[100];
    snprintf(line, sizeof(line), "4 TRACKS / %u GRAINS EACH / FX %u", cap, mask);
    lab::text(canvas, 18, 110, line);
    snprintf(line, sizeof(line), "%u / %u SECONDS", current / AudioRate, seconds + 2);
    lab::text(canvas, 18, 150, line);
    for (unsigned t = 0; t < 4; ++t) {
      unsigned step = (current / 500) % (16 - t);
      for (unsigned s = 0; s < 16; ++s) {
        canvas.box(18 + s * 38, 208 + t * 42, 30, 32, lab::LINE);
        if (s == step)
          canvas.rect(19 + s * 38, 209 + t * 42, 28, 30, lab::TEXT);
      }
    }
    snprintf(line, sizeof(line), "BACKGROUND SAVES %u / ERRORS %u", captureSaved, captureErrors);
    lab::text(canvas, 18, 400, line, lab::DIM);
    lab::text(canvas, 18, 447, "RETURNS TO GARLIC WHEN FINISHED", lab::DIM);
    SDL_BlitSurface(source, nullptr, screen, nullptr);
    SDL_Flip(screen);
    uint64_t elapsed = nowUs() - begin;
    if (elapsed > uiMax)
      uiMax = elapsed;
    if (elapsed > 33333)
      ++uiSlow;
    ++uiFrames;
    if (elapsed < 33333)
      SDL_Delay(unsigned((33333 - elapsed) / 1000));
  }
  SDL_PauseAudio(1);
  if (writer.thread) {
    writer.join();
    unsigned result = writer.poll();
    if (result == 2)
      ++captureSaved;
    if (result == 3)
      ++captureErrors;
  }
  unsigned p99 = 0, accumulated = 0;
  for (unsigned i = 0; i < 256; ++i) {
    accumulated += histogram[i];
    if (accumulated >= (timing.count * 99 + 99) / 100) {
      p99 = (i + 1) * 250;
      break;
    }
  }
  unsigned peak = 0;
  uint64_t skipped = 0, spawned = 0;
  for (auto &v : harness.voices) {
    if (v.stats.peak > peak)
      peak = v.stats.peak;
    skipped += v.stats.skipped;
    spawned += v.stats.spawned;
  }
  bool completed = shownFrames >= (seconds + 2) * AudioRate;
  bool passed = completed && !timing.overBudget && !timing.lateIntervals && !captureErrors &&
                captureSaved && !priorityError && !uiSlow;
  FILE *f = fopen(report, "w");
  if (!f)
    return 6;
  fprintf(f,
          "{\"experiment\":\"granular-v1\",\"tracks\":4,\"capacity\":%u,\"fx_mask\":%u,"
          "\"mode\":%u,\"target_seconds\":%u,\"frames\":%u,\"rate\":32000,\"buffer\":1024,"
          "\"callbacks\":%u,\"mean_us\":%llu,\"max_us\":%llu,\"p99_upper_us\":%u,"
          "\"over_budget\":%u,\"late_intervals\":%u,\"max_gap_us\":%llu,"
          "\"ui_max_us\":%llu,\"ui_slow_frames\":%u,\"ui_frames\":%u,"
          "\"capture_saved\":%u,\"capture_errors\":%u,\"priority_error\":%d,"
          "\"peak_grains_per_track\":%u,\"spawned\":%llu,\"skipped\":%llu,"
          "\"output_peak\":%d,\"reserved_bytes\":%u,\"completed\":%s,\"passed\":%s,"
          "\"application_integration_pending\":true}\n",
          cap, mask, mode, seconds, shownFrames, timing.count,
          (unsigned long long)(timing.count ? timing.totalUs / timing.count : 0),
          (unsigned long long)timing.maxUs, p99, timing.overBudget, timing.lateIntervals,
          (unsigned long long)timing.maxGapUs, (unsigned long long)uiMax, uiSlow, uiFrames,
          captureSaved, captureErrors, priorityError, peak, (unsigned long long)spawned,
          (unsigned long long)skipped, harness.peak,
          unsigned(sizeof(harness) + sizeof(songMemory) + sizeof(canvas) + sizeof(writer)),
          completed ? "true" : "false", passed ? "true" : "false");
  bool writeOk = !ferror(f);
  if (fclose(f))
    writeOk = false;
  SDL_CloseAudio();
  SDL_FreeSurface(source);
  SDL_Quit();
  return passed && writeOk ? 0 : 1;
}
