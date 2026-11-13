#include "lab_ui.h"
#include "lab_input.h"
#include "lab_sample.h"
#include <SDL/SDL.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>

using namespace lab;
static_assert(sizeof(void *) == 4, "LAB requires the original RG35XX 32-bit ABI");
static_assert(sizeof(input_event) == 16, "evdev layout must match 32-bit Linux");
#ifndef __UCLIBC__
#error "Build against the pinned Garlic-compatible uClibc SDK headers"
#endif
static View view;
static Canvas canvas;
static char reportPath[512], reportDir[384], fatalError[256], storageError[160];
static bool completed = false, storageChecked = false, storageOK = false, videoOK = false;
static uint64_t bootUs = 0, storageWriteUs = 0, storageReadUs = 0;
static unsigned droppedEvents = 0, unknownEvents = 0, unknownAxes = 0, rawEvents = 0,
                flipErrors = 0;
static uint64_t freeBytes = 0;
static unsigned parameterAttempts[4] = {}, parameterChanges[4] = {};
static uint64_t micros() {
  timespec t = {};
  if (clock_gettime(CLOCK_MONOTONIC, &t))
    return uint64_t(SDL_GetTicks()) * 1000;
  return uint64_t(t.tv_sec) * 1000000 + t.tv_nsec / 1000;
}
struct Device {
  int fd = -1;
  char path[32] = {}, name[128] = {};
  RawInputState input;
  int readError = 0;
  bool syncLost = false;
};
static Device devices[8];
static int deviceCount = 0;
static int mappedDevices = 0;
struct Raw {
  unsigned ms;
  int device, type, code, value;
};
static Raw rawLog[512];
static unsigned logCount = 0;
struct AudioState {
  Tone tone;
  SamplePlayer player;
  bool sampleMode = false;
  Timing timing;
  int rate = 44100, frames = 1024;
};
static AudioState audio;
static SampleBuffer sample;
struct Run {
  int rate, frames, hz, mode, actualRate, actualFrames;
  bool opened;
  bool sampleMode;
  int sampleIndex, pitch, trimStart, trimEnd, sampleModeId;
  unsigned sourceRate, sourceChannels, sourceFrames, pcmBytes, voices, wraps;
  uint64_t loadUs;
  char error[160], verdict[32];
  Timing timing;
  unsigned playedFrames;
  unsigned wallMs;
  int sdlAudioStatus;
  char endReason[24];
};
static Run runs[32];
static int runCount = 0;
static bool audioOpen = false;
static uint64_t audioStarted = 0;
static long memField(const char *path, const char *key) {
  FILE *f = fopen(path, "r");
  if (!f)
    return -1;
  char line[256];
  long value = -1;
  size_t n = strlen(key);
  while (fgets(line, sizeof(line), f))
    if (!strncmp(line, key, n)) {
      value = strtol(line + n, nullptr, 10);
      break;
    }
  fclose(f);
  return value;
}
static void memory() {
  view.totalKb = memField("/proc/meminfo", "MemTotal:");
  view.availableKb = memField("/proc/meminfo", "MemAvailable:");
  view.rssKb = memField("/proc/self/status", "VmRSS:");
}
static void quoted(FILE *f, const char *s) {
  fputc('"', f);
  for (; *s; s++) {
    unsigned char c = *s;
    if (c == '"' || c == '\\') {
      fputc('\\', f);
      fputc(c, f);
    } else if (c < 32)
      fprintf(f, "\\u%04x", c);
    else
      fputc(c, f);
  }
  fputc('"', f);
}
static void keyString(FILE *f, const char *k, const char *v, bool comma = true) {
  quoted(f, k);
  fputc(':', f);
  quoted(f, v);
  if (comma)
    fputc(',', f);
  fputc('\n', f);
}
static bool saveReport() {
  memory();
  char tmp[540];
  snprintf(tmp, sizeof(tmp), "%s.tmp", reportPath);
  FILE *f = fopen(tmp, "wb");
  if (!f) {
    snprintf(view.reportStatus, sizeof(view.reportStatus), "REPORT WRITE FAILED: %d", errno);
    return false;
  }
  fprintf(f,
          "{\n\"schema\":4,\"build\":\"MOSS LAB 0.1.3\",\"target_os\":\"GarlicOS "
          "1.3.9\",\"hardware_test\":true,\"completed\":%s,\"uptime_ms\":%llu,\n",
          completed ? "true" : "false", (unsigned long long)((micros() - bootUs) / 1000));
  keyString(f, "fatal_error", fatalError);
  keyString(f, "machine", view.machine);
  keyString(f, "kernel", view.kernel);
  keyString(f, "input_backend", view.inputBackend);
  fprintf(f, "\"memory_kib\":{\"total\":%ld,\"available\":%ld,\"rss\":%ld},\n", view.totalKb,
          view.availableKb, view.rssKb);
  fprintf(f,
          "\"video\":{\"opened\":%s,\"width\":640,\"height\":480,\"bpp\":%d,\"present_fps\":%.3f,"
          "\"flip_errors\":%u,\"driver\":",
          videoOK ? "true" : "false", view.bpp, view.fps, flipErrors);
  quoted(f, view.videoDriver);
  fprintf(f, "},\n");
  fprintf(f,
          "\"input\":{\"seen_mask\":%u,\"released_mask\":%u,\"held_mask\":%u,\"buttons_completed\":"
          "%d,\"chords_mask\":%u,\"chords_completed\":%u,\"l1_r1\":%s,\"raw_event_count\":%u,"
          "\"unknown_key_events\":%u,\"unknown_axis_events\":%u,\"syn_dropped\":%u,\"presses\":[",
          view.buttons.seen, view.buttons.released, view.buttons.held, view.buttons.completed(),
          view.buttons.chords, popcount(view.buttons.chords),
          view.buttons.shoulders ? "true" : "false", rawEvents, unknownEvents, unknownAxes,
          droppedEvents);
  for (int i = 0; i < ButtonCount; i++)
    fprintf(f, "%s%u", i ? "," : "", view.buttons.presses[i]);
  fprintf(f, "],\"max_hold_ms\":[");
  for (int i = 0; i < ButtonCount; i++)
    fprintf(f, "%s%u", i ? "," : "", view.holds.maxMs[i]);
  fprintf(f, "],\"long_holds_500ms\":[");
  for (int i = 0; i < ButtonCount; i++)
    fprintf(f, "%s%u", i ? "," : "", view.holds.longHolds[i]);
  fprintf(f, "],\"parameter_attempts\":[");
  for (int i = 0; i < 4; i++)
    fprintf(f, "%s%u", i ? "," : "", parameterAttempts[i]);
  fprintf(f, "],\"parameter_changes\":[");
  for (int i = 0; i < 4; i++)
    fprintf(f, "%s%u", i ? "," : "", parameterChanges[i]);
  fprintf(f, "],\"input_values\":[%d,%d,%d,%d", view.values[0], view.values[1], view.values[2],
          view.values[3]);
  fprintf(f, "],\"devices\":[");
  for (int i = 0; i < deviceCount; i++) {
    if (i)
      fputc(',', f);
    fprintf(f, "{\"path\":");
    quoted(f, devices[i].path);
    fprintf(f, ",\"name\":");
    quoted(f, devices[i].name);
    fprintf(f, ",\"profile\":");
    quoted(f, profileName(devices[i].input.profile));
    fprintf(f, ",\"read_errno\":%d,\"triggers\":[", devices[i].readError);
    for (int j = 0; j < 2; j++) {
      const TriggerRange &range = devices[i].input.triggers[j];
      fprintf(f, "%s{\"code\":%d,\"known\":%s,\"min\":%d,\"max\":%d}", j ? "," : "", j == 0 ? 2 : 5,
              range.known ? "true" : "false", range.minimum, range.maximum);
    }
    fputc(']', f);
    fputc('}', f);
  }
  fprintf(f, "],\"last_events\":[");
  unsigned count = logCount > 512 ? 512 : logCount;
  for (unsigned i = 0; i < count; i++) {
    Raw &r = rawLog[(logCount - count + i) % 512];
    fprintf(f, "%s[%u,%d,%d,%d,%d]", i ? "," : "", r.ms, r.device, r.type, r.code, r.value);
  }
  fprintf(f, "]},\n");
  fprintf(f, "\"audio_runs\":[");
  for (int i = 0; i < runCount; i++) {
    Run &r = runs[i];
    if (i)
      fputc(',', f);
    fprintf(f,
            "{\"requested_rate\":%d,\"requested_frames\":%d,\"tone_hz\":%d,\"channel_mode\":%d,"
            "\"opened\":%s,\"actual_rate\":%d,\"actual_frames\":%d,\"played_frames\":%u,"
            "\"callbacks\":%u,\"callback_max_us\":%llu,\"callback_total_us\":%llu,\"over_budget\":%"
            "u,\"late_intervals\":%u,\"max_interval_us\":%llu,\"error\":",
            r.rate, r.frames, r.hz, r.mode, r.opened ? "true" : "false", r.actualRate,
            r.actualFrames, r.playedFrames, r.timing.count, (unsigned long long)r.timing.maxUs,
            (unsigned long long)r.timing.totalUs, r.timing.overBudget, r.timing.lateIntervals,
            (unsigned long long)r.timing.maxGapUs);
    quoted(f, r.error);
    fprintf(f, ",\"listening_verdict\":");
    quoted(f, r.verdict);
    fprintf(f, ",\"wall_ms\":%u,\"sdl_audio_status\":%d,\"end_reason\":", r.wallMs,
            r.sdlAudioStatus);
    quoted(f, r.endReason);
    fprintf(f,
            ",\"sample\":{\"enabled\":%s,\"index\":%d,\"source_rate\":%u,\"channels\":%u,"
            "\"frames\":%u,\"pcm_bytes\":%u,\"load_us\":%llu,\"pitch\":%d,\"start_percent\":%d,"
            "\"end_percent\":%d,\"mode\":%d,\"voices\":%u,\"wraps\":%u}",
            r.sampleMode ? "true" : "false", r.sampleIndex, r.sourceRate, r.sourceChannels,
            r.sourceFrames, r.pcmBytes, (unsigned long long)r.loadUs, r.pitch, r.trimStart,
            r.trimEnd, r.sampleModeId, r.voices, r.wraps);
    fputc('}', f);
  }
  fprintf(f, "],\n");
  keyString(f, "audio_driver", view.audioDriver);
  keyString(f, "timing_note",
            "Callback timing is not physical output latency or an ALSA xrun counter.");
  fprintf(f,
          "\"storage\":{\"tested\":%s,\"verified\":%s,\"bytes\":262144,\"free_bytes\":%llu,\"write_"
          "fsync_us\":%llu,\"read_verify_us\":%llu,\"error\":",
          storageChecked ? "true" : "false", storageOK ? "true" : "false",
          (unsigned long long)freeBytes, (unsigned long long)storageWriteUs,
          (unsigned long long)storageReadUs);
  quoted(f, storageError);
  fprintf(f, "}\n}\n");
  bool ok = !ferror(f);
  if (fflush(f))
    ok = false;
  if (fsync(fileno(f)))
    ok = false;
  if (fclose(f))
    ok = false;
  if (ok && rename(tmp, reportPath) == 0) {
    snprintf(view.reportStatus, sizeof(view.reportStatus),
             "REPORT SAVED / L2 R2 TABS / MENU HOLD EXIT");
    return true;
  }
  snprintf(view.reportStatus, sizeof(view.reportStatus), "REPORT SAVE FAILED: %d", errno);
  return false;
}
static void storageTest() {
  storageChecked = true;
  storageOK = false;
  storageError[0] = 0;
  char path[512];
  snprintf(path, sizeof(path), "%s/storage-%ld.tmp", reportDir, (long)getpid());
  int fd = open(path, O_CREAT | O_EXCL | O_RDWR, 0600);
  if (fd < 0) {
    snprintf(storageError, sizeof(storageError), "open: %s", strerror(errno));
    snprintf(view.writeStatus, sizeof(view.writeStatus), "SD TEST: COULD NOT CREATE FILE");
    saveReport();
    return;
  }
  unsigned char block[4096];
  for (unsigned i = 0; i < sizeof(block); i++)
    block[i] = (i * 37 + 11) & 255;
  uint64_t start = micros();
  bool ok = true;
  for (int i = 0; i < 64 && ok; i++)
    if (write(fd, block, sizeof(block)) != (ssize_t)sizeof(block))
      ok = false;
  if (fsync(fd))
    ok = false;
  storageWriteUs = micros() - start;
  if (lseek(fd, 0, SEEK_SET) < 0)
    ok = false;
  start = micros();
  unsigned char readback[4096];
  for (int i = 0; i < 64 && ok; i++)
    if (read(fd, readback, sizeof(readback)) != (ssize_t)sizeof(readback) ||
        memcmp(block, readback, sizeof(block)))
      ok = false;
  storageReadUs = micros() - start;
  int savedErrno = errno;
  if (close(fd))
    ok = false;
  if (unlink(path))
    ok = false;
  storageOK = ok;
  if (!ok)
    snprintf(storageError, sizeof(storageError), "write/read/cleanup failed: %s",
             strerror(savedErrno));
  snprintf(view.writeStatus, sizeof(view.writeStatus), "SD 256 KIB: %s / WRITE %llu MS",
           ok ? "VERIFIED" : "FAILED", (unsigned long long)(storageWriteUs / 1000));
  saveReport();
}
static void callback(void *userdata, Uint8 *stream, int bytes) {
  AudioState *a = (AudioState *)userdata;
  uint64_t start = micros();
  memset(stream, 0, bytes);
  if (a->sampleMode)
    a->player.render((int16_t *)stream, unsigned(bytes) / 4);
  else
    a->tone.render((int16_t *)stream, unsigned(bytes) / 4);
  a->timing.record(start, micros(), uint64_t(bytes / 4) * 1000000 / a->rate);
}
static void audioSnapshot() {
  if (!audioOpen)
    return;
  SDL_LockAudio();
  Run &r = runs[runCount - 1];
  r.timing = audio.timing;
  r.playedFrames = audio.sampleMode ? audio.player.position : audio.tone.position;
  if (audio.sampleMode) {
    r.wraps = audio.player.wraps;
    view.samplePosition = unsigned(audio.player.phase[0] >> 32);
  }
  r.sdlAudioStatus = int(SDL_GetAudioStatus());
  r.wallMs = unsigned(elapsedUs(micros(), audioStarted) / 1000);
  view.callbacks = audio.timing.count;
  view.callbackMaxUs = audio.timing.maxUs;
  view.late = audio.timing.lateIntervals;
  view.elapsedMs = unsigned(uint64_t(r.playedFrames) * 1000 / audio.rate);
  SDL_UnlockAudio();
}
static void closeAudio() {
  if (!audioOpen)
    return;
  int stateBeforeClose = int(SDL_GetAudioStatus());
  SDL_PauseAudio(1);
  audioSnapshot();
  runs[runCount - 1].sdlAudioStatus = stateBeforeClose;
  SDL_CloseAudio();
  audioOpen = false;
  view.audioRunning = false;
  const Run &r = runs[runCount - 1];
  snprintf(view.audioStatus, sizeof(view.audioStatus), "%s",
           r.error[0]       ? "AUDIO ERROR / SEE REPORT"
           : r.timing.count ? "FINISHED / TAP A B X TO RATE SOUND"
                            : "NO AUDIO CALLBACK / SEE REPORT");
  saveReport();
}
static void stopAudio() {
  if (!audioOpen)
    return;
  SDL_LockAudio();
  if (audio.sampleMode)
    audio.player.stop();
  else
    audio.tone.stop();
  SDL_UnlockAudio();
  strcpy(runs[runCount - 1].endReason, "stop requested");
}
static void startAudio() {
  if (audioOpen) {
    stopAudio();
    return;
  }
  if (runCount == 32) {
    snprintf(view.audioStatus, sizeof(view.audioStatus), "32 RUN LIMIT / REOPEN LAB FOR MORE");
    return;
  }
  Run &r = runs[runCount++];
  r = Run{};
  r.rate = view.rate;
  r.frames = view.buffer;
  r.mode = view.mode;
  r.hz = view.hz;
  strcpy(r.verdict, "NOT LISTENED");
  strcpy(view.verdict, r.verdict);
  audio.timing = Timing{};
  view.callbacks = 0;
  view.callbackMaxUs = 0;
  view.late = 0;
  view.elapsedMs = 0;
  view.audioOpened = false;
  r.sampleMode = audio.sampleMode = view.page == 4;
  if (r.sampleMode) {
    r.sampleIndex = view.sampleIndex;
    r.pitch = view.samplePitch;
    r.trimStart = view.sampleStart;
    r.trimEnd = view.sampleEnd;
    r.sampleModeId = view.sampleMode;
    uint64_t loadStart = micros();
    bool loaded = sample.load(sampleFiles[view.sampleIndex]);
    r.loadUs = micros() - loadStart;
    view.sampleLoaded = loaded;
    if (!loaded) {
      snprintf(r.error, sizeof(r.error), "%s", sample.error);
      strcpy(view.audioStatus, "WAV LOAD FAILED / SEE REPORT");
      saveReport();
      return;
    }
    r.sourceRate = view.sampleRate = sample.rate;
    r.sourceChannels = view.sampleChannels = sample.channels;
    r.sourceFrames = view.sampleFrames = sample.frames;
    r.pcmBytes = view.sampleBytes = sample.bytes;
    view.sampleLoadMs = unsigned(r.loadUs / 1000);
    memcpy(view.samplePeaks, sample.peaks, sizeof(sample.peaks));
  }
  if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
    snprintf(r.error, sizeof(r.error), "audio init: %s", SDL_GetError());
    snprintf(view.audioStatus, sizeof(view.audioStatus), "AUDIO INIT FAILED / SEE REPORT");
    saveReport();
    return;
  }
  SDL_AudioDriverName(view.audioDriver, sizeof(view.audioDriver));
  SDL_AudioSpec want = {}, got = {};
  want.freq = view.rate;
  want.format = AUDIO_S16SYS;
  want.channels = 2;
  want.samples = Uint16(view.buffer);
  want.callback = callback;
  want.userdata = &audio;
  if (SDL_OpenAudio(&want, &got) < 0) {
    snprintf(r.error, sizeof(r.error), "open: %s", SDL_GetError());
    strcpy(view.audioStatus, "AUDIO OPEN FAILED / SEE REPORT");
    saveReport();
    return;
  }
  r.actualRate = view.obtainedRate = got.freq;
  r.actualFrames = view.obtainedFrames = got.samples;
  if (got.format != AUDIO_S16SYS || got.channels != 2 || got.freq < 8000 || got.freq > 96000) {
    snprintf(r.error, sizeof(r.error), "Unsupported returned audio format %u, channels %u, rate %d",
             got.format, got.channels, got.freq);
    SDL_CloseAudio();
    strcpy(view.audioStatus, "UNEXPECTED FORMAT / SEE REPORT");
    saveReport();
    return;
  }
  audio.rate = got.freq;
  audio.frames = got.samples;
  if (audio.sampleMode) {
    audio.player.prepare(sample, got.freq, view.samplePitch, view.sampleStart, view.sampleEnd,
                         view.sampleMode);
    r.voices = audio.player.voices;
  } else
    audio.tone.prepare(got.freq, view.hz, view.mode);
  r.opened = true;
  view.audioOpened = true;
  audioOpen = true;
  view.audioRunning = true;
  audioStarted = micros();
  strcpy(view.audioStatus, audio.sampleMode ? "WAV PLAYING / START STOP / 10 SEC MAX"
                                            : "QUIET TONE / START TO STOP / 10 SEC MAX");
  saveReport();
  // Playback can start after frameStart: use its own fresh monotonic timestamp.
  audioStarted = micros();
  SDL_PauseAudio(0);
}
static int sdlButton(SDLKey k) {
  switch (k) {
  case SDLK_UP:
    return Up;
  case SDLK_DOWN:
    return Down;
  case SDLK_LEFT:
    return Left;
  case SDLK_RIGHT:
    return Right;
  case SDLK_z:
    return A;
  case SDLK_x:
    return B;
  case SDLK_c:
    return X;
  case SDLK_v:
    return Y;
  case SDLK_SPACE:
  case SDLK_RETURN:
    return Start;
  case SDLK_TAB:
    return Select;
  case SDLK_q:
    return L1;
  case SDLK_e:
    return R1;
  case SDLK_LEFTBRACKET:
    return L2;
  case SDLK_RIGHTBRACKET:
    return R2;
  case SDLK_ESCAPE:
    return Menu;
  default:
    return -1;
  }
}
static void adjust();
static bool gesture[4] = {};
static uint64_t pressAt[ButtonCount] = {};
static void eventButton(int b, bool down) {
  if (b < 0)
    return;
  bool was = (view.buttons.held & bit(b)) != 0;
  if (was == down)
    return;
  view.buttons.event(b, down);
  view.missingInput = false;
  uint64_t now = micros();
  view.holds.update(view.buttons.held, now);
  snprintf(view.lastEvent, sizeof(view.lastEvent), "%s %s / EVENT %u", names[b],
           down ? "DOWN" : "UP", view.buttons.changes);
  if (down) {
    pressAt[b] = now;
    if (b >= A && b <= Y)
      gesture[b - A] = false;
    if (b == L2 || b == R2) {
      view.page = (view.page + (b == R2 ? 1 : 4)) % 5;
      stopAudio();
    }
    if (view.page == 4 && (b == L1 || b == R1)) {
      stopAudio();
      view.sampleIndex = (view.sampleIndex + (b == R1 ? 1 : 2)) % 3;
      view.sampleLoaded = false;
      strcpy(view.audioStatus, "START LOADS AND PLAYS SELECTED WAV");
    }
    if (b == Start) {
      if ((view.page == 1 || view.page == 4) && !(view.buttons.held & bit(Select)))
        startAudio();
      if (view.page == 2)
        view.motion = !view.motion;
    }
  }
  adjust();
  if (!down) {
    uint64_t held = now - pressAt[b];
    if (b == Select && !(view.buttons.held & bit(Start)) && held < 1500000) {
      audioSnapshot();
      saveReport();
    }
    if (b >= A && b <= Y && !gesture[b - A] && held < 500000) {
      if ((view.page == 1 || view.page == 4) && runCount &&
          runs[runCount - 1].sampleMode == (view.page == 4)) {
        const char *verdicts[] = {"HEARD", "SILENT", "CLICKS", "NOT LISTENED"};
        strcpy(view.verdict, verdicts[b - A]);
        strcpy(runs[runCount - 1].verdict, view.verdict);
        audioSnapshot();
        saveReport();
      }
      if (view.page == 3 && b == A)
        storageTest();
    }
  }
}
static void openInput() {
  for (int i = 0; i < 8; i++) {
    char path[32];
    snprintf(path, sizeof(path), "/dev/input/event%d", i);
    int fd = open(path, O_RDONLY | O_NONBLOCK);
    if (fd < 0)
      continue;
    Device &d = devices[deviceCount++];
    d.fd = fd;
    strcpy(d.path, path);
    if (ioctl(fd, EVIOCGNAME(sizeof(d.name)), d.name) < 0)
      strcpy(d.name, "UNKNOWN");
    if (!strcmp(d.name, "RG35XX Gamepad")) {
      d.input.profile = InputProfile::GarlicGamepad;
      for (int j = 0; j < 2; j++) {
        input_absinfo info = {};
        if (ioctl(fd, EVIOCGABS(j == 0 ? ABS_Z : ABS_RZ), &info) >= 0)
          d.input.triggers[j].configure(info.minimum, info.maximum);
      }
    } else {
      unsigned char supported[(KEY_MAX + 8) / 8] = {};
      if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(supported)), supported) >= 0) {
        bool legacy = true;
        for (int code = 0x5a; code <= 0x68; code++)
          if (!(supported[code / 8] & (1u << (code % 8))))
            legacy = false;
        if (legacy)
          d.input.profile = InputProfile::LegacyKeys;
      }
    }
    if (d.input.profile != InputProfile::Unknown)
      mappedDevices++;
  }
  snprintf(view.inputBackend, sizeof(view.inputBackend),
           mappedDevices ? "EVDEV / %d MAPPED DEVICES" : "SDL KEYBOARD / %d MAPPED EVDEV",
           mappedDevices);
  view.missingInput = true;
}
static void reconcileInput() {
  uint32_t all = 0;
  for (int i = 0; i < deviceCount; i++)
    all |= devices[i].input.held();
  for (int b = 0; b < ButtonCount; b++)
    eventButton(b, (all & bit(b)) != 0);
}
static void pollInput() {
  for (int i = 0; i < deviceCount; i++) {
    Device &d = devices[i];
    input_event ev;
    int count = 0;
    while (count++ < 256) {
      ssize_t size = read(d.fd, &ev, sizeof(ev));
      if (size != (ssize_t)sizeof(ev)) {
        if (size < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
          d.readError = errno;
        break;
      }
      rawEvents++;
      if (ev.type == EV_SYN && ev.code == SYN_DROPPED) {
        droppedEvents++;
        d.syncLost = true;
        continue;
      }
      if (d.syncLost) {
        if (ev.type != EV_SYN || ev.code != SYN_REPORT)
          continue;
        unsigned char keys[(KEY_MAX + 8) / 8] = {};
        d.input.clear();
        if (ioctl(d.fd, EVIOCGKEY(sizeof(keys)), keys) >= 0)
          for (int code = 0; code <= KEY_MAX; code++)
            if (keys[code / 8] & (1u << (code % 8)))
              d.input.key(code, 1);
        if (d.input.profile == InputProfile::GarlicGamepad) {
          const int axes[] = {ABS_Z, ABS_RZ, ABS_HAT0X, ABS_HAT0Y};
          for (int code : axes) {
            input_absinfo info = {};
            if (ioctl(d.fd, EVIOCGABS(code), &info) >= 0)
              d.input.axis(code, info.value);
          }
        }
        d.syncLost = false;
        reconcileInput();
        continue;
      }
      if (ev.type != EV_KEY && ev.type != EV_ABS)
        continue;
      rawLog[logCount++ % 512] = {unsigned((micros() - bootUs) / 1000), i, ev.type, ev.code,
                                  ev.value};
      bool recognized =
          ev.type == EV_KEY ? d.input.key(ev.code, ev.value) : d.input.axis(ev.code, ev.value);
      if (!recognized) {
        if (ev.type == EV_KEY)
          unknownEvents++;
        else
          unknownAxes++;
        snprintf(view.lastEvent, sizeof(view.lastEvent), "RAW %s %u = %d / UNMAPPED",
                 ev.type == EV_KEY ? "KEY" : "AXIS", ev.code, ev.value);
        continue;
      }
      reconcileInput();
    }
  }
}

static void adjust() {
  static ParameterGesture repeater;
  uint32_t held = view.buttons.held;
  // Mark gestures as soon as both keys overlap, even within one display frame.
  if (held & 15)
    for (int i = 0; i < 4; i++)
      if (held & bit(A + i))
        gesture[i] = true;
  if (!repeater.tick(held, micros()))
    return;
  int direction = repeater.direction;
  for (int i = 0; i < 4; i++)
    if (repeater.faces & (1u << i)) {
      if (view.page == 0 || view.page == 1)
        parameterAttempts[i]++;
      int previousValue = view.page == 0 ? view.values[i]
                          : i == 0       ? view.rate
                          : i == 1       ? view.buffer
                          : i == 2       ? view.mode
                                         : view.hz;
      if (view.page == 0)
        view.values[i] = clamp(view.values[i] + direction * repeater.step, 0, 100);
      if (view.page == 1) {
        stopAudio();
        if (i == 0)
          view.rate = direction > 0 ? 48000 : 44100;
        if (i == 1)
          view.buffer = clamp(direction > 0 ? view.buffer * 2 : view.buffer / 2, 256, 2048);
        if (i == 2)
          view.mode = clamp(view.mode + direction, 0, 3);
        if (i == 3)
          view.hz = clamp(direction > 0 ? view.hz * 2 : view.hz / 2, 220, 880);
      }
      if (view.page == 4) {
        stopAudio();
        int *values[] = {&view.samplePitch, &view.sampleStart, &view.sampleEnd, &view.sampleMode};
        int low[] = {-24, 0, view.sampleStart + 1, 0};
        int high[] = {24, view.sampleEnd - 1, 100, 2};
        *values[i] = clamp(*values[i] + direction * (i == 3 ? 1 : repeater.step), low[i], high[i]);
      }
      int nextValue = view.page == 0 ? view.values[i]
                      : i == 0       ? view.rate
                      : i == 1       ? view.buffer
                      : i == 2       ? view.mode
                                     : view.hz;
      if ((view.page == 0 || view.page == 1) && nextValue != previousValue)
        parameterChanges[i]++;
    }
}

int main(int argc, char **argv) {
  const char *dir = argc > 1 ? argv[1] : "reports";
  if (strlen(dir) > 350)
    return 2;
  strcpy(reportDir, dir);
  if (mkdir(dir, 0755) && errno != EEXIST) {
    fprintf(stderr, "Cannot create report directory: %s\n", strerror(errno));
    return 3;
  }
  time_t t = time(nullptr);
  tm date = {};
  localtime_r(&t, &date);
  char stamp[64];
  strftime(stamp, sizeof(stamp), "%Y%m%d-%H%M%S", &date);
  // A device without a valid RTC may repeat both timestamp and PID after reboot.
  // Reserve a unique report instead of overwriting an earlier session.
  bool reserved = false;
  for (unsigned sequence = 0; sequence < 10000; sequence++) {
    snprintf(reportPath, sizeof(reportPath), "%s/report-%s-%ld-%u.json", dir, stamp, (long)getpid(),
             sequence);
    int fd = open(reportPath, O_CREAT | O_EXCL | O_WRONLY, 0644);
    if (fd >= 0) {
      close(fd);
      reserved = true;
      break;
    }
    if (errno != EEXIST)
      break;
  }
  if (!reserved) {
    fprintf(stderr, "Cannot reserve report: %s\n", strerror(errno));
    return 3;
  }
  bootUs = micros();
  utsname u = {};
  if (!uname(&u)) {
    snprintf(view.machine, sizeof(view.machine), "%s", u.machine);
    snprintf(view.kernel, sizeof(view.kernel), "%s", u.release);
  }
  struct statvfs st = {};
  if (!statvfs(dir, &st))
    freeBytes = uint64_t(st.f_bavail) * st.f_frsize;
  saveReport();
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
    snprintf(fatalError, sizeof(fatalError), "SDL init: %s", SDL_GetError());
    saveReport();
    fprintf(stderr, "%s\n", fatalError);
    return 4;
  }
  SDL_Surface *screen = SDL_SetVideoMode(W, H, 16, SDL_HWSURFACE);
  if (!screen)
    screen = SDL_SetVideoMode(W, H, 16, SDL_SWSURFACE);
  if (!screen)
    screen = SDL_SetVideoMode(W, H, 32, SDL_SWSURFACE);
  SDL_Surface *source = SDL_CreateRGBSurfaceFrom(canvas.pixels, W, H, 32, W * 4, 0x00ff0000,
                                                 0x0000ff00, 0x000000ff, 0);
  if (!screen || !source || screen->w != W || screen->h != H) {
    snprintf(fatalError, sizeof(fatalError), "640x480 video: %s", SDL_GetError());
    saveReport();
    SDL_Quit();
    return 5;
  }
  view.bpp = screen->format->BitsPerPixel;
  videoOK = true;
  SDL_VideoDriverName(view.videoDriver, sizeof(view.videoDriver));
  SDL_ShowCursor(SDL_DISABLE);
  openInput();
  saveReport();
  bool running = true;
  uint64_t exitSince = 0, frameStart = micros(), fpsStart = frameStart, lastMemory = frameStart;
  uint64_t lastSave = frameStart;
  unsigned frames = 0;
  while (running) {
    frameStart = micros();
    pollInput();
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
      if (ev.type == SDL_QUIT)
        running = false;
      if (!mappedDevices && (ev.type == SDL_KEYDOWN || ev.type == SDL_KEYUP))
        eventButton(sdlButton(ev.key.keysym.sym), ev.type == SDL_KEYDOWN);
    }
    adjust();
    uint64_t now = micros();
    view.holds.update(view.buttons.held, now);
    bool exitHeld =
        (view.buttons.held & bit(Menu)) ||
        ((view.buttons.held & (bit(Start) | bit(Select))) == (bit(Start) | bit(Select)));
    if (exitHeld) {
      if (!exitSince)
        exitSince = now;
      if (deadlineReached(now, exitSince, 2000000))
        running = false;
    } else
      exitSince = 0;
    if (view.missingInput && deadlineReached(now, bootUs, 60000000)) {
      strcpy(fatalError, "No recognized buttons in 60 seconds; automatic exit");
      running = false;
    }
    if (audioOpen) {
      audioSnapshot();
      SDL_LockAudio();
      bool done = audio.sampleMode ? audio.player.done : audio.tone.done;
      SDL_UnlockAudio();
      if (done || deadlineReached(now, audioStarted, 12000000)) {
        if (!done) {
          strcpy(runs[runCount - 1].error, "12 second watchdog expired");
          strcpy(runs[runCount - 1].endReason, "watchdog");
        } else if (!runs[runCount - 1].endReason[0])
          strcpy(runs[runCount - 1].endReason, "completed");
        closeAudio();
      }
    }
    if (frameStart - lastMemory > 2000000) {
      memory();
      lastMemory = frameStart;
    }
    if (!audioOpen && frameStart - lastSave >= 15000000) {
      saveReport();
      lastSave = frameStart;
    }
    view.liveSeconds = unsigned((frameStart - bootUs) / 1000000);
    view.rawCount = rawEvents;
    view.tick = unsigned((frameStart - bootUs) / 6000);
    render(canvas, view);
    if (SDL_BlitSurface(source, nullptr, screen, nullptr) < 0 || SDL_Flip(screen) < 0)
      flipErrors++;
    frames++;
    uint64_t elapsed = micros() - fpsStart;
    if (elapsed >= 1000000) {
      view.fps = double(frames) * 1000000 / elapsed;
      frames = 0;
      fpsStart = micros();
    }
    elapsed = micros() - frameStart;
    if (elapsed < 33333)
      SDL_Delay(unsigned((33333 - elapsed) / 1000));
  }
  stopAudio();
  if (audioOpen) {
    SDL_Delay(30);
    closeAudio();
  }
  completed = true;
  saveReport();
  for (int i = 0; i < deviceCount; i++)
    close(devices[i].fd);
  SDL_FreeSurface(source);
  SDL_Quit();
  return fatalError[0] ? 6 : 0;
}
