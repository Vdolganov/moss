#ifndef MOSS_LAB_SAMPLE_H
#define MOSS_LAB_SAMPLE_H
#include "lab_core.h"
#include <stdio.h>

namespace lab {
constexpr unsigned SampleBudget = 256 * 1024;
static const char *const sampleNames[3] = {"CHIME", "AIR", "DUST"};
static const char *const sampleFiles[3] = {"samples/chime.wav", "samples/air.wav",
                                           "samples/dust.wav"};
inline unsigned little16(const unsigned char *p) {
  return unsigned(p[0]) | (unsigned(p[1]) << 8);
}
inline uint32_t little32(const unsigned char *p) {
  return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
struct SampleBuffer {
  int16_t pcm[SampleBudget / 2] = {};
  unsigned rate = 0, channels = 0, frames = 0, bytes = 0;
  int peaks[64] = {};
  char error[128] = {};
  bool load(const char *path) {
    rate = channels = frames = bytes = 0;
    memset(peaks, 0, sizeof(peaks));
    error[0] = 0;
    FILE *f = fopen(path, "rb");
    if (!f) {
      strcpy(error, "Cannot open WAV file");
      return false;
    }
    auto fail = [&](const char *why) {
      snprintf(error, sizeof(error), "%s", why);
      fclose(f);
      return false;
    };
    if (fseek(f, 0, SEEK_END))
      return fail("Cannot seek WAV");
    long length = ftell(f);
    if (length < 12 || length > 512 * 1024)
      return fail("WAV file must be 12..524288 bytes");
    rewind(f);
    unsigned char h[16];
    if (fread(h, 1, 12, f) != 12)
      return fail("Truncated RIFF header");
    if (memcmp(h, "RIFF", 4) || memcmp(h + 8, "WAVE", 4))
      return fail("Expected RIFF WAVE");
    uint64_t end = uint64_t(little32(h + 4)) + 8;
    if (end < 12 || end > unsigned(length))
      return fail("Invalid RIFF size");
    uint64_t cursor = 12;
    unsigned dataOffset = 0, dataBytes = 0;
    unsigned fmtRate = 0, fmtChannels = 0;
    bool haveFmt = false, haveData = false;
    unsigned chunks = 0;
    while (cursor < end) {
      if (++chunks > 128 || end - cursor < 8)
        return fail("Invalid WAV chunk list");
      if (fseek(f, long(cursor), SEEK_SET) || fread(h, 1, 8, f) != 8)
        return fail("Truncated chunk header");
      uint32_t size = little32(h + 4);
      uint64_t payload = cursor + 8, next = payload + size + (size & 1);
      if (next > end)
        return fail("Chunk exceeds RIFF boundary");
      if (!memcmp(h, "fmt ", 4)) {
        if (haveFmt || size < 16)
          return fail("Invalid or duplicate fmt chunk");
        if (fread(h, 1, 16, f) != 16)
          return fail("Truncated format");
        fmtChannels = little16(h + 2);
        fmtRate = little32(h + 4);
        if (little16(h) != 1 || little16(h + 14) != 16)
          return fail("Only uncompressed PCM16 WAV is supported");
        if ((fmtChannels != 1 && fmtChannels != 2) || fmtRate < 8000 || fmtRate > 96000)
          return fail("Use mono/stereo WAV at 8..96 kHz");
        if (little16(h + 12) != fmtChannels * 2 || little32(h + 8) != fmtRate * fmtChannels * 2)
          return fail("Inconsistent PCM format");
        haveFmt = true;
      } else if (!memcmp(h, "data", 4)) {
        if (haveData || size == 0 || size > SampleBudget)
          return fail("PCM data must be 1..262144 bytes");
        dataOffset = unsigned(payload);
        dataBytes = size;
        haveData = true;
      }
      cursor = next;
    }
    if (!haveFmt || !haveData || dataBytes % (fmtChannels * 2))
      return fail("Missing or unaligned PCM data");
    if (fseek(f, dataOffset, SEEK_SET) || fread(pcm, 1, dataBytes, f) != dataBytes)
      return fail("Truncated PCM data");
    fclose(f);
    auto *raw = reinterpret_cast<unsigned char *>(pcm);
    for (unsigned i = 0; i < dataBytes / 2; i++) {
      unsigned v = little16(raw + i * 2);
      pcm[i] = int16_t(v >= 32768 ? int(v) - 65536 : int(v));
    }
    rate = fmtRate;
    channels = fmtChannels;
    bytes = dataBytes;
    frames = bytes / (channels * 2);
    for (unsigned i = 0; i < frames; i++)
      for (unsigned ch = 0; ch < channels; ch++) {
        int v = pcm[i * channels + ch];
        if (v < 0)
          v = -v;
        unsigned bin = unsigned(uint64_t(i) * 64 / frames);
        if (v > peaks[bin])
          peaks[bin] = v;
      }
    return true;
  }
};
struct SamplePlayer {
  const SampleBuffer *sample = nullptr;
  uint64_t phase[3] = {}, increment[3] = {};
  bool active[3] = {}, loop = false, done = true;
  unsigned voices = 1, startFrame = 0, endFrame = 0, position = 0, limit = 0, fade = 441, wraps = 0;
  void prepare(const SampleBuffer &source, unsigned outputRate, int pitch, int startPercent,
               int endPercent, int mode) {
    sample = &source;
    position = wraps = 0;
    done = true;
    if (!source.frames || outputRate < 8000 || outputRate > 96000)
      return;
    startPercent = clamp(startPercent, 0, 99);
    startFrame = unsigned(uint64_t(source.frames) * clamp(startPercent, 0, 99) / 100);
    endFrame = unsigned(uint64_t(source.frames) * clamp(endPercent, startPercent + 1, 100) / 100);
    if (endFrame <= startFrame)
      endFrame = startFrame + 1;
    if (endFrame > source.frames)
      endFrame = source.frames;
    voices = mode == 2 ? 3 : 1;
    loop = mode != 0;
    limit = outputRate * 10;
    fade = outputRate / 100;
    const int offsets[3] = {0, -5, 7};
    for (unsigned i = 0; i < 3; i++) {
      phase[i] = uint64_t(startFrame) << 32;
      active[i] = i < voices;
      double speed =
          double(source.rate) / outputRate * pow(2.0, (clamp(pitch, -24, 24) + offsets[i]) / 12.0);
      increment[i] = uint64_t(speed * 4294967296.0);
      if (!increment[i])
        increment[i] = 1;
    }
    done = false;
  }
  void stop() {
    unsigned tail = position + fade;
    if (tail < limit)
      limit = tail;
  }
  int read(unsigned voice, unsigned channel) const {
    unsigned index = unsigned(phase[voice] >> 32),
             fraction = unsigned((phase[voice] >> 16) & 65535);
    unsigned next = index + 1 < endFrame ? index + 1 : loop ? startFrame : index;
    unsigned ch = sample->channels == 1 ? 0 : channel;
    int a = sample->pcm[index * sample->channels + ch],
        b = sample->pcm[next * sample->channels + ch];
    int value = int((int64_t(a) * (65536 - fraction) + int64_t(b) * fraction) / 65536);
    unsigned edge = (endFrame - startFrame) / 2;
    if (edge > 32)
      edge = 32;
    if (edge) {
      unsigned distance = index - startFrame + 1;
      if (endFrame - index < distance)
        distance = endFrame - index;
      if (distance < edge)
        value = int(int64_t(value) * distance / edge);
    }
    return value;
  }
  void render(int16_t *out, unsigned count) {
    for (unsigned frame = 0; frame < count; frame++) {
      int sum[2] = {};
      bool any = false;
      if (!done && position < limit)
        for (unsigned v = 0; v < voices; v++)
          if (active[v]) {
            any = true;
            sum[0] += read(v, 0);
            sum[1] += read(v, 1);
            phase[v] += increment[v];
            if (phase[v] >= (uint64_t(endFrame) << 32)) {
              if (loop) {
                uint64_t span = uint64_t(endFrame - startFrame) << 32;
                phase[v] =
                    (uint64_t(startFrame) << 32) + (phase[v] - (uint64_t(startFrame) << 32)) % span;
                wraps++;
              } else
                active[v] = false;
            }
          }
      if (!any || position >= limit)
        done = true;
      unsigned ramp = position < fade ? position : fade;
      if (limit - position < ramp)
        ramp = limit - position;
      for (int ch = 0; ch < 2; ch++)
        out[frame * 2 + ch] =
            done ? 0 : int16_t(int64_t(sum[ch]) * ramp / (int64_t(fade) * voices * 2));
      if (!done)
        position++;
      if (position >= limit)
        done = true;
    }
  }
};
} // namespace lab
#endif
