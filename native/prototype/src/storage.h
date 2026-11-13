#pragma once
#include "model.h"
#include <stdio.h>
#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif
namespace moss {
struct Codec {
  unsigned char bytes[2 * 1024 * 1024] = {};
  unsigned cursor = 16;
  bool ok = true;
  void put(int n) {
    if (cursor + 2 > sizeof(bytes)) {
      ok = false;
      return;
    }
    bytes[cursor++] = n & 255;
    bytes[cursor++] = (n >> 8) & 255;
  }
  int get() {
    if (cursor + 2 > sizeof(bytes)) {
      ok = false;
      return 0;
    }
    unsigned n = bytes[cursor] | unsigned(bytes[cursor + 1]) << 8;
    cursor += 2;
    return n >= 32768 ? int(n) - 65536 : int(n);
  }
  uint32_t hash(unsigned size) const {
    uint32_t h = 2166136261u;
    for (unsigned i = 16; i < size; i++) {
      h ^= bytes[i];
      h *= 16777619u;
    }
    return h;
  }
  void word(unsigned at, uint32_t n) {
    for (int i = 0; i < 4; i++)
      bytes[at + i] = (n >> (i * 8)) & 255;
  }
  uint32_t word(unsigned at) const { return lab::little32(bytes + at); }
  void fields(Project &p, bool writing, int version = 6) {
    constexpr int LegacyTracks = 8, LegacyKeys = 12;
    const bool legacy = version == 5;
    auto value = [&](auto &n) {
      if (writing)
        put(n);
      else
        n = get();
    };
    auto fx = [&](Effect &e) {
      value(e.type);
      value(e.enabled);
      for (auto &n : e.enabledTypes)
        value(n);
      for (auto &n : e.order)
        value(n);
      for (auto &row : e.values)
        for (auto &v : row)
          value(v);
    };
    auto name = [&](char *text) {
      for (int i = 0; i < 25; i++) {
        int n = writing ? static_cast<unsigned char>(text[i]) : 0;
        value(n);
        if (!writing) {
          if (n < 0 || n > 126)
            ok = false;
          text[i] = char(n);
        }
      }
    };
    name(p.name);
    value(p.trackCount);
    fx(p.fx);
    value(p.bpm);
    value(p.master);
    int lo = int(p.seed & 65535), hi = int(p.seed >> 16);
    value(lo);
    value(hi);
    if (!writing)
      p.seed = uint16_t(lo) | (uint32_t(uint16_t(hi)) << 16);
    Track discarded;
    Hit legacyHits[LegacyKeys][Steps];
    Key legacyKeys[LegacyKeys];
    for (int track = 0; track < (legacy ? LegacyTracks : Tracks); ++track) {
      Track &t = track < Tracks ? p.tracks[track] : discarded;
      name(t.name);
      value(t.instrument);
      value(t.audition);
      for (auto &row : t.synthParams)
        for (auto &n : row)
          value(n);
      for (auto &n : t.synthMode)
        value(n);
      for (auto &n : t.synthPreset)
        value(n);
      fx(t.fx);
      value(t.cycle);
      value(t.division);
      for (int key = 0; key < (legacy ? LegacyKeys : Keys); ++key)
        for (int step = 0; step < Steps; ++step) {
          Hit &h = legacy ? legacyHits[key][step] : t.hits[key][step];
          value(h.on);
          value(h.velocity);
          value(h.probability);
          value(h.fadeIn);
          value(h.fadeOut);
          for (auto &n : h.locks)
            value(n);
          if (!legacy)
            value(h.length);
          else
            h.length = 0;
        }
      value(t.level);
      value(t.pan);
      value(t.mute);
      for (int key = 0; key < (legacy ? LegacyKeys : Keys); ++key) {
        Key &k = legacy ? legacyKeys[key] : t.keys[key];
        value(k.asset);
        value(k.pitch);
        value(k.start);
        value(k.end);
        value(k.level);
        value(k.mode);
      }
      int keyMap[LegacyKeys];
      for (int &v : keyMap)
        v = -1;
      if (legacy) {
        int to = 0;
        for (int from = 0; from < LegacyKeys && to < Keys; ++from)
          if (legacyKeys[from].asset >= 0) {
            keyMap[from] = to;
            t.keys[to] = legacyKeys[from];
            memcpy(t.hits[to], legacyHits[from], sizeof(t.hits[to]));
            ++to;
          }
        for (; to < Keys; ++to) {
          t.keys[to] = Key{};
          for (auto &h : t.hits[to])
            h = Hit{};
        }
        t.fx.constrain();
      }
      for (auto &s : t.steps) {
        value(s.note);
        for (auto &row : s.synthLocks)
          for (auto &n : row)
            value(n);
        for (auto &row : s.fxLocks)
          for (auto &v : row)
            value(v);
        value(s.on);
        value(s.key);
        value(s.velocity);
        value(s.length);
        value(s.probability);
        for (auto &n : s.locks)
          value(n);
        if (legacy)
          s.key = s.key >= 0 && s.key < LegacyKeys && keyMap[s.key] >= 0
                      ? keyMap[s.key]
                      : 0;
      }
    }
    if (legacy) {
      if (p.trackCount > Tracks)
        p.trackCount = Tracks;
      p.fx.constrain();
    }
  }
  bool encode(const Project &p) {
    cursor = 16;
    ok = true;
    fields(const_cast<Project &>(p), true);
    memcpy(bytes, "MOS6", 4);
    word(4, 6);
    word(8, cursor);
    word(12, hash(cursor));
    return ok && valid(p);
  }
  bool decode(Project &p, unsigned size) {
    bool legacy = !memcmp(bytes, "MOS5", 4) && word(4) == 5;
    if (size < 16 || size > sizeof(bytes) ||
        (!legacy && (memcmp(bytes, "MOS6", 4) || word(4) != 6)) ||
        word(8) != size || word(12) != hash(size))
      return false;
    Project temp;
    cursor = 16;
    ok = true;
    fields(temp, false, legacy ? 5 : 6);
    if (!ok || cursor != size || !valid(temp))
      return false;
    p = temp;
    return true;
  }
};
inline bool loadProject(const char *path, Project &p) {
  static Codec c;
  FILE *f = fopen(path, "rb");
  if (!f)
    return false;
  unsigned size = unsigned(fread(c.bytes, 1, sizeof(c.bytes), f));
  bool extra = fgetc(f) != EOF;
  bool error = ferror(f);
  fclose(f);
  return !extra && !error && c.decode(p, size);
}
inline bool exists(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return false;
  fclose(f);
  return true;
}
inline bool writeAtomic(const char *path, const void *bytes, unsigned size) {
  char temp[512];
  snprintf(temp, sizeof(temp), "%s.tmp", path);
  FILE *f = fopen(temp, "wb");
  if (!f)
    return false;
  bool ok = fwrite(bytes, 1, size, f) == size;
  if (fflush(f))
    ok = false;
#ifdef _WIN32
  if (_commit(_fileno(f)))
    ok = false;
#else
  if (fsync(fileno(f)))
    ok = false;
#endif
  if (fclose(f))
    ok = false;
  if (!ok)
    return false;
#ifdef _WIN32
  return MoveFileExA(temp, path,
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
  return rename(temp, path) == 0;
#endif
}
inline bool saveProject(const char *path, const Project &p) {
  static Codec c;
  if (!c.encode(p))
    return false;
  return writeAtomic(path, c.bytes, c.cursor);
}
} // namespace moss
