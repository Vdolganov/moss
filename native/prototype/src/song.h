#pragma once
#include "model.h"
namespace moss {
constexpr int Songs = 32, SongSlots = 64, TicksPerBar = 64;
struct SongSlot {
  int sequence = 0, repeats = 1, bars = 1;
};
struct Song {
  char name[25] = "UNTITLED";
  int bpm = 120, count = 0, loop = 0;
  SongSlot slots[SongSlots];
};
inline bool valid(const Song &s) {
  if (!s.name[0] || s.name[24] || s.bpm < 30 || s.bpm > 240 || s.count < 0 ||
      s.count > SongSlots || s.loop < 0 || s.loop > 1)
    return false;
  bool nonspace = false;
  for (int i = 0; s.name[i]; ++i) {
    if (s.name[i] < 32 || s.name[i] > 126)
      return false;
    nonspace |= s.name[i] != ' ';
  }
  if (!nonspace)
    return false;
  for (int i = 0; i < s.count; ++i) {
    const auto &r = s.slots[i];
    if (r.sequence < 0 || r.sequence >= Projects || r.repeats < 1 ||
        r.repeats > 64 || r.bars < 1 || r.bars > 64)
      return false;
  }
  return true;
}
inline int defaultBars(const Project &p) {
  int longest = 1;
  for (int t = 0; t < p.trackCount; ++t)
    if (p.tracks[t].cycle > longest)
      longest = p.tracks[t].cycle;
  return (longest + TicksPerBar - 1) / TicksPerBar;
}
// Immutable while playing. Allocated and populated with audio paused, only
// referenced sequences occupy entries; bounded by the existing library limit.
struct SongBank {
  Song song;
  Project sequences[Projects];
  int map[Projects], count = 0;
  const Project &at(int slot) const {
    return sequences[map[song.slots[slot].sequence]];
  }
};
} // namespace moss
