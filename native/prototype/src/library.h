#pragma once
#include "storage.h"
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <dirent.h>
#endif
 #include "sample_catalog.h"
namespace moss {
// Paths only: increasing the source catalog does not load more PCM into RAM.
constexpr int Assets = 32, Sources = 2048, SourcePageRows = 7;
inline void makeDir(const char *path) {
#ifdef _WIN32
  _mkdir(path);
#else
  mkdir(path, 0755);
#endif
}
inline void put16(FILE *f, unsigned v) {
  fputc(v & 255, f);
  fputc(v >> 8, f);
}
inline void put32(FILE *f, unsigned v) {
  put16(f, v & 65535);
  put16(f, v >> 16);
}
inline bool writeWav(const char *path, const int16_t *data, unsigned frames,
                     unsigned rate, unsigned channels) {
  char temp[512];
  snprintf(temp, sizeof(temp), "%s.tmp", path);
  FILE *f = fopen(temp, "wb");
  if (!f)
    return false;
  unsigned bytes = frames * channels * 2;
  fwrite("RIFF", 1, 4, f);
  put32(f, bytes + 36);
  fwrite("WAVEfmt ", 1, 8, f);
  put32(f, 16);
  put16(f, 1);
  put16(f, channels);
  put32(f, rate);
  put32(f, rate * channels * 2);
  put16(f, channels * 2);
  put16(f, 16);
  fwrite("data", 1, 4, f);
  put32(f, bytes);
  // All supported targets are little endian; this file format is explicitly LE.
  bool ok = fwrite(data, 1, bytes, f) == bytes && !ferror(f);
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
struct WavSource {
  char path[512] = {}, error[100] = {};
  int peaks[64] = {};
  unsigned rate = 0, channels = 0, bits = 0, format = 0, frames = 0, offset = 0;
  bool open(const char *filename) {
    rate = frames = 0;
    error[0] = 0;
    snprintf(path, sizeof(path), "%s", filename);
    FILE *f = fopen(path, "rb");
    if (!f) {
      strcpy(error, "CANNOT OPEN WAV");
      return false;
    }
    auto fail = [&](const char *s) {
      snprintf(error, sizeof(error), "%s", s);
      fclose(f);
      return false;
    };
    fseek(f, 0, SEEK_END);
    long bytes = ftell(f);
    rewind(f);
    unsigned char h[40];
    if (bytes < 44 || fread(h, 1, 12, f) != 12 || memcmp(h, "RIFF", 4) ||
        memcmp(h + 8, "WAVE", 4))
      return fail("EXPECTED RIFF WAV");
    uint64_t end = uint64_t(lab::little32(h + 4)) + 8;
    if (end > unsigned(bytes))
      return fail("TRUNCATED WAV");
    unsigned data = 0;
    bool fmt = false, found = false;
    for (uint64_t cursor = 12; cursor + 8 <= end;) {
      if (fseek(f, long(cursor), SEEK_SET) || fread(h, 1, 8, f) != 8)
        return fail("WAV READ ERROR");
      unsigned size = lab::little32(h + 4);
      uint64_t next = cursor + 8 + size + (size & 1);
      if (next > end)
        return fail("INVALID WAV CHUNK");
      if (!memcmp(h, "fmt ", 4)) {
        if (fmt || size < 16 || fread(h, 1, 16, f) != 16)
          return fail("INVALID FORMAT");
        format = lab::little16(h);
        channels = lab::little16(h + 2);
        rate = lab::little32(h + 4);
        bits = lab::little16(h + 14);
        if ((channels != 1 && channels != 2) || rate < 8000 || rate > 96000 ||
            !((format == 1 && (bits == 16 || bits == 24 || bits == 32)) ||
              (format == 3 && bits == 32)))
          return fail("USE PCM16/24/32 OR FLOAT32 WAV");
        if (lab::little16(h + 12) != channels * (bits / 8))
          return fail("BAD WAV ALIGNMENT");
        fmt = true;
      } else if (!memcmp(h, "data", 4)) {
        if (found)
          return fail("DUPLICATE DATA");
        offset = unsigned(cursor + 8);
        data = size;
        found = true;
      }
      cursor = next;
    }
    if (!fmt || !found || !data || data % (channels * (bits / 8)))
      return fail("MISSING PCM DATA");
    frames = data / (channels * (bits / 8));
    fclose(f);
    return true;
  }
  void overview() {
    memset(peaks, 0, sizeof(peaks));
    FILE *f = fopen(path, "rb");
    if (!f)
      return;
    // A bounded overview: 64 windows of 128 frames, independent of source
    // length.
    unsigned char b[1024];
    unsigned stride = channels * (bits / 8);
    for (unsigned k = 0; k < 64; k++) {
      unsigned first = unsigned(uint64_t(k) * frames / 64), n = frames - first;
      if (n > 128)
        n = 128;
      if (fseek(f, long(offset + uint64_t(first) * stride), SEEK_SET) ||
          fread(b, stride, n, f) != n)
        continue;
      for (unsigned i = 0; i < n * channels; i++) {
        unsigned char *v = b + i * (bits / 8);
        int sample = 0;
        if (format == 3) {
          uint32_t raw = lab::little32(v);
          float x;
          memcpy(&x, &raw, 4);
          if (isfinite(x))
            sample = clamp(int((x > 1    ? 1
                                : x < -1 ? -1
                                         : x) *
                               32768),
                           -32768, 32767);
        } else if (bits == 16)
          sample = int16_t(lab::little16(v));
        else if (bits == 24)
          sample = int16_t(lab::little16(v + 1));
        else
          sample = int32_t(lab::little32(v)) / 65536;
        if (sample < 0)
          sample = -sample;
        if (sample > peaks[k])
          peaks[k] = sample;
      }
    }
    fclose(f);
  }
  bool extract(moss::SampleBuffer &output, unsigned first, unsigned last,
               int fadeInMs, int fadeOutMs) {
    if (first >= last || last > frames) {
      strcpy(error, "INVALID START / END");
      return false;
    }
    unsigned n = last - first, bytes = n * channels * 2;
    if (bytes > moss::SampleBudget) {
      strcpy(error, "SLICE TOO LARGE / CUT SMALLER THAN 1 MIB");
      return false;
    }
    FILE *f = fopen(path, "rb");
    if (!f) {
      strcpy(error, "SOURCE NOT AVAILABLE");
      return false;
    }
    if (fseek(f, long(offset + uint64_t(first) * channels * (bits / 8)),
              SEEK_SET)) {
      fclose(f);
      return false;
    }
    unsigned in = unsigned(fadeInMs) * rate / 1000,
             out = unsigned(fadeOutMs) * rate / 1000;
    for (unsigned i = 0; i < n; i++)
      for (unsigned ch = 0; ch < channels; ch++) {
        unsigned char b[4] = {};
        if (fread(b, 1, bits / 8, f) != bits / 8) {
          strcpy(error, "TRUNCATED PCM");
          fclose(f);
          return false;
        }
        int value = 0;
        if (format == 3) {
          uint32_t raw = lab::little32(b);
          float v;
          memcpy(&v, &raw, 4);
          if (!isfinite(v))
            v = 0;
          value = clamp(int((v > 1    ? 1
                             : v < -1 ? -1
                                      : v) *
                            32768),
                        -32768, 32767);
        } else if (bits == 16)
          value = int(int16_t(lab::little16(b)));
        else if (bits == 24) {
          int32_t raw = int32_t(unsigned(b[0]) | unsigned(b[1]) << 8 |
                                unsigned(b[2]) << 16);
          if (raw & 0x800000)
            raw -= 0x1000000;
          value = raw / 256;
        } else
          value = int(int32_t(lab::little32(b)) / 65536);
        if (in && i < in)
          value = int(int64_t(value) * i / in);
        if (out && n - 1 - i < out)
          value = int(int64_t(value) * (n - 1 - i) / out);
        output.pcm[i * channels + ch] = int16_t(value);
      }
    fclose(f);
    output.rate = rate;
    output.channels = channels;
    output.frames = n;
    output.bytes = bytes;
    memset(output.peaks, 0, sizeof(output.peaks));
    for (unsigned i = 0; i < n; i++)
      for (unsigned ch = 0; ch < channels; ch++) {
        int v = output.pcm[i * channels + ch];
        if (v < 0)
          v = -v;
        unsigned bin = unsigned(uint64_t(i) * 64 / n);
        if (v > output.peaks[bin])
          output.peaks[bin] = v;
      }
    return true;
  }
};
struct Library {
  moss::SampleBuffer pcm[Assets], scratch;
  char names[CatalogAssets][32] = {}, sourcePaths[Sources][512] = {};
  char assetFiles[CatalogAssets][512] = {};
  SampleView views[CatalogAssets];
  unsigned catalogBytes[CatalogAssets] = {};
  int resident[Assets];
  bool catalogReady = false;
  struct Kit { char name[25] = {}; int ids[4] = {-1,-1,-1,-1}; } kits[CatalogAssets / 4];
  int kitCount = 0;
  Library() { for (int &id : resident) id = -1; }
  bool has(int id) const { return id>=0 && id<CatalogAssets &&
    (assetFiles[id][0] || (!catalogReady && id<Assets && pcm[id].frames)); }
  unsigned assetBytes(int id) const { return catalogReady ? catalogBytes[id] : id<Assets ? pcm[id].bytes : 0; }
  SampleView view(int id) const { return catalogReady ? views[id] : id<Assets ? static_cast<const SampleView &>(pcm[id]) : SampleView{}; }
  void syncViews() {
    for (auto &v : views) v = SampleView{};
    for (int i=0;i<Assets;++i) if (resident[i]>=0) views[resident[i]]=pcm[i];
  }
  bool loadOne(int id, const AssetMask &pinned) {
    if (id<0 || id>=CatalogAssets || !has(id)) { strcpy(error,"SAMPLE FILE MISSING / CHECK LIBRARY"); return false; }
    for (int i=0;i<Assets;++i) if (resident[i]==id && pcm[i].frames) return true;
    int slot=-1;
    for (int i=0;i<Assets;++i) if (resident[i]<0) {slot=i;break;}
    if(slot<0) for(int i=0;i<Assets;++i) if(!pinned.has(resident[i])) {slot=i;break;}
    if(slot<0) {strcpy(error,"32 LOADED SAMPLES MAX / REDUCE SONG");return false;}
    if(!scratch.load(assetFiles[id])) {snprintf(error,sizeof(error),"%s",scratch.error);return false;}
    pcm[slot]=scratch; resident[slot]=id; syncViews(); return true;
  }
  bool prepare(const AssetMask &needed) {
    if(!catalogReady) {
      for(int i=0;i<Assets;++i) views[i]=pcm[i];
      return true; // In-memory test/preview fixtures have no disk catalog.
    }
    int count=0;
    for(int id=0;id<CatalogAssets;++id) if(needed.has(id)) ++count;
    if(count>Assets) {strcpy(error,"SONG NEEDS MORE THAN 32 SAMPLES");return false;}
    for(int id=0;id<CatalogAssets;++id) if(needed.has(id) && !loadOne(id,needed)) return false;
    syncViews();return true;
  }
  bool prepare(const Project &p) { return prepare(projectAssets(p)); }
  int freeId() const {for(int id=0;id<CatalogAssets;++id)if(!has(id))return id;return -1;}

  int sourceCount = 0;
  char error[100] = {};
  void assetPath(char *p, int n, int id) {
    if(id>=0 && id<CatalogAssets && assetFiles[id][0]) snprintf(p,n,"%s",assetFiles[id]);
    else snprintf(p, n, "library/asset-%02d.wav", id);
  }
  void scanFolder(const char *root, int depth = 0) {
    if (depth > 5 || sourceCount == Sources)
      return;
#ifdef _WIN32
    char pattern[512];
    snprintf(pattern, sizeof(pattern), "%s/*", root);
    WIN32_FIND_DATAA d;
    HANDLE h = FindFirstFileA(pattern, &d);
    if (h == INVALID_HANDLE_VALUE)
      return;
    do {
      const char *name = d.cFileName;
      if (name[0] == '.')
        continue;
      char p[512];
      snprintf(p, sizeof(p), "%s/%s", root, name);
      if (d.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        scanFolder(p, depth + 1);
      else
        addSource(p);
    } while (sourceCount < Sources && FindNextFileA(h, &d));
    FindClose(h);
#else
    DIR *dir = opendir(root);
    if (!dir)
      return;
    dirent *d;
    while (sourceCount < Sources && (d = readdir(dir))) {
      if (d->d_name[0] == '.')
        continue;
      char p[512];
      snprintf(p, sizeof(p), "%s/%s", root, d->d_name);
      struct stat st = {};
      if (stat(p, &st))
        continue;
      if (S_ISDIR(st.st_mode))
        scanFolder(p, depth + 1);
      else
        addSource(p);
    }
    closedir(dir);
#endif
  }
  void addSource(const char *p) {
    size_t n = strlen(p);
    if (n > 4 && (p[n - 4] == '.') && (p[n - 3] == 'w' || p[n - 3] == 'W') &&
        (p[n - 2] == 'a' || p[n - 2] == 'A') &&
        (p[n - 1] == 'v' || p[n - 1] == 'V') && sourceCount < Sources)
      snprintf(sourcePaths[sourceCount++], 512, "%s", p);
  }
  void refresh() {
    sourceCount = 0;
    scanFolder("imports");
    scanFolder("captures");
    scanFolder("../../../MOSS-SampleLibrary");
  }
  void rememberSource(const char *path) {
    for (int i=0;i<sourceCount;++i) if(!strcmp(sourcePaths[i],path)) return;
    int last=sourceCount < Sources ? sourceCount++ : Sources-1;
    for(int i=last;i>0;--i) memcpy(sourcePaths[i],sourcePaths[i-1],sizeof(sourcePaths[i]));
    snprintf(sourcePaths[0],sizeof(sourcePaths[0]),"%s",path);
  }
  void init() {
    for(auto &p:assetFiles)p[0]=0;
    for(auto &v:catalogBytes)v=0;
    for(auto &v:views)v=SampleView{};
    for(int &id:resident)id=-1;
    kitCount=0;
    makeDir("library");
    makeDir("imports");
    makeDir("captures");
    bool bootstrap = !exists("library/initialized"), bootOk = true;
    for (int i = 0; i < CatalogAssets; i++) {
      char p[128]; snprintf(p,sizeof(p),"library/asset-%02d.wav",i);
      if (exists(p)) {
        snprintf(assetFiles[i],512,"%s",p);
        WavSource info;
        if(info.open(p)) catalogBytes[i]=info.frames*info.channels*2;
        snprintf(names[i],32,"ASSET %02d",i);
        char label[128];snprintf(label,sizeof(label),"library/asset-%02d.name",i);
        FILE *f=fopen(label,"rb");if(f){fgets(names[i],32,f);fclose(f);names[i][strcspn(names[i],"\r\n")]=0;}
        if(i<Assets && pcm[i].load(p))resident[i]=i;
      } else if(bootstrap && i<3 && pcm[i].load(lab::sampleFiles[i])) {
        snprintf(names[i],32,"%s",lab::sampleNames[i]);
        bool ok=writeWav(p,pcm[i].pcm,pcm[i].frames,pcm[i].rate,pcm[i].channels);
        bootOk=ok && bootOk;
        if(ok) {snprintf(assetFiles[i],512,"%s",p);catalogBytes[i]=pcm[i].bytes;resident[i]=i;saveName(i);}
      }
    }
    catalogReady=true; scanKits(); syncViews();
    if (bootstrap && bootOk) {
      FILE *f = fopen("library/initialized", "wb");
      if (f) {
        fputs("MOSS sample library 1\n", f);
        fflush(f);
#ifdef _WIN32
        _commit(_fileno(f));
#else
        fsync(fileno(f));
#endif
        fclose(f);
      }
    }
    refresh();
  }
  int saveSlice(WavSource &source, unsigned first, unsigned last, int fadeIn,
                int fadeOut, const char *label = nullptr, int suffix = 0) {
    int slot=freeId();
    if(slot<0) {strcpy(error,"LIBRARY FULL / 256 SAMPLES");return -1;}
    if (!source.extract(scratch, first, last, fadeIn, fadeOut)) {
      snprintf(error, sizeof(error), "%s", source.error);
      return -1;
    }
    char p[128];
    assetPath(p, sizeof(p), slot);
    if (!writeWav(p, scratch.pcm, scratch.frames, scratch.rate,
                  scratch.channels)) {
      strcpy(error, "SD WRITE FAILED / SLICE NOT SAVED");
      return -1;
    }
    if (label && label[0]) {
      if (suffix)
        snprintf(names[slot], 32, "%.20s %02d", label, suffix);
      else
        snprintf(names[slot], 32, "%.24s", label);
    } else
      snprintf(names[slot], 32, "SLICE %02d", slot);
    if (!saveName(slot)) {
      remove(p);
      names[slot][0] = 0;
      strcpy(error, "NAME WRITE FAILED / RETRY SAVE");
      return -1;
    }
    snprintf(assetFiles[slot],512,"%s",p);
    catalogBytes[slot]=scratch.bytes;
    if(!catalogReady && slot<Assets) pcm[slot]=scratch;
    if(catalogReady) {AssetMask empty;loadOne(slot,empty);}

    return slot;
  }

  bool saveName(int id) {
    char p[128];
    snprintf(p, sizeof(p), "library/asset-%02d.name", id);
    FILE *f = fopen(p, "wb");
    if (!f)
      return false;
    bool ok = fwrite(names[id], 1, strlen(names[id]), f) == strlen(names[id]);
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
    return ok;
  }
  bool removeAsset(int id) {
    if (id < 0 || id >= CatalogAssets)
      return false;
    char p[128];
    assetPath(p, sizeof(p), id);
    if (kitFor(id)>=0) {strcpy(error,"KIT MEMBER / KEEP THE COMPLETE SET");return false;}
    if (remove(p)) {
      strcpy(error, "DELETE FAILED / FILE KEPT");
      return false;
    }
    if(!catalogReady && id<Assets) pcm[id].frames=pcm[id].bytes=0;
    for(int i=0;i<Assets;++i)if(resident[i]==id){resident[i]=-1;pcm[i].frames=0;}
    assetFiles[id][0]=0;catalogBytes[id]=0;syncViews();
    names[id][0] = 0;
    snprintf(p, sizeof(p), "library/asset-%02d.name", id);
    remove(p);
    return true;
  }
  #include "library_kits.h"
  int count() const {
    int n = 0;
    for (int i=0;i<CatalogAssets;++i) if(has(i)) ++n;
    return n;
  }
  int next(int id, int direction) const {
    for (int i = id + direction; i >= 0 && i < CatalogAssets; i += direction)
      if (has(i))
        return i;
    return id;
  }
  unsigned bytes() const {
    unsigned n = 0;
    for (const auto &s : pcm)
      n += s.bytes;
    return n;
  }
};
} // namespace moss
