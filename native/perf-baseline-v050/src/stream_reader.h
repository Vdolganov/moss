#pragma once
#include "library.h"
#include "stream_preview.h"
namespace moss {
// UI-side file lifetime; begin/close/reset happen with audio paused. pump only
// publishes completed frames and never reuses a slot until the consumer frees
// it.
struct StreamReader {
  WavSource source;
  FILE *file = nullptr;
  unsigned supplied = 0, total = 0, fadeIn = 0, fadeOut = 0;
  bool failed = false;
  void close() {
    if (file)
      fclose(file);
    file = nullptr;
  }
  static int decode(const unsigned char *b, unsigned bits, unsigned format) {
    if (format == 3) {
      uint32_t raw = lab::little32(b);
      float v;
      memcpy(&v, &raw, 4);
      if (!isfinite(v))
        return 0;
      return clamp(int(fmaxf(-1, fminf(1, v)) * 32768), -32768, 32767);
    }
    if (bits == 16)
      return int16_t(lab::little16(b));
    if (bits == 24) {
      int32_t raw = int32_t(unsigned(b[0]) | (unsigned(b[1]) << 8) |
                            (unsigned(b[2]) << 16));
      if (raw & 0x800000)
        raw -= 0x1000000;
      return raw / 256;
    }
    return int32_t(lab::little32(b)) / 65536;
  }
  bool begin(StreamPreview &stream, const char *path, unsigned outputRate,
             unsigned first, unsigned last, int inMs = 0, int outMs = 0) {
    close();
    failed = false;
    if (!source.open(path))
      return false;
    if (!last)
      last = source.frames;
    if (first >= last || last > source.frames) {
      strcpy(source.error, "INVALID PREVIEW RANGE");
      return false;
    }
    file = fopen(path, "rb");
    if (!file || fseek(file,
                       long(source.offset + uint64_t(first) * source.channels *
                                                (source.bits / 8)),
                       SEEK_SET)) {
      close();
      strcpy(source.error, "CANNOT READ SOURCE");
      return false;
    }
    supplied = 0;
    total = last - first;
    fadeIn = unsigned(inMs) * source.rate / 1000;
    fadeOut = unsigned(outMs) * source.rate / 1000;
    // A short edge ramp is also applied when a trim has no explicit fade.
    if (fadeIn < 64)
      fadeIn = 64;
    if (fadeOut < 64)
      fadeOut = 64;
    stream.begin(source.rate, outputRate, first, total);
    pump(stream);
    if (failed) {
      stream.reset();
      return false;
    }
    __atomic_store_n(&stream.state, 1, __ATOMIC_RELEASE);
    return true;
  }
  void pump(StreamPreview &stream) {
    if (!file)
      return;
    unsigned char raw[8192];
    unsigned stride = source.channels * (source.bits / 8);
    unsigned r = __atomic_load_n(&stream.readIndex, __ATOMIC_ACQUIRE),
             w = __atomic_load_n(&stream.writeIndex, __ATOMIC_RELAXED);
    unsigned space = StreamPreview::Capacity - (w - r);
    while (space && supplied < total) {
      unsigned n = unsigned(sizeof(raw)) / stride;
      if (n > space)
        n = space;
      if (n > total - supplied)
        n = total - supplied;
      unsigned got = unsigned(fread(raw, stride, n, file));
      for (unsigned i = 0; i < got; i++)
        for (unsigned ch = 0; ch < 2; ch++) {
          int value =
              decode(raw + i * stride +
                         (source.channels == 1 ? 0 : ch) * (source.bits / 8),
                     source.bits, source.format);
          unsigned at = supplied + i;
          if (at < fadeIn)
            value = int(int64_t(value) * at / fadeIn);
          if (total - 1 - at < fadeOut)
            value = int(int64_t(value) * (total - 1 - at) / fadeOut);
          stream.pcm[(w + i) % StreamPreview::Capacity][ch] = int16_t(value);
        }
      w += got;
      supplied += got;
      space -= got;
      __atomic_store_n(&stream.writeIndex, w, __ATOMIC_RELEASE);
      if (got != n) {
        failed = true;
        strcpy(source.error, "SOURCE READ ERROR / PREVIEW STOPPED");
        break;
      }
    }
    if (supplied == total || failed) {
      close();
      __atomic_store_n(&stream.eof, 1, __ATOMIC_RELEASE);
    }
  }
};
} // namespace moss
