#pragma once
#include "model.h"
#include "reel_legacy.h"
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
  bool upgradedReel=false;
  static int storedSynthCount(int version,int type) {
    return version>=18?20:version>=16?(type<2?20:8):version>=15&&type==0?20:8;
  }
  void fields(Project &p, bool writing, int version = 18) {
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
      const int types=version>=9?EffectTypes:4;
      for(int i=0;i<types;++i) value(e.enabledTypes[i]);
      for(int i=0;i<types;++i) value(e.order[i]);
      for(int t=0;t<types;++t) {
        int old[16]={};
        for(int i=0;i<(version>=14?fxParameters(t):version==13?(t==Pond?4:fxParameters(t)):version>=9?8:4);++i) {
          int v=i<fxParameters(t)?e.values[t][i]:0;value(v);old[i]=v;
          if(version>=13 || t!=Reel) {if(i<fxParameters(t) && !(version<14 && t==Pond && i>=4))e.values[t][i]=v;}
        }
        if(!writing && version<13 && t==Reel) {
          for(int i=0;i<4;++i)if(old[i]<0 || old[i]>100)ok=false;
          migrateReel(e.values[Reel],old);
        }
      }
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
        for (int i=0;i<(version>=14?GranularParameterCount:version>=8?16:12);++i) value(t.granular.values[i]);
      }
      for(int type=0;type<3;++type)
        for(int i=0;i<storedSynthCount(version,type);++i)value(t.synthParams[type][i]);
      for (auto &n : t.synthMode)
        value(n);
      for (auto &n : t.synthPreset)
        value(n);
      fx(t.fx);
      value(t.cycle);
      value(t.division);
      if(version>=12) value(t.swing);
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
          for (int i=0;i<(version>=14?GranularParameterCount:version>=8?16:12);++i) value(s.grainLocks[i]);
        for(int type=0;type<3;++type)
          for(int i=0;i<storedSynthCount(version,type);++i)value(s.synthLocks[type][i]);
        for(int type=0;type<(version>=9?EffectTypes:4);++type) {
          int old[16];for(auto &v:old)v=Inherit;
          for(int i=0;i<(version>=14?fxParameters(type):version==13?(type==Pond?4:fxParameters(type)):version>=9?8:4);++i) {
            int v=i<fxParameters(type)?s.fxLocks[type][i]:Inherit;value(v);old[i]=v;
            if(version>=13 || type!=Reel) {if(i<fxParameters(type) && !(version<14 && type==Pond && i>=4))s.fxLocks[type][i]=int16_t(v);}
          }
          if(!writing && version<13 && type==Reel) {
            for(int i=0;i<4;++i)if(lockValue(old[i]) && (old[i]<0 || old[i]>100))ok=false;
            migrateReelLocks(s.fxLocks[Reel],old);
          }
        }
        if(!writing && version<9) {
          int16_t mix=s.fxLocks[Orbit][2];s.fxLocks[Orbit][2]=s.fxLocks[Orbit][3];s.fxLocks[Orbit][3]=mix;
        }
        value(s.on);
        value(s.key);
        value(s.velocity);
        value(s.length);
        value(s.probability);
        if(version>=11) value(s.repeat);
        if(version>=12) value(s.shift);
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
    memcpy(bytes, "MS18", 4);
    word(4, 18);
    word(8, cursor);
    word(12, hash(cursor));
    return ok && valid(p);
  }
  bool decode(Project &p, unsigned size) {
    bool legacy = !memcmp(bytes, "MOS5", 4) && word(4) == 5;
    bool v6 = !memcmp(bytes, "MOS6", 4) && word(4) == 6;
    bool v7 = !memcmp(bytes, "MOS7", 4) && word(4) == 7;
    bool v8 = !memcmp(bytes, "MOS8", 4) && word(4) == 8;
    bool v9 = !memcmp(bytes, "MOS9", 4) && word(4) == 9;
    bool v10 = !memcmp(bytes, "MS10", 4) && word(4) == 10;
    bool v11 = !memcmp(bytes, "MS11", 4) && word(4) == 11;
    bool v12 = !memcmp(bytes, "MS12", 4) && word(4) == 12;
    bool v13 = !memcmp(bytes, "MS13", 4) && word(4) == 13;
    bool v14 = !memcmp(bytes, "MS14", 4) && word(4) == 14;
    bool v17 = !memcmp(bytes, "MS17", 4) && word(4) == 17;
    bool v16 = !memcmp(bytes, "MS16", 4) && word(4) == 16;
    bool v15 = !memcmp(bytes, "MS15", 4) && word(4) == 15;
    if (size < 16 || size > sizeof(bytes) ||
        (!legacy && !v6 && !v7 && !v8 && !v9 && !v10 && !v11 && !v12 && !v13 && !v14 && !v15 && !v16 && !v17 && (memcmp(bytes, "MS18", 4) || word(4) != 18)) ||
        word(8) != size || word(12) != hash(size))
      return false;
    Project temp;
    cursor = 16;
    ok = true;
    int version=legacy?5:v6?6:v7?7:v8?8:v9?9:v10?10:v11?11:v12?12:v13?13:v14?14:v15?15:v16?16:v17?17:18;
    fields(temp,false,version);
    if(version<18)for(auto &t:temp.tracks){
      for(int i=8;i<FmParameters;++i)t.synthParams[2][i]=FmPresets[0][i];
      t.synthParams[2][FmModDecay]=t.synthParams[2][FmDecay];
      t.synthParams[2][FmModHold]=t.synthParams[2][FmBody];
      t.synthParams[2][FmRelease]=t.synthParams[2][FmDecay]/2;
      t.synthParams[2][FmDynamics]=0;t.synthParams[2][FmClick]=0;
      t.synthParams[2][FmSpread]=0;t.synthParams[2][FmMotion]=0;
      t.synthPreset[2]=-1; // same controls retained; revised DSP is not bit-identical.
    }
    if(version<16)for(auto &t:temp.tracks){
      const int map[]={OrgShape,OrgDrift,OrgCouple,OrgDensity,OrgCutoff,OrgNoise,OrgAttack,OrgRelease};
      auto convert=[](int index,int value){return index==1?value*30/100:index==4?100+value*value*12/10:value;};
      int old[8];memcpy(old,t.synthParams[1],sizeof(old));
      memcpy(t.synthParams[1],OrganismPresets[0],sizeof(t.synthParams[1]));
      for(int i=0;i<8;++i){if(old[i]<0||old[i]>(i==6?5000:i==7?10000:100))ok=false;else t.synthParams[1][map[i]]=convert(i,old[i]);}
      t.synthPreset[1]=-1;
      for(auto &s:t.steps){
        int16_t oldLocks[8];memcpy(oldLocks,s.synthLocks[1],sizeof(oldLocks));
        for(auto &v:s.synthLocks[1])v=Inherit;
        for(int i=0;i<8;++i){int v=oldLocks[i];if(v==Inherit||v==ResetLock)s.synthLocks[1][map[i]]=int16_t(v);
          else if(v<0||v>(i==6?5000:i==7?10000:100))ok=false;else s.synthLocks[1][map[i]]=int16_t(convert(i,v));}
      }
    }
    if(version<15)for(auto &t:temp.tracks){
      const int extra[]={50,0,0,0,500,100,80+t.synthParams[0][3]*2/5,0,30,0,0,0};
      for(int i=8;i<SynthParameterCount;++i)t.synthParams[0][i]=extra[i-8];
    }
    if(version<14) for(auto &t:temp.tracks) for(int i=16;i<GranularParameterCount;++i)t.granular.values[i]=0;
    // The old timing-irregularity control must not silently become head jumps.
    if (legacy || v6 || v7 || v8 || v9) for (auto &t : temp.tracks) {
      if (t.granular.values[9] < 0 || t.granular.values[9] > 100) ok = false;
      t.granular.values[9] = 0;
      for (auto &s : t.steps) {
        if (s.grainLocks[9] != Inherit && (s.grainLocks[9] < 0 || s.grainLocks[9] > 100)) ok = false;
        s.grainLocks[9] = Inherit;
      }
    }
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
    upgradedReel=version<13;
    return true;
  }
};
inline bool loadProject(const char *path, Project &p,bool *adaptedReel=nullptr) {
  if(adaptedReel)*adaptedReel=false;
  static Codec c;
  FILE *f = fopen(path, "rb");
  if (!f)
    return false;
  unsigned size = unsigned(fread(c.bytes, 1, sizeof(c.bytes), f));
  bool extra = fgetc(f) != EOF;
  bool error = ferror(f);
  fclose(f);
  bool loaded=!extra && !error && c.decode(p,size);
  if(loaded && adaptedReel && c.upgradedReel) {
    *adaptedReel=p.fx.active(Reel);
    for(int t=0;t<p.trackCount;++t)*adaptedReel|=p.tracks[t].fx.active(Reel);
  }
  return loaded;
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
