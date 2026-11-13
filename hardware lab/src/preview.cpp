#include "lab_ui.h"
#include <stdlib.h>
using namespace lab;
static Canvas canvas;
static void le(FILE *f, unsigned v, int bytes) {
  for (int i = 0; i < bytes; i++)
    fputc((v >> (i * 8)) & 255, f);
}
int main(int argc, char **argv) {
  if (argc != 3 && argc != 4) {
    fprintf(stderr, "Usage: preview PAGE OUTPUT.bmp [hold]\n");
    return 1;
  }
  View v;
  v.preview = true;
  v.page = clamp(atoi(argv[1]), 0, 4);
  if (argc == 4 && !strcmp(argv[3], "hold")) {
    v.buttons.event(Y, true);
    v.buttons.event(Up, true);
    v.holds.update(v.buttons.held, 1000000);
    v.holds.update(v.buttons.held, 4800000);
    v.values[3] = 61;
  }
  if (v.page == 4) {
    static SampleBuffer sample;
    if (sample.load(sampleFiles[0])) {
      v.sampleLoaded = true;
      v.sampleRate = sample.rate;
      v.sampleChannels = sample.channels;
      v.sampleBytes = sample.bytes;
      v.sampleFrames = sample.frames;
      memcpy(v.samplePeaks, sample.peaks, sizeof(sample.peaks));
      v.sampleStart = 10;
      v.sampleEnd = 75;
      strcpy(v.audioStatus, "START PLAYS WAV / HOLD FACE + D-PAD EDIT");
    }
  }
  render(canvas, v);
  FILE *f = fopen(argv[2], "wb");
  if (!f)
    return 2;
  fwrite("BM", 1, 2, f);
  le(f, 54 + W * H * 3, 4);
  le(f, 0, 4);
  le(f, 54, 4);
  le(f, 40, 4);
  le(f, W, 4);
  le(f, H, 4);
  le(f, 1, 2);
  le(f, 24, 2);
  le(f, 0, 4);
  le(f, W * H * 3, 4);
  for (int i = 0; i < 4; i++)
    le(f, 0, 4);
  for (int y = H - 1; y >= 0; y--)
    for (int x = 0; x < W; x++)
      le(f, canvas.pixels[y * W + x], 3);
  int result = ferror(f) ? 3 : 0;
  fclose(f);
  return result;
}
