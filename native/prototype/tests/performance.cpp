// Same headless harness builds against the preserved baseline and current
// engine. No filesystem access, hashing or profiling calls inside the timed DSP
// region.
#include "../src/diagnostics/stress_scenario.h"
#include <engine.h>
#include <stdlib.h>
#include <storage.h>
#ifdef _WIN32
#include <psapi.h>
#else
#include <time.h>
#endif
using namespace moss;
static Engine engine;
static Project project, original;
static SampleBuffer samples[32];
static int16_t output[8192];
static double durations[32768];
static double nowUs() {
#ifdef _WIN32
  LARGE_INTEGER t, f;
  QueryPerformanceCounter(&t);
  QueryPerformanceFrequency(&f);
  return double(t.QuadPart) * 1000000 / double(f.QuadPart);
#else
  timespec t = {};
  clock_gettime(CLOCK_MONOTONIC, &t);
  return double(t.tv_sec) * 1000000 + t.tv_nsec / 1000.;
#endif
}
static int compareTime(const void *a, const void *b) {
  double x = *(const double *)a, y = *(const double *)b;
  return (x > y) - (x < y);
}
static void disableFx(Effect &fx) {
  fx.enabled = 0;
  for (int &v : fx.enabledTypes)
    v = 0;
}
static void memoryLimit(int mb, int cpu) {
#ifdef _WIN32
  if (!mb && !cpu)
    return;
  HANDLE job = CreateJobObjectA(nullptr, nullptr);
  JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {};
  if (mb) {
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_PROCESS_MEMORY;
    limits.ProcessMemoryLimit = SIZE_T(mb) * 1024 * 1024;
  }
  if (!job ||
      !SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits,
                               sizeof(limits)) ||
      !AssignProcessToJobObject(job, GetCurrentProcess())) {
    fprintf(stderr, "Job limit failed: %lu\n", GetLastError());
    exit(3);
  }
  if (cpu) {
    JOBOBJECT_CPU_RATE_CONTROL_INFORMATION limit = {};
    limit.ControlFlags = JOB_OBJECT_CPU_RATE_CONTROL_ENABLE |
                         JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP;
    limit.CpuRate = DWORD(cpu * 100);
    if (!SetInformationJobObject(job, JobObjectCpuRateControlInformation,
                                 &limit, sizeof(limit)))
      exit(4);
  }
  // Windows retains the assigned limits until this process terminates.
  fprintf(stderr,
          "Windows process budget: %d MiB; CPU quota: %d%% (0=uncapped); not "
          "an ARM emulator\n",
          mb, cpu);
#else
  if (mb || cpu) {
    fprintf(stderr, "Use native device memory/CPU, no Windows quota\n");
    exit(3);
  }
#endif
}
static void runCase(const char *name, unsigned rate, unsigned block,
                    int seconds, const char *dump) {
  engine.init(project, samples, rate, 32);
  engine.handle({Op::Transport});
  for (unsigned n = 0; n < rate; n += block)
    engine.render(output, block);
  engine.init(project, samples, rate, 32);
  engine.handle({Op::Transport});
  FILE *f = nullptr;
  if (dump && strcmp(dump, "-")) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s.pcm", dump, name);
    f = fopen(path, "wb");
    if (!f)
      exit(5);
  }
  double sum = 0, max = 0;
  unsigned over = 0, blocks = 0;
  uint32_t hash = 2166136261u;
  const unsigned total = rate * unsigned(seconds);
  for (unsigned at = 0; at < total;) {
    unsigned n = total - at < block ? total - at : block;
    double begin = nowUs();
    engine.render(output, n);
    double elapsed = nowUs() - begin;
    durations[blocks++] = elapsed;
    sum += elapsed;
    if (elapsed > max)
      max = elapsed;
    if (elapsed > double(n) * 1000000 / rate)
      over++;
    for (unsigned i = 0; i < n * 2; i++) {
      hash ^= uint16_t(output[i]);
      hash *= 16777619u;
    }
    if (f && fwrite(output, 4, n, f) != n)
      exit(6);
    at += n;
  }
  if (f)
    fclose(f);
  qsort(durations, blocks, sizeof(double), compareTime);
  printf("{\"case\":\"%s\",\"rate\":%u,\"block\":%u,\"seconds\":%d,\"mean_us\":"
         "%.3f,\"median_us\":%.3f,\"p99_us\":%.3f,\"max_us\":%.3f,\"over_"
         "budget\":%u,\"render_ms\":%.3f,\"hash\":\"%08x\"}\n",
         name, rate, block, seconds, sum / blocks, durations[blocks / 2],
         durations[(blocks - 1) * 99 / 100], max, over, sum / 1000, hash);
  fflush(stdout);
}
int main(int argc, char **argv) {
  // bank-root seconds dump-directory rate block memory-MiB cpu-percent
  // case-filter
  if (argc < 2)
    return 2;
  int seconds = argc > 2 ? atoi(argv[2]) : 10;
  const char *dump = argc > 3 ? argv[3] : "-";
  unsigned rate = argc > 4 ? unsigned(atoi(argv[4])) : 44100,
           block = argc > 5 ? unsigned(atoi(argv[5])) : 1024;
  int mb = argc > 6 ? atoi(argv[6]) : 0, cpu = argc > 7 ? atoi(argv[7]) : 0;
  const char *filter = argc > 8 ? argv[8] : nullptr;
  unsigned effectMask = argc > 9 ? unsigned(atoi(argv[9])) : 15;
  if (seconds < 1 || seconds > 20 || rate < 8000 || rate > 96000 ||
      block < 64 || block > 4096 || mb < 0 || cpu < 0 || cpu > 100)
    return 2;
  memoryLimit(mb, cpu);
  char path[1024];
  for (int a = 0; a < 32; a++) {
    snprintf(path, sizeof(path), "%s/library/asset-%02d.wav", argv[1], a);
    samples[a].load(path);
  }
  for (int id = 1; id <= 32; id++) {
    snprintf(path, sizeof(path), "%s/projects/project-%d.moss", argv[1], id);
    if (!loadProject(path, original))
      continue;
    fprintf(stderr, "Project %d: %s; %d tracks; BPM %d\n", id, original.name,
            original.trackCount, original.bpm);
    for (int t = 0; t < original.trackCount; t++) {
      const auto &tr = original.tracks[t];
      fprintf(stderr, "  %s %s %d cells 1/%d mute=%d; FX", tr.name,
              instrumentName(tr.instrument), tr.cells(), tr.division, tr.mute);
      for (int k : tr.fx.order)
        if (tr.fx.active(k))
          fprintf(stderr, " %s", fxName(k));
      fprintf(stderr, "\n");
    }
    const char *modes[] = {"full", "no-fm", "dry", "with-fm", "all-on"};
    for (int mode = 0; mode < 5; mode++) {
      char name[64];
      snprintf(name, sizeof(name), "project-%d-%s", id, modes[mode]);
      if (filter && strcmp(filter, name))
        continue;
      project = original;
      for (int t = 0; t < project.trackCount; t++) {
        if (mode == 1 && project.tracks[t].instrument == FmInstrument)
          project.tracks[t].mute = 1;
        if (mode == 3 && project.tracks[t].instrument == FmInstrument)
          project.tracks[t].mute = 0;
        if (mode == 4)
          project.tracks[t].mute = 0;
        if (mode == 2)
          disableFx(project.tracks[t].fx);
      }
      if (mode == 2)
        disableFx(project.fx);
      runCase(name, rate, block, seconds, dump);
    }
  }
  for (auto &sample : samples)
    stressSample(sample);
  const int trackCounts[] = {2, 4};
  for (int tracks : trackCounts)
    for (int kind = -1; kind <= FmInstrument; ++kind) {
      char name[64];
      snprintf(name, sizeof(name), "stress-%d-%s", tracks,
               kind < 0 ? "mixed" : instrumentName(kind));
      if (filter && strcmp(filter, name))
        continue;
      stressProject(project, tracks, kind, effectMask);
      runCase(name, rate, block, seconds, dump);
    }
  for (int kind = MonoInstrument; kind <= FmInstrument; kind++) {
    char name[32];
    snprintf(name, sizeof(name), "solo-%s", instrumentName(kind));
    if (filter && strcmp(filter, name))
      continue;
    project = Project{};
    project.trackCount = 1;
    auto &tr = project.tracks[0];
    tr.instrument = kind;
    tr.cycle = 128;
    tr.steps[0].on = 1;
    tr.steps[0].length = 128;
    runCase(name, rate, block, seconds, dump);
  }
#ifdef _WIN32
  PROCESS_MEMORY_COUNTERS_EX memory = {};
  memory.cb = sizeof(memory);
  GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&memory,
                       sizeof(memory));
  fprintf(stderr, "Memory private=%llu peak_working_set=%llu bytes\n",
          (unsigned long long)memory.PrivateUsage,
          (unsigned long long)memory.PeakWorkingSetSize);
#endif
}
