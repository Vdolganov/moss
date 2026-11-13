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
  void fields(Project &p, bool writing, int version = 9) {
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
      const int types=version>=9?EffectTypes:4, parameters=version>=9?EffectParameters:4;
      for(int i=0;i<types;++i) value(e.enabledTypes[i]);
      for(int i=0;i<types;++i) value(e.order[i]);
      for(int t=0;t<types;++t) for(int i=0;i<parameters;++i) value(e.values[t][i]);
      if(!writing && version<9) {
        int mix=e.values[Orbit][2];e.values[Orbit][2]=e.values[Orbit][3];e.values[Orbit][3]=mix;
      }
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
      if (version >= 7) {
        value(t.granular.asset);
        value(t.granular.freeze);
        for (int i=0;i<(version>=8?GranularParameterCount:12);++i) value(t.granular.values[i]);
      }
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
        if (version >= 7)
          for (int i=0;i<(version>=8?GranularParameterCount:12);++i) value(s.grainLocks[i]);
        for (auto &row : s.synthLocks)
          for (auto &n : row)
            value(n);
        for(int type=0;type<(version>=9?EffectTypes:4);++type)
          for(int i=0;i<(version>=9?EffectParameters:4);++i) value(s.fxLocks[type][i]);
        if(!writing && version<9) {
          int16_t mix=s.fxLocks[Orbit][2];s.fxLocks[Orbit][2]=s.fxLocks[Orbit][3];s.fxLocks[Orbit][3]=mix;
        }
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
    memcpy(bytes, "MOS9", 4);
    word(4, 9);
    word(8, cursor);
    word(12, hash(cursor));
    return ok && valid(p);
  }
  bool decode(Project &p, unsigned size) {
    bool legacy = !memcmp(bytes, "MOS5", 4) && word(4) == 5;
    bool v6 = !memcmp(bytes, "MOS6", 4) && word(4) == 6;
    bool v7 = !memcmp(bytes, "MOS7", 4) && word(4) == 7;
    bool v8 = !memcmp(bytes, "MOS8", 4) && word(4) == 8;
    if (size < 16 || size > sizeof(bytes) ||
        (!legacy && !v6 && !v7 && !v8 && (memcmp(bytes, "MOS9", 4) || word(4) != 9)) ||
        word(8) != size || word(12) != hash(size))
      return false;
    Project temp;
    cursor = 16;
    ok = true;
    fields(temp, false, legacy ? 5 : v6 ? 6 : v7 ? 7 : v8 ? 8 : 9);
    // Experimental MOS7 used milliseconds in slot 7. Never interpret them as
    // percentages. Upgrade that control to the new cloud default; old jitter
    // locks have no source-independent equivalent and are explicitly discarded.
    if (v7) for (auto &t : temp.tracks) {
      if (t.granular.values[7] < 0 || t.granular.values[7] > 500) ok = false;
      t.granular.values[7] = t.granular.values[7] ? 100 : 0;
      if (t.granular.values[11] < 0 || t.granular.values[11] > 9000) ok = false;
      t.granular.values[14] = t.granular.values[15] = t.granular.values[11];
      t.granular.values[11] = 100;
      for (auto &s : t.steps) {
        if (s.grainLocks[7] != Inherit && (s.grainLocks[7] < 0 || s.grainLocks[7] > 500)) ok = false;
        s.grainLocks[7] = Inherit;
        if (s.grainLocks[11] != Inherit && (s.grainLocks[11] < 0 || s.grainLocks[11] > 9000)) ok = false;
        s.grainLocks[14] = s.grainLocks[15] = s.grainLocks[11];
        s.grainLocks[11] = Inherit;
      }
    }
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
