#include "granular_harness.h"
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif
using namespace moss;
static GranularHarness harness;
static int16_t output[AudioBlockFrames * 2];
static double durations[20000];
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
static int compare(const void *a, const void *b) {
  double x = *(const double *)a, y = *(const double *)b;
  return (x > y) - (x < y);
}
int main(int argc, char **argv) {
  if (argc != 6)
    return 2;
  unsigned tracks = unsigned(atoi(argv[1])), cap = unsigned(atoi(argv[2]));
  unsigned mask = unsigned(atoi(argv[3])), mode = unsigned(atoi(argv[4]));
  unsigned seconds = unsigned(atoi(argv[5]));
  unsigned secondBit = mask & (mask - 1);
  if (!tracks || tracks > 4 || !cap || cap > 32 || mask > 15 || mode > 3 || !seconds ||
      seconds > 600 || (secondBit & (secondBit - 1)))
    return 2;
  harness.init(tracks, cap, mask, mode);
  for (unsigned n = 0; n < AudioRate * 2; n += AudioBlockFrames)
    harness.render(output, AudioBlockFrames);
  // Reset only counters; preserve warm grain/filter histories for timing.
  for (auto &v : harness.voices)
    v.stats = {};
  harness.capture.begin(AudioRate, AudioRate * 8, -1);
  double total = 0, maximum = 0;
  unsigned blocks = (AudioRate * seconds + AudioBlockFrames - 1) / AudioBlockFrames;
  unsigned over = 0;
  uint32_t hash = 2166136261u;
  for (unsigned b = 0; b < blocks; ++b) {
    double start = nowUs();
    harness.render(output, AudioBlockFrames);
    double elapsed = nowUs() - start;
    durations[b] = elapsed;
    total += elapsed;
    if (elapsed > maximum)
      maximum = elapsed;
    if (elapsed > 32000)
      ++over;
    for (int16_t value : output) {
      hash ^= uint16_t(value);
      hash *= 16777619u;
    }
  }
  qsort(durations, blocks, sizeof(double), compare);
  uint64_t spawned = 0, skipped = 0;
  unsigned peak = 0;
  for (unsigned t = 0; t < tracks; ++t) {
    spawned += harness.voices[t].stats.spawned;
    skipped += harness.voices[t].stats.skipped;
    if (harness.voices[t].stats.peak > peak)
      peak = harness.voices[t].stats.peak;
  }
  printf(
      "{\"tracks\":%u,\"capacity\":%u,\"fx_mask\":%u,\"mode\":%u,\"seconds\":%u,"
      "\"rate\":32000,\"block\":1024,\"blocks\":%u,\"mean_us\":%.3f,\"p99_us\":%.3f,"
      "\"max_us\":%.3f,\"over_budget\":%u,\"spawned\":%llu,\"skipped\":%llu,"
      "\"peak_grains_per_track\":%u,\"output_peak\":%d,\"hash\":\"%08x\",\"reserved_bytes\":%u}\n",
      tracks, cap, mask, mode, seconds, blocks, total / blocks, durations[(blocks - 1) * 99 / 100],
      maximum, over, (unsigned long long)spawned, (unsigned long long)skipped, peak, harness.peak,
      hash, unsigned(sizeof(harness)));
  return over ? 1 : 0;
}
