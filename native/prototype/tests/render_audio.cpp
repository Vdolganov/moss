#include "../src/engine.h"
#include <stdio.h>
using namespace moss;
static moss::SampleBuffer samples[3];
static Engine engine;
static int16_t buffer[2048];
static void le(FILE *f, unsigned n, int bytes) {
  for (int i = 0; i < bytes; i++)
    fputc((n >> (8 * i)) & 255, f);
}
int main(int argc, char **argv) {
  if (argc != 2 && argc != 3)
    return 1;
  for (int i = 0; i < 3; i++)
    if (!samples[i].load(lab::sampleFiles[i]))
      return 2;
  Project p = makeSynthDemo();
  if (argc == 3) {
    int kind = atoi(argv[2]);
    p = makeProject(false);
    p.bpm = 90;
    p.tracks[0].instrument = kind;
    p.tracks[1].mute = p.tracks[2].mute = 1;
    for (int n = 0; n < 256; n += 32) {
      p.tracks[0].steps[n].on = 1;
      p.tracks[0].steps[n].note = kind == 2 ? 43 : 55 + (n % 24);
      p.tracks[0].steps[n].length = 28;
    }
    p.tracks[0].cycle = 128;
  }
  engine.init(p, samples, 44100);
  engine.handle({Op::Transport});
  FILE *f = fopen(argv[1], "wb");
  if (!f)
    return 3;
  unsigned frames = 44100 * 16;
  fwrite("RIFF", 1, 4, f);
  le(f, 36 + frames * 4, 4);
  fwrite("WAVEfmt ", 1, 8, f);
  le(f, 16, 4);
  le(f, 1, 2);
  le(f, 2, 2);
  le(f, 44100, 4);
  le(f, 176400, 4);
  le(f, 4, 2);
  le(f, 16, 2);
  fwrite("data", 1, 4, f);
  le(f, frames * 4, 4);
  while (frames) {
    unsigned n = frames > 1024 ? 1024 : frames;
    engine.render(buffer, n);
    for (unsigned i = 0; i < n * 2; i++)
      le(f, unsigned(buffer[i]), 2);
    frames -= n;
  }
  fclose(f);
  return 0;
}
