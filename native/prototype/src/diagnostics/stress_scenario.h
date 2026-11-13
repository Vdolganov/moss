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
                          unsigned effectMask = 15) {
  p = Project{};
  p.trackCount = tracks;
  p.bpm = 240;
  p.master = 100;
  strcpy(p.name, "MAXIMUM LOAD");
  auto enable = [effectMask](Effect &fx) {
    fx.enabled = (effectMask & 1) != 0;
    for (int i = 0; i < 4; ++i)
      fx.enabledTypes[i] = (effectMask >> i) & 1;
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
    for (int i = 0; i < 4; ++i)
      tr.fx.order[i] = (i + t) % 4;
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
      for (int kind = 0; kind < 4; ++kind)
        for (int i = 0; i < 4; ++i)
          step.fxLocks[kind][i] =
              int16_t(fxLow(kind, i) + (fxHigh(kind, i) - fxLow(kind, i)) *
                                           ((s + t + i) % 3) / 2);
      step.fxLocks[2][2] = 1;
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
      const int values[]={i%2?300:-300,24,mode==2?1:mode==3&&(i%4==0)?1:500,128,
                           (i*13)%1000,0,1000,500,100,100,i%2?14000:20,20};
      for(int k=0;k<12;++k) s.grainLocks[k]=int16_t(values[k]);
    }
  }
}
} // namespace moss
