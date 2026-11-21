#pragma once
namespace moss {
struct AssetMask {
  uint32_t words[CatalogAssets / 32] = {};
  AssetMask(uint32_t value = 0) {
    if (value == ~uint32_t(0)) for (auto &w : words) w = value;
    else words[0] = value;
  }
  bool has(int id) const { return id >= 0 && id < CatalogAssets && (words[id / 32] & (1u << (id % 32))); }
  void add(int id) { if (id >= 0 && id < CatalogAssets) words[id / 32] |= 1u << (id % 32); }
  uint32_t operator&(uint32_t v) const { return words[0] & v; }
};
inline AssetMask projectAssets(const Project &p) {
  AssetMask set;
  for (int t = 0; t < p.trackCount; ++t) {
    set.add(p.tracks[t].granular.asset);
    for (const auto &key : p.tracks[t].keys) set.add(key.asset);
  }
  return set;
}
// Only names/paths are enumerated. WAV data is read on explicit open/audition.
struct FolderBrowser {
  static constexpr int Capacity = 512, Depth = 16;
  struct Entry { char name[256] = {}; bool folder = false; } entries[Capacity];
  char path[512] = {}, parents[Depth][512] = {};
  int cursors[Depth] = {}, depth = 0, count = 0;
  bool truncated = false;
  char error[80] = {};
  static bool wav(const char *s) {
    size_t n = strlen(s);
    return n > 4 && s[n-4] == '.' && (s[n-3] == 'w' || s[n-3] == 'W') &&
      (s[n-2] == 'a' || s[n-2] == 'A') && (s[n-1] == 'v' || s[n-1] == 'V');
  }
  void add(const char *name, bool dir) {
    if (name[0] == '.' || (!dir && !wav(name))) return;
    if (count == Capacity) { truncated = true; return; }
    snprintf(entries[count].name, sizeof(entries[count].name), "%s", name);
    entries[count++].folder = dir;
  }
  bool fullPath(int row, char *out, size_t n) const {
    if (row < 0 || row >= count) return false;
    int written = snprintf(out, n, "%s%s%s", path, path[0] ? "/" : "", entries[row].name);
    return written >= 0 && size_t(written) < n;
  }
  void scan() {
    count = 0; truncated = false; error[0] = 0;
    if (!path[0]) {
      add("imports", true); add("captures", true);
      // Virtual root label, resolved only on entering this row.
      add("MOSS-SampleLibrary", true); return;
    }
#ifdef _WIN32
    char pattern[520]; snprintf(pattern, sizeof(pattern), "%s/*", path);
    WIN32_FIND_DATAA d; HANDLE h = FindFirstFileA(pattern, &d);
    if (h == INVALID_HANDLE_VALUE) { strcpy(error, "FOLDER EMPTY OR UNAVAILABLE / B BACK"); return; }
    do { if (!(d.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
      add(d.cFileName, bool(d.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
    } while (FindNextFileA(h, &d));
    FindClose(h);
#else
    DIR *dir = opendir(path);
    if (!dir) { strcpy(error, "FOLDER UNAVAILABLE / B BACK"); return; }
    dirent *d;
    while ((d = readdir(dir))) {
      char file[1024]; snprintf(file, sizeof(file), "%s/%s", path, d->d_name);
      struct stat st = {}; if (lstat(file, &st) || S_ISLNK(st.st_mode)) continue;
      if (S_ISDIR(st.st_mode) || S_ISREG(st.st_mode)) add(d->d_name, S_ISDIR(st.st_mode));
    }
    closedir(dir);
#endif
    // Stable order: folders, then filenames. No audio work or recursion here.
    for (int i = 1; i < count; ++i) {
      Entry item = entries[i]; int j = i;
      while (j && ((item.folder && !entries[j-1].folder) ||
             (item.folder == entries[j-1].folder && strcmp(item.name, entries[j-1].name) < 0))) {
        entries[j] = entries[j-1]; --j;
      }
      entries[j] = item;
    }
  }
  void root() { depth = 0; path[0] = 0; scan(); }
  bool enter(int row) {
    if (row < 0 || row >= count || !entries[row].folder) return false;
    if (depth == Depth) { strcpy(error, "FOLDER TOO DEEP / B BACK"); return false; }
    char next[512];
    if (!fullPath(row, next, sizeof(next))) { strcpy(error, "PATH TOO LONG / B BACK"); return false; }
    if (!path[0] && !strcmp(next, "MOSS-SampleLibrary")) strcpy(next, "../../../MOSS-SampleLibrary");
    strcpy(parents[depth], path); cursors[depth++] = row;
    strcpy(path, next); scan(); return true;
  }
  int back() { if (!depth) return -1; --depth; strcpy(path, parents[depth]); scan(); return cursors[depth]; }
};
} // namespace moss
