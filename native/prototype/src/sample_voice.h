#pragma once
#include "config.h"
#include "sample.h"
namespace moss {
struct Voice {
  const moss::SampleBuffer *sample = nullptr;
  uint64_t phase = 0, increment = 0;
  unsigned start = 0, end = 0, age = 0;
  int key = 0, velocity = 0, level = 0, remaining = 0, release = 0, mode = 0;
  int16_t locks[4] = {Inherit, Inherit, Inherit, Inherit};
  unsigned fadeInFrames = 0, fadeOutSource = 0;
  float amplitude = 0;
  bool active = false;
  void stop() {
    if (active && !release)
      release = 128;
  }
  void render(int &l, int &r) {
    if (!active)
      return;
    unsigned index = unsigned(phase >> 32);
    if (index >= end) {
      active = false;
      return;
    }
    unsigned next = index + 1 < end ? index + 1 : mode == 2 ? start : index;
    unsigned frac = unsigned((phase >> 16) & 65535);
    int gain = int(age < 128 ? age : 128);
    if (release && release < gain)
      gain = release;
    unsigned edge = (end - start) / 2;
    if (edge > 64)
      edge = 64;
    unsigned dist = index - start;
    if (end - 1 - index < dist)
      dist = end - 1 - index;
    if (edge && dist < edge)
      gain = gain * int(dist) / int(edge);
    if (fadeInFrames && age < fadeInFrames)
      gain = int(unsigned(gain) * age / fadeInFrames);
    if (fadeOutSource && end - 1 - index < fadeOutSource)
      gain = int(unsigned(gain) * (end - 1 - index) / fadeOutSource);
    int *output[] = {&l, &r};
    for (int c = 0; c < 2; c++) {
      int ch = sample->channels == 1 ? 0 : c;
      int a = sample->pcm[index * sample->channels + ch],
          b = sample->pcm[next * sample->channels + ch];
      int value = int((int64_t(a) * (65536 - frac) + int64_t(b) * frac) >> 16);
      // Gain is prepared on trigger/edit. Float multiplication avoids a 64-bit
      // software divide for each sample/channel on Cortex-A9 (<= 1 PCM LSB).
      *output[c] += int(float(value * gain) * amplitude);
    }
    age++;
    phase += increment;
    if (phase >= (uint64_t(end) << 32)) {
      if (mode == 2)
        phase = (uint64_t(start) << 32) + (phase - (uint64_t(start) << 32)) %
                                              (uint64_t(end - start) << 32);
      else
        active = false;
    }
    if (release && !--release)
      active = false;
  }
};
} // namespace moss
