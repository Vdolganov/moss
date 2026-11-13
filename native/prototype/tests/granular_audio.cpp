#include "../src/granular.h"
#include "../src/engine.h"
#include "../src/library.h"
#include <stdlib.h>
using namespace moss;
static SampleBuffer sample;
static GranularTables tables;
static GranularVoice voice;
static FxState fx;
static int16_t output[AudioRate * 10 * 2];
int main(int argc, char **argv) {
  if (argc != 3)
    return 2;
  tables.init();
  const char *sources[] = {"air", "dust", "chime"};
  const char *names[] = {"frozen-pad", "scanning-texture", "pitched-pulses"};
  for (unsigned example = 0; example < 3; ++example) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s.wav", argv[1], sources[example]);
    if (!sample.load(path) || !voice.init(sample, tables, AudioRate))
      return 3;
    GranularParameters p;
    Effect e;
    if (example == 0) {
      p.freeze = true;
      p.position = .4f;
      p.pitch = -12;
      p.sizeMs = 180;
      p.density = 40;
      p.scatter = 1;
      p.spread = .4f;
      p.cutoff = 2400;
      p.attackMs = p.releaseMs = 800;
      e.select(1);
      e.enable(true);
      e.values[1][3] = 35;
    } else if (example == 1) {
      p.speed = .15f;
      p.pitch = -12;
      p.sizeMs = 35;
      p.density = 80;
      p.irregularity = .7f;
      p.scatter = .65f;
      p.spread = .8f;
      p.cutoff = 6000;
      p.attackMs = p.releaseMs = 300;
      e.select(1);
      e.enable(true);
      e.values[1][3] = 25;
    } else {
      p.speed = 0;
      p.sizeMs = 100;
      p.density = 70;
      p.scatter = .02f;
      p.spread = .3f;
      p.cutoff = 8000;
      p.attackMs = p.releaseMs = 60;
      e.enable(true);
      e.values[0][0] = 1;
      e.values[0][2] = 30;
    }
    fx.reset();
    fx.prepare(e.values, AudioRate, 80);
    voice.setParameters(p);
    voice.noteOn();
    int peak = 0;
    double energy = 0;
    for (unsigned n = 0; n < AudioRate * 10; ++n) {
      if (example == 0 && n == AudioRate * 4) {
        p.freeze = false;
        p.speed = .05f;
        voice.setParameters(p);
      }
      if (example == 2 && n < AudioRate * 8) {
        if (n % (AudioRate / 2) == 0) {
          const float notes[] = {0, 7, 3, -5, 12, 7, 0, -12};
          voice.seek(.05f);
          voice.noteOn(notes[(n / (AudioRate / 2)) % 8]);
        }
        if (n % (AudioRate / 2) == AudioRate / 4)
          voice.noteOff();
      }
      if (n == AudioRate * 8)
        voice.noteOff();
      float l, r;
      voice.render(l, r);
      int left = int(l * 32768), right = int(r * 32768);
      fx.processChain(left, right, e, e.values, AudioRate, 80);
      int values[] = {Engine::monitor(left), Engine::monitor(right)};
      for (unsigned ch = 0; ch < 2; ++ch) {
        output[n * 2 + ch] = int16_t(values[ch]);
        int v = values[ch] < 0 ? -values[ch] : values[ch];
        if (v > peak)
          peak = v;
        energy += double(values[ch]) * values[ch];
      }
    }
    snprintf(path, sizeof(path), "%s/%s.wav", argv[2], names[example]);
    if (!writeWav(path, output, AudioRate * 10, AudioRate, 2))
      return 4;
    printf("%s: peak %d, RMS %.1f, skipped %llu\n", names[example], peak,
           sqrt(energy / (AudioRate * 20)), (unsigned long long)voice.stats.skipped);
  }
}
