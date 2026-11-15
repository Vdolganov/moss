// Engine members. No files, allocations, locks or buffer clearing at
// boundaries.
const SongBank *songBank = nullptr;
int songSlot = 0, songBpm = 120;
uint32_t songTicks = 0, shownSongSlot = 0, shownSongRepeat = 0;
unsigned transitionFrames = 0;
int previousOutput[2] = {}, transitionFrom[2] = {};
void enterSongSlot(int index) {
  songSlot = index;
  songTicks = 0;
  tick = 0;
  project = songBank->at(index);
  project.bpm = songBpm;
  // Cut old notes, keep the existing rack's delay/reverb history. A short
  // output ramp bounds discontinuities even when instruments/routes change.
  for (int t = 0; t < Tracks; ++t) {
    for (int k = 0; k < Keys; ++k) {
      voice(t, k) = Voice{};
      tail(t, k) = Voice{};
    }
    synth[t] = SynthVoice{};
    synthTails[t] = SynthVoice{};
    resetEvents(t);
    bindGranular(t);
    muteGain[t] = project.tracks[t].mute ? 0 : 1;
    __atomic_store_n(&shownStep[t], 0, __ATOMIC_RELEASE);
    if (t >= project.trackCount)
      fx[t].invalidate();
  }
  transitionFrames = 128;
  transitionFrom[0] = previousOutput[0];
  transitionFrom[1] = previousOutput[1];
  __atomic_store_n(&shownSongSlot, unsigned(index), __ATOMIC_RELEASE);
  __atomic_store_n(&shownSongRepeat, 0, __ATOMIC_RELEASE);
}
void startSong(const SongBank &bank, int start) {
  init(bank.at(start), samples, rate, sampleCount);
  songBank = &bank;
  songBpm = bank.song.bpm;
  enterSongSlot(start);
  playing = true;
  first = true;
  __atomic_store_n(&shownPlaying, 1, __ATOMIC_RELEASE);
}
bool advanceSong() {
  if (!songBank)
    return true;
  const auto &s = songBank->song.slots[songSlot];
  ++songTicks;
  unsigned duration = unsigned(s.bars * TicksPerBar);
  if (songTicks >= duration * unsigned(s.repeats)) {
    int next = songSlot + 1;
    if (next == songBank->song.count) {
      if (!songBank->song.loop) {
        handle({Op::Stop});
        return false;
      }
      next = 0;
    }
    enterSongSlot(next);
  } else
    __atomic_store_n(&shownSongRepeat, songTicks / duration, __ATOMIC_RELEASE);
  return true;
}
