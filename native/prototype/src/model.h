#pragma once
#include "../../../hardware lab/src/lab_sample.h"
#include "config.h"
#include "effect_defs.h"
#include "sample.h"
#include "synth_defs.h"
#include "granular_defs.h"
namespace moss {
using lab::clamp;
struct Key {
  int asset = -1, pitch = 0, start = 0, end = 100, level = 80, mode = 0;
};
struct Step {
  int16_t grainLocks[GranularParameterCount] = {Inherit, Inherit, Inherit, Inherit, Inherit, Inherit,
                            Inherit, Inherit, Inherit, Inherit, Inherit, Inherit, Inherit, Inherit, Inherit, Inherit,
                            Inherit, Inherit, Inherit, Inherit};
  int note = 48;
  int16_t synthLocks[3][8] = {
      {Inherit, Inherit, Inherit, Inherit, Inherit, Inherit, Inherit, Inherit},
      {Inherit, Inherit, Inherit, Inherit, Inherit, Inherit, Inherit, Inherit},
      {Inherit, Inherit, Inherit, Inherit, Inherit, Inherit, Inherit, Inherit}};
  FxLocks fxLocks;
  int on = 0, key = 0, velocity = 90, length = 4, probability = 100;
  int repeat = Inherit; // Track audio fragment: --, OFF, 1/8, 1/16, 1/32, 1/64.
  int shift = Inherit; // Percent of the displayed step, added to track swing.
  int16_t locks[4] = {Inherit, Inherit, Inherit, Inherit};
};
struct Hit {
  int16_t on = 0, velocity = 90, probability = 100, fadeIn = 0, fadeOut = 0;
  int16_t locks[4] = {Inherit, Inherit, Inherit, Inherit};
  int16_t length = 0; // 0 = full one-shot; otherwise a cap in 1/64 notes.
};
struct Track {
  GranularPatch granular;
  char name[25] = "TRACK";
  int instrument = SampleInstrument, audition = 48;
  int synthParams[3][8] = {{1200, 30, 8, 600, 35, 25, 0, 20},
                           {30, 35, 15, 65, 55, 8, 450, 2500},
                           {45, 4, 12, 2200, 4, 0, 3, 0}};
  int synthMode[3] = {}, synthPreset[3] = {};
  Effect fx;
  int cycle = 64, division = 16, level = 75, pan = 0,
      mute = 0; // cycle in 1/64 notes
  int swing = SwingStraight; // First half of a two-cell pair, in percent.
  Key keys[Keys];
  Step steps[Steps];
  Hit hits[Keys][Steps];
  int stride() const { return 64 / division; }
  int cells() const { return (cycle + stride() - 1) / stride(); }
};
inline bool hitLocksPresent(const Hit &h) {
  for(int v:h.locks) if(v!=Inherit) return true;
  return false;
}
inline bool stepLocksPresent(const Track &t, const Step &s) {
  if(s.repeat!=Inherit) return true;
  for(int v:s.fxLocks.data) if(v!=Inherit) return true;
  if(t.instrument==GranularInstrument) { for(int v:s.grainLocks) if(v!=Inherit) return true; }
  else if(t.instrument) { for(int v:s.synthLocks[t.instrument-1]) if(v!=Inherit) return true; }
  return false;
}
inline bool noteAt(const Track &t,int at) {
  if(t.instrument) return t.steps[at].on==NoteEvent;
  for(const auto &lane:t.hits) if(lane[at].on==NoteEvent) return true;
  return false;
}
inline void normalizeStepEvent(Track &t,int at) {
  auto &s=t.steps[at];
  if(s.on==NoteEvent) return;
  if(s.on==EmptyEvent && !t.instrument && noteAt(t,at)) return;
  s.on=stepLocksPresent(t,s)?LockEvent:EmptyEvent;
}
inline int selectedEvent(const Track &t,int at,int key) {
  if(t.instrument) return t.steps[at].on;
  return t.hits[key][at].on ? t.hits[key][at].on : t.steps[at].on==LockEvent?LockEvent:EmptyEvent;
}
inline const char *eventName(int type) {
  return type==NoteEvent?"NOTE":type==LockEvent?"LOCK ONLY":"EMPTY";
}
struct Project {
  char name[25] = "UNTITLED";
  int trackCount = 3;
  Effect fx;
  int bpm = 70, master = 75;
  uint32_t seed = 357;
  Track tracks[Tracks];
};
inline Project makeProject(bool demo) {
  Project p;
  snprintf(p.name, sizeof(p.name), "%s", demo ? "SLOW SIGNAL" : "UNTITLED");
  for (int t = 0; t < p.trackCount; t++) {
    auto &tr = p.tracks[t];
    const char *roles[] = {"DRONE", "PULSE", "DUST"};
    if (demo)
      snprintf(tr.name, sizeof(tr.name), "%s", roles[t]);
    else
      snprintf(tr.name, sizeof(tr.name), "TRACK %02d", t + 1);
    const int assigned[] = {0, 1, 2};
    for (int k : assigned) {
      tr.keys[k].asset = t;
      tr.keys[k].pitch = k;
    }
    if (demo) {
      tr.cycle = (t == 2 ? 15 : 16) * 4;
      for (int s = 0; s < 16; s++) {
        auto &st = tr.steps[s * 4];
        st.key = t == 1 ? (s % 3 == 0 ? 2 : 1) : 0;
        st.on = t == 0   ? s == 0
                : t == 1 ? (s == 2 || s == 7 || s == 12)
                         : (s == 0 || s == 9);
        st.length = (t == 0 ? 16 : t == 1 ? 3 : 5) * 4;
        st.velocity = t == 2 ? 45 : t == 0 ? 80 : 100;
        st.probability = t == 2 ? 75 : 100;
        auto &hit = tr.hits[st.key][s * 4];
        hit.on = st.on;
        hit.velocity = st.velocity;
        hit.probability = st.probability;
      }
      tr.keys[0].pitch = t == 0 ? -12 : 0;
      tr.pan = t == 1 ? -25 : t == 2 ? 25 : 0;
    }
  }
  return p;
}
inline int nextKey(const Track &t, int key, int direction) {
  for (int k = key + direction; k >= 0 && k < Keys; k += direction)
    if (t.keys[k].asset >= 0)
      return k;
  return key;
}
inline Project makeSynthDemo() {
  Project p = makeProject(true);
  const int instruments[] = {OrganismInstrument, MonoInstrument, FmInstrument};
  for (int t = 0; t < p.trackCount; t++) {
    auto &tr = p.tracks[t];
    tr.instrument = instruments[t];
    tr.audition = t == 0 ? 43 : t == 1 ? 55 : 67;
    for (int s = 0; s < Steps; s++) {
      tr.steps[s].note = tr.audition + (t == 1 ? (s == 28   ? 3
                                                  : s == 48 ? 7
                                                            : 0)
                                               : 0);
      if (t == 2)
        tr.steps[s].length = 32;
    }
  }
  return p;
}
inline bool valid(const Project &p) {
  auto nameValid = [](const char *s) {
    int length = 0;
    bool nonspace = false;
    for (; length < 25 && s[length]; length++) {
      if (s[length] < 32 || s[length] > 126)
        return false;
      nonspace |= s[length] != ' ';
    }
    return length > 0 && length < 25 && nonspace;
  };
  if (p.trackCount < 0 || p.trackCount > Tracks || !nameValid(p.name))
    return false;
  if (p.bpm < 30 || p.bpm > 240 || p.master < 0 || p.master > 100)
    return false;
  auto effectValid = [](const Effect &f) {
    if (f.count() > ActiveEffects)
      return false;
    if (f.type < 0 || f.type >= EffectTypes || f.enabled < 0 || f.enabled > 1)
      return false;
    unsigned seen = 0;
    for (int i = 0; i < EffectTypes; i++) {
      if (f.order[i] < 0 || f.order[i] >= EffectTypes || (seen & (1u << f.order[i])) ||
          f.enabledTypes[i] < 0 || f.enabledTypes[i] > 1)
        return false;
      seen |= 1u << f.order[i];
    }
    for (int type = 0; type < EffectTypes; type++)
      for (int i = 0; i < fxParameters(type); i++)
        if (f.values[type][i] < fxLow(type, i) ||
            f.values[type][i] > fxHigh(type, i))
          return false;
    return true;
  };
  if (!effectValid(p.fx))
    return false;
  for (const auto &t : p.tracks) {
    if (!nameValid(t.name))
      return false;
    if (t.instrument < 0 || t.instrument >= InstrumentCount || t.audition < 24 ||
        t.audition > 96)
      return false;
    if (t.granular.asset < -1 || t.granular.asset >= 32 ||
        t.granular.freeze < 0 || t.granular.freeze > 1 ||
        t.granular.values[5] >= t.granular.values[6])
      return false;
    for (int i = 0; i < GranularParameterCount; ++i)
      if (t.granular.values[i] < GranularSpecs[i].low ||
          t.granular.values[i] > GranularSpecs[i].high)
        return false;
    for (int type = 0; type < 3; type++) {
      if (t.synthMode[type] < 0 || t.synthMode[type] > 1 ||
          t.synthPreset[type] < -1 || t.synthPreset[type] > 3)
        return false;
      for (int i = 0; i < 8; i++)
        if (t.synthParams[type][i] < synthMin(type + 1, i) ||
            t.synthParams[type][i] > synthMax(type + 1, i))
          return false;
    }
    if (!effectValid(t.fx))
      return false;
    if (t.swing < SwingStraight || t.swing > SwingMaximum ||
        t.cycle < 1 || t.cycle > Steps || t.level < 0 || t.level > 100 ||
        t.pan < -100 || t.pan > 100 || t.mute < 0 || t.mute > 1)
      return false;
    if (t.division != 8 && t.division != 16 && t.division != 32 &&
        t.division != 64)
      return false;
    for (const auto &lane : t.hits)
      for (const auto &h : lane) {
        if (h.on < 0 || h.on > LockEvent || h.velocity < 0 || h.velocity > 127 ||
            h.probability < 0 || h.probability > 100 || h.fadeIn < 0 ||
            h.fadeIn > 5000 || h.fadeOut < 0 || h.fadeOut > 5000 ||
            h.length < 0 || h.length > Steps)
          return false;
        for (int i = 0; i < 4; i++)
          if (lockValue(h.locks[i]) && (h.locks[i] < (i == 0 ? -24 : 0) ||
                                        h.locks[i] > (i == 0 ? 24 : 100)))
            return false;
      }
    for (const auto &k : t.keys)
      if (k.asset < -1 || k.asset > 31 || k.pitch < -24 || k.pitch > 24 ||
          k.start < 0 || k.start >= k.end || k.end > 100 || k.level < 0 ||
          k.level > 100 || k.mode < 0 || k.mode > 2)
        return false;
    for (const auto &s : t.steps) {
      for (int i = 0; i < GranularParameterCount; ++i)
        if (lockValue(s.grainLocks[i]) &&
            (s.grainLocks[i] < GranularSpecs[i].low || s.grainLocks[i] > GranularSpecs[i].high))
          return false;
      if ((s.shift != Inherit && (s.shift < -ShiftLimit || s.shift > ShiftLimit)) ||
          !repeatValid(s.repeat) || s.note < 24 || s.note > 96)
        return false;
      for (int type = 0; type < 3; type++)
        for (int i = 0; i < 8; i++)
          if (lockValue(s.synthLocks[type][i]) &&
              (s.synthLocks[type][i] < synthMin(type + 1, i) ||
               s.synthLocks[type][i] > synthMax(type + 1, i)))
            return false;
      for (int type = 0; type < EffectTypes; type++)
        for (int i = 0; i < fxParameters(type); i++)
          if (lockValue(s.fxLocks[type][i]) &&
              (s.fxLocks[type][i] < fxLow(type, i) ||
               s.fxLocks[type][i] > fxHigh(type, i)))
            return false;
      if (s.on < 0 || s.on > LockEvent || s.key < 0 || s.key >= Keys ||
          s.velocity < 0 || s.velocity > 127 || s.length < 1 ||
          s.length > Steps || s.probability < 0 || s.probability > 100)
        return false;
      for (int i = 0; i < 4; i++)
        if (lockValue(s.locks[i]) && (s.locks[i] < (i == 0 ? -24 : 0) ||
                                      s.locks[i] > (i == 0 ? 24 : 100)))
          return false;
    }
  }
  return true;
}
enum class Op {
  Transport,
  Hit,
  Step,
  Key,
  Track,
  Global,
  Rewind,
  Audition,
  Stop,
  Fx,
  FxLock,
  CaptureStart,
  CaptureStop,
  CaptureDiscard,
  Instrument,
  SynthParam,
  SynthMode,
  SynthPreset,
  SynthLock,
  AuditionNote,
  GranularParam,
  GranularLock,
  GranularSource,
  GranularFreeze,
  SetEventType,
  ClearEvent,
  ReelPreset
};
struct Command {
  Op op;
  int track = 0, index = 0, field = 0, value = 0;
};
// Single UI producer / single audio consumer. 32-bit acquire/release only on
// ARMv7.
struct Queue {
  Command items[256];
  uint32_t readIndex = 0, writeIndex = 0;
  bool push(Command c) {
    auto w = __atomic_load_n(&writeIndex, __ATOMIC_RELAXED);
    auto r = __atomic_load_n(&readIndex, __ATOMIC_ACQUIRE);
    if (w - r == 256)
      return false;
    items[w % 256] = c;
    __atomic_store_n(&writeIndex, w + 1, __ATOMIC_RELEASE);
    return true;
  }
  bool pop(Command &c) {
    auto r = __atomic_load_n(&readIndex, __ATOMIC_RELAXED);
    auto w = __atomic_load_n(&writeIndex, __ATOMIC_ACQUIRE);
    if (r == w)
      return false;
    c = items[r % 256];
    __atomic_store_n(&readIndex, r + 1, __ATOMIC_RELEASE);
    return true;
  }
};
inline void apply(Project &p, const Command &c) {
  if(c.op==Op::ReelPreset) {
    if(c.track<0||c.track>Tracks||c.value<0||c.value>=ReelPresetCount)return;
    auto &f=c.track==Tracks?p.fx:p.tracks[c.track].fx;
    for(int i=0;i<16;++i)f.values[Reel][i]=ReelPresets[c.value][i];
    return;
  }
  if (c.op == Op::Fx) {
    Effect &f = c.track == Tracks ? p.fx : p.tracks[c.track].fx;
    if (c.field == 0)
      f.select(c.value);
    else if (c.field == 1)
      f.enable(c.value != 0);
    else if (c.field == FxMoveField)
      f.move(c.index, c.value);
    else
      f.values[c.index][c.field - 2] = c.value;
    return;
  }
  if (c.op == Op::FxLock) {
    p.tracks[c.track].steps[c.index].fxLocks[c.field / EffectParameters][c.field % EffectParameters] =
        c.value;
    normalizeStepEvent(p.tracks[c.track],c.index);
    return;
  }
  if (c.op == Op::Global) {
    if (c.field == 0)
      p.bpm = c.value;
    else
      p.master = c.value;
    return;
  }
  if (c.track < 0 || c.track >= Tracks)
    return;
  auto &t = p.tracks[c.track];
  if(c.op==Op::SetEventType) {
    if(t.instrument) t.steps[c.index].on=c.value;
    else if(c.value==NoteEvent) t.hits[c.field][c.index].on=NoteEvent;
    else {
      t.hits[c.field][c.index].on=c.value==LockEvent && hitLocksPresent(t.hits[c.field][c.index])?LockEvent:EmptyEvent;
      if(!noteAt(t,c.index)) t.steps[c.index].on=c.value==LockEvent && stepLocksPresent(t,t.steps[c.index])?LockEvent:EmptyEvent;
    }
    return;
  }
  if(c.op==Op::ClearEvent) {
    t.steps[c.index]=Step{};
    if(!t.instrument) t.hits[c.field][c.index]=Hit{};
    return;
  }
  if (c.op == Op::GranularParam && c.field >= 0 && c.field < GranularParameterCount)
    t.granular.values[c.field] = c.value;
  if (c.op == Op::GranularLock && c.index >= 0 && c.index < Steps && c.field >= 0 && c.field < GranularParameterCount)
    { t.steps[c.index].grainLocks[c.field] = int16_t(c.value); normalizeStepEvent(t,c.index); }
  if (c.op == Op::GranularSource)
    t.granular.asset = c.value;
  if (c.op == Op::GranularFreeze)
    t.granular.freeze = c.value;
  if (c.op == Op::Hit && c.index >= 0 && c.index < Steps && c.field >= 0 &&
      c.field < Keys * HitFieldCount) {
    auto &h = t.hits[c.field / HitFieldCount][c.index];
    int16_t *fields[] = {&h.on,          &h.velocity, &h.fadeIn,   &h.fadeOut,
                         &h.probability, &h.locks[0], &h.locks[1], &h.locks[2],
                         &h.locks[3],    &h.length};
    *fields[c.field % HitFieldCount] = int16_t(c.value);
    if(c.field%HitFieldCount>=HitLock0 && c.field%HitFieldCount<=HitLock3 && h.on!=NoteEvent)
      h.on=hitLocksPresent(h)?LockEvent:EmptyEvent;
  }
  if (c.op == Op::Instrument)
    t.instrument = c.value;
  if (c.op == Op::AuditionNote)
    t.audition = c.value;
  if (c.op == Op::SynthParam) {
    t.synthParams[c.index][c.field] = c.value;
    t.synthPreset[c.index] = -1;
  }
  if (c.op == Op::SynthMode)
    t.synthMode[c.index] = c.value;
  if (c.op == Op::SynthPreset) {
    t.synthPreset[c.index] = c.value;
    for (int i = 0; i < 8; i++)
      t.synthParams[c.index][i] = presetValues(c.index + 1, c.value)[i];
  }
  if (c.op == Op::SynthLock)
    { t.steps[c.index].synthLocks[c.field / 8][c.field % 8] = c.value; normalizeStepEvent(t,c.index); }
  if (c.op == Op::Track) {
    int *f[] = {&t.cycle, &t.level, &t.pan, &t.mute, &t.division, &t.swing};
    if (c.field >= 0 && c.field < 6)
      *f[c.field] = c.value;
  }
  if (c.op == Op::Key && c.index >= 0 && c.index < Keys) {
    auto &k = t.keys[c.index];
    int *f[] = {&k.asset, &k.pitch, &k.start, &k.end, &k.level, &k.mode};
    if (c.field >= 0 && c.field < 6)
      *f[c.field] = c.value;
  }
  if (c.op == Op::Step && c.index >= 0 && c.index < Steps) {
    auto &s = t.steps[c.index];
    if(c.field==StepShiftField) { s.shift=c.value; return; }
    if(c.field==StepRepeatField) { s.repeat=c.value; normalizeStepEvent(t,c.index); return; }
    if (c.field >= 5 && c.field <= 8)
      s.locks[c.field - 5] = int16_t(c.value);
    else {
      int *f[] = {&s.on, &s.key, &s.velocity, &s.length, &s.probability};
      if (c.field >= 0 && c.field < 5)
        *f[c.field] = c.value;
      if (c.field == 9)
        s.note = c.value;
    }
  }
}
} // namespace moss
