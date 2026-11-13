#pragma once
#include "song.h"
#include "storage.h"
namespace moss {
inline void songPath(char *out, int n, int index) {
  snprintf(out, n, "projects/song-%d.moss", index + 1);
}
struct SongCodec {
  static constexpr unsigned Size = 16 + 25 + 8 + SongSlots * 6;
  unsigned char bytes[Size] = {};
  void put(unsigned at, unsigned v) {
    bytes[at] = v & 255;
    bytes[at + 1] = (v >> 8) & 255;
  }
  unsigned get(unsigned at) const {
    return bytes[at] | unsigned(bytes[at + 1]) << 8;
  }
  uint32_t hash() const {
    uint32_t h = 2166136261u;
    for (unsigned i = 16; i < Size; ++i) {
      h ^= bytes[i];
      h *= 16777619u;
    }
    return h;
  }
  bool encode(const Song &s) {
    if (!valid(s))
      return false;
    memset(bytes, 0, Size);
    memcpy(bytes, "SON1", 4);
    put(4, Size);
    memcpy(bytes + 16, s.name, 25);
    put(41, s.bpm);
    put(43, s.count);
    put(45, s.loop);
    for (int i = 0; i < s.count; ++i) {
      put(49 + i * 6, s.slots[i].sequence);
      put(51 + i * 6, s.slots[i].repeats);
      put(53 + i * 6, s.slots[i].bars);
    }
    uint32_t h = hash();
    put(8, h);
    put(10, h >> 16);
    return true;
  }
  bool decode(Song &s) const {
    if (memcmp(bytes, "SON1", 4) || get(4) != Size ||
        lab::little32(bytes + 8) != hash())
      return false;
    Song next;
    memcpy(next.name, bytes + 16, 25);
    next.bpm = get(41);
    next.count = get(43);
    next.loop = get(45);
    if (next.count > SongSlots)
      return false;
    for (int i = 0; i < next.count; ++i)
      next.slots[i] = {int(get(49 + i * 6)), int(get(51 + i * 6)),
                       int(get(53 + i * 6))};
    if (!valid(next))
      return false;
    s = next;
    return true;
  }
};
inline bool loadSong(const char *path, Song &s) {
  SongCodec c;
  FILE *f = fopen(path, "rb");
  if (!f)
    return false;
  unsigned size = unsigned(fread(c.bytes, 1, sizeof(c.bytes), f));
  bool ok = size == sizeof(c.bytes) && fgetc(f) == EOF && !ferror(f);
  fclose(f);
  return ok && c.decode(s);
}
inline bool saveSong(const char *path, const Song &s) {
  SongCodec c;
  return c.encode(s) && writeAtomic(path, c.bytes, sizeof(c.bytes));
}
} // namespace moss
