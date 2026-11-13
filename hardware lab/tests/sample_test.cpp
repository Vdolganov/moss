#include "../src/lab_sample.h"
#undef NDEBUG
#include <assert.h>
#include <vector>
#include <algorithm>
using namespace lab;
static SampleBuffer sample;
static void put32(std::vector<unsigned char> &b, unsigned at, unsigned n) {
  for (unsigned i = 0; i < 4; i++)
    b[at + i] = (n >> (8 * i)) & 255;
}
static bool loadBytes(const std::vector<unsigned char> &b) {
  const char *p = "build/sample-invalid.wav";
  FILE *f = fopen(p, "wb");
  assert(f);
  assert(fwrite(b.data(), 1, b.size(), f) == b.size());
  fclose(f);
  bool result = sample.load(p);
  remove(p);
  return result;
}
int main() {
  for (int file = 0; file < 3; file++) {
    assert(sample.load(sampleFiles[file]));
    assert(sample.bytes <= SampleBudget);
    assert(sample.frames > 0);
    for (unsigned rate : {44100u, 48000u})
      for (int pitch : {-24, 0, 24})
        for (int mode = 0; mode < 3; mode++) {
          SamplePlayer player;
          player.prepare(sample, rate, pitch, 13, 87, mode);
          assert(player.voices == unsigned(mode == 2 ? 3 : 1));
          int16_t buffer[516];
          unsigned rendered = 0;
          long long energy = 0;
          while (!player.done && rendered < rate * 11) {
            buffer[0] = 12345;
            buffer[515] = -12345;
            player.render(buffer + 1, 257);
            assert(buffer[0] == 12345 && buffer[515] == -12345);
            for (int i = 1; i < 515; i++)
              energy += buffer[i] < 0 ? -int(buffer[i]) : buffer[i];
            for (unsigned v = 0; v < player.voices; v++)
              if (player.active[v])
                assert((player.phase[v] >> 32) < player.endFrame);
            rendered += 257;
          }
          assert(player.done && energy > 0);
          assert(player.position <= rate * 10);
          if (mode)
            assert(player.position == rate * 10 && player.wraps > 0);
          if (mode == 0 && pitch == 0) {
            unsigned expected =
                unsigned(uint64_t(player.endFrame - player.startFrame) * rate / sample.rate);
            assert(player.position >= expected && player.position <= expected + 2);
          }
          player.prepare(sample, rate, 0, 0, 100, 1);
          player.render(buffer, 128);
          unsigned stopped = player.position;
          player.stop();
          for (int n = 0; n < 10 && !player.done; n++)
            player.render(buffer, 256);
          assert(player.done && player.position <= stopped + rate / 100);
        }
  }
  assert(sample.load(sampleFiles[1]));
  SamplePlayer stereo;
  stereo.prepare(sample, 48000, 0, 0, 100, 0);
  int16_t out[2048];
  stereo.render(out, 1024);
  bool different = false;
  for (int i = 0; i < 1024; i++)
    different |= out[2 * i] != out[2 * i + 1];
  assert(different);
  assert(sample.load(sampleFiles[0]));
  SamplePlayer mono;
  mono.prepare(sample, 44100, 0, 0, 100, 0);
  mono.render(out, 1024);
  for (int i = 0; i < 1024; i++)
    assert(out[2 * i] == out[2 * i + 1]);
  FILE *f = fopen(sampleFiles[0], "rb");
  assert(f);
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  rewind(f);
  std::vector<unsigned char> valid(size);
  assert(fread(valid.data(), 1, valid.size(), f) == valid.size());
  fclose(f);
  auto b = valid;
  b.resize(10);
  assert(!loadBytes(b) && sample.frames == 0);
  b = valid;
  b[0] = 'X';
  assert(!loadBytes(b));
  b = valid;
  b.pop_back();
  assert(!loadBytes(b));
  b = valid;
  b[20] = 3;
  assert(!loadBytes(b));
  b = valid;
  b[22] = 3;
  assert(!loadBytes(b));
  b = valid;
  b[34] = 8;
  assert(!loadBytes(b));
  b = valid;
  put32(b, 40, 0xffffffffu);
  assert(!loadBytes(b));
  b = valid;
  b.resize(44 + SampleBudget + 2);
  put32(b, 4, unsigned(b.size() - 8));
  put32(b, 40, SampleBudget + 2);
  assert(!loadBytes(b));
  b = valid;
  b.insert(b.begin() + 12, {'J', 'U', 'N', 'K', 1, 0, 0, 0, 42, 0});
  put32(b, 4, unsigned(b.size() - 8));
  assert(loadBytes(b));
  SamplePlayer tiny;
  sample.frames = 1;
  sample.channels = 1;
  sample.rate = 8000;
  sample.pcm[0] = 1000;
  tiny.prepare(sample, 48000, 24, 99, 100, 2);
  tiny.render(out, 1024);
  assert(!tiny.done && tiny.wraps > 0);
  tiny.prepare(sample, 1, 0, 0, 100, 0);
  assert(tiny.done);
  tiny.render(out, 1024);
  for (int i = 0; i < 2048; i++)
    assert(out[i] == 0);
  puts("Sample tests passed: bounded WAV loading, rates, pitch, trims, loops, three voices, fades "
       "and malformed files");
}
