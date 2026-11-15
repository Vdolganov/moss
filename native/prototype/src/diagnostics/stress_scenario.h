#pragma once
#include <model.h>
namespace moss {
// Shared by the hardware profiler and desktop benchmark; never used by
// projects. Deterministic, dense patterns: maximum timeline, fastest clock,
// every key, every effect including master, alternating locks and effect order
// per track.
inline void stressSample(SampleBuffer &sample) {
  sample.rate = 48000;
  sample.channels = 2;
  sample.frames = SampleBudget / 4;
  sample.bytes = SampleBudget;
  uint32_t state = 357;
  for (unsigned n = 0; n < sample.frames * 2; ++n) {
    state = state * 1664525u + 1013904223u;
    sample.pcm[n] = int16_t(int(state >> 16) - 32768);
  }
}
inline void stressProject(Project &p, int tracks, int instrument,
                          unsigned effectMask = 3) {
  p = Project{};
  p.trackCount = tracks;
  p.bpm = 240;
  p.master = 100;
  strcpy(p.name, "MAXIMUM LOAD");
  auto enable = [effectMask](Effect &fx) {
    fx.enabled = (effectMask & 1) != 0;
    for (int i = 0; i < EffectTypes; ++i)
      fx.enabledTypes[i] = (effectMask >> i) & 1;
    for(int kind=0;kind<EffectTypes;++kind) fx.values[kind][3]=100;
    fx.values[Drive][0]=100;fx.values[Drive][4]=2;fx.values[Drive][7]=100;
    fx.values[Orbit][6]=100;fx.values[Orbit][7]=100;
    fx.values[0][1] = 95;
    fx.values[1][1] = 100;
    fx.values[2][2] =
        1; // CRUSH's most expensive rate: recalculate every frame.
  };
  enable(p.fx);
  for (int t = 0; t < tracks; ++t) {
    auto &tr = p.tracks[t];
    tr.instrument = instrument < 0 ? t % 4 : instrument;
    tr.level = 100;
    for (int &mode : tr.synthMode)
      mode = t % 2;
    tr.cycle = Steps;
    tr.division = 64;
    enable(tr.fx);
    for (int i = 0; i < EffectTypes; ++i)
      tr.fx.order[i] = (i + t) % EffectTypes;
    for (int k = 0; k < Keys; ++k) {
      tr.keys[k].asset = (t * Keys + k) % 32;
      tr.keys[k].pitch = -24 + k * 48 / (Keys - 1);
      tr.keys[k].level = 100;
    }
    for (int s = 0; s < Steps; ++s) {
      auto &step = tr.steps[s];
      step.on = 1;
      step.note = 24 + (s * 7 + t * 13) % 73;
      step.length = Steps;
      step.velocity = 127;
      for (int kind = 1; kind <= 3; ++kind)
        for (int i = 0; i < 8; ++i) {
          int lo = synthMin(kind, i), hi = synthMax(kind, i);
          step.synthLocks[kind - 1][i] =
              int16_t(lo + (hi - lo) * ((s + t + i) % 3) / 2);
        }
      for (int kind = 0; kind < EffectTypes; ++kind)
        for (int i = 0; i < fxParameters(kind); ++i)
          step.fxLocks[kind][i] =
              int16_t(fxLow(kind, i) + (fxHigh(kind, i) - fxLow(kind, i)) *
                                           ((s + t + i) % 3) / 2);
      step.fxLocks[2][2] = 1;
      for(int kind=0;kind<EffectTypes;++kind) step.fxLocks[kind][3]=100;
      step.fxLocks[Drive][4]=2; // most expensive polynomial
      step.fxLocks[Orbit][7]=100;
      for (int k = 0; k < Keys; ++k) {
        auto &hit = tr.hits[k][s];
        hit.on = 1;
        hit.velocity = 127;
        hit.fadeIn = 37 + k * 13;
        hit.fadeOut = 61 + k * 11;
      }
    }
  }
}
inline void stressGranular(Project &p, unsigned mask, int mode = 0) {
  stressProject(p, Tracks, GranularInstrument, mask);
  for (int t=0;t<Tracks;++t) {
    auto &tr=p.tracks[t];
    tr.granular.asset=t*Keys;
    if (mode==1 && t) tr.instrument=t; // GRANULAR / MONO / ORGANISM / FM
    for (int i=0;i<Steps;++i) {
      auto &s=tr.steps[i];
      s.note=72; // +24 note +24 PITCH: worst measured PCM cache traversal.
      const int size=mode==5?8000:mode==6?(i%4?8000:1):mode==2?1:mode==3&&(i%4==0)?1:500;
      const int values[]={i%2?300:-300,24,size,128,
                           (i*13)%1000,0,1000,100,100,100,i%2?14000:20,200,
                           i%2?90:10,i%2?10:90,20,20};
      for(int k=0;k<GranularParameterCount;++k) s.grainLocks[k]=int16_t(values[k]);
      if(mode==4) s.fxLocks[Drive][4]=int16_t(i%3); // 64 curve changes/second.
    }
  }
}
// Long held notes plus dense parameter events. Repeats get enough time to
// capture and loop; this is different from retriggering on every tick.
inline void stressEvents(Project &p,unsigned mask,int mode=0) {
  stressGranular(p,mask,5);
  for(int t=0;t<Tracks;++t) {
    auto &tr=p.tracks[t];
    if(mode==1) tr.instrument=SampleInstrument;
    else if(mode==2 && t) tr.instrument=t;
    for(int i=0;i<Steps;++i) {
      auto &s=tr.steps[i];s.on=i==0?NoteEvent:LockEvent;
      s.length=Steps;
      // Keep eight long grains alive; new grain targets still vary per tick.
      s.grainLocks[2]=8000;s.grainLocks[9]=100;
      s.repeat=i%64==0?8:i%64==16?16:i%64==32?32:i%64==48?64:Inherit;
      if(i%64==63) s.repeat=0;
      for(int k=0;k<Keys;++k) {
        auto &h=tr.hits[k][i];h.on=i%32==0?NoteEvent:LockEvent;h.length=Steps;
        h.locks[0]=int16_t((i+k)%49-24);h.locks[3]=int16_t(50+i%51);
      }
    }
  }
}
inline void stressSwing(Project &p,unsigned mask,int mode=0) {
  stressEvents(p,mask,mode);
  const int divisions[]={8,16,32,64}, swings[]={75,66,58,73};
  const int shifts[]={-50,50,-17,31,0,Inherit};
  for(int t=0;t<Tracks;++t) {
    auto &tr=p.tracks[t];tr.division=divisions[t];tr.swing=swings[t];
    for(int i=0;i<Steps;++i) tr.steps[i].shift=shifts[(i+t)%6];
  }
}
} // namespace moss
