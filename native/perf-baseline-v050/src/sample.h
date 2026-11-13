#pragma once
#include "../../../hardware lab/src/lab_sample.h"
namespace moss {
using lab::little16;
using lab::little32;
constexpr unsigned SampleBudget = 1024 * 1024;
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
    if (length < 12 || length > long(SampleBudget + 4096))
      return fail("WAV EXCEEDS ASSET BUDGET");
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
        if ((fmtChannels != 1 && fmtChannels != 2) || fmtRate < 8000 ||
            fmtRate > 96000)
          return fail("Use mono/stereo WAV at 8..96 kHz");
        if (little16(h + 12) != fmtChannels * 2 ||
            little32(h + 8) != fmtRate * fmtChannels * 2)
          return fail("Inconsistent PCM format");
        haveFmt = true;
      } else if (!memcmp(h, "data", 4)) {
        if (haveData || size == 0 || size > SampleBudget)
          return fail("PCM EXCEEDS 1 MIB");
        dataOffset = unsigned(payload);
        dataBytes = size;
        haveData = true;
      }
      cursor = next;
    }
    if (!haveFmt || !haveData || dataBytes % (fmtChannels * 2))
      return fail("Missing or unaligned PCM data");
    if (fseek(f, dataOffset, SEEK_SET) ||
        fread(pcm, 1, dataBytes, f) != dataBytes)
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
} // namespace moss
