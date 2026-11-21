#pragma once
#include "parameter_spec.h"
#include "organism_defs.h"
#include "fm_defs.h"
namespace moss {
enum Instrument {
  SampleInstrument,
  MonoInstrument,
  OrganismInstrument,
  FmInstrument,
  GranularInstrument,
  InstrumentCount
};
inline const char *instrumentName(int i) {
  const char *n[] = {"SAMPLER", "MONO", "ORGANISM", "FM", "GRANULAR"};
  return n[i];
}
inline constexpr int SynthParameterCount=20;
inline int synthCount(int kind) { return kind>=MonoInstrument && kind<=FmInstrument?20:8; }
inline int synthBanks(int kind) { return synthCount(kind)/4; }
inline const char *synthBankName(int kind,int bank) {
  const char *mono[]={"CORE","COLOR","OSC","ENV","MOTION"};
  const char *organism[]={"CORE","TONE","MOTION","VOICES","ENVELOPE"};
  const char *fm[]={"CORE","BODY","HIT","COLOR","MOTION"};
  return kind==MonoInstrument?mono[bank]:kind==OrganismInstrument?organism[bank]:fm[bank];
}
inline constexpr ParameterSpec SynthParameters[3][SynthParameterCount] = {
    {{"CUTOFF", 20, 12000, "HZ", 100},
     {"RESONANCE", 0, 100, "PERCENT"},
     {"ATTACK", 0, 5000, "MS", 100},
     {"RELEASE", 0, 10000, "MS", 100},
     {"ENV AMT", -100, 100, "PERCENT"},
     {"SUB", 0, 100, "PERCENT"},
     {"NOISE", 0, 100, "PERCENT"},
     {"DRIVE", 0, 100, "PERCENT"},
     {"PULSE W", 5, 95, "PERCENT"}, {"DETUNE", 0, 30, "CENTS", 5},
     {"GLIDE", 0, 2000, "MS", 100}, {"DRIFT", 0, 100, "PERCENT"},
     {"DECAY", 10, 8000, "MS", 100}, {"SUSTAIN", 0, 100, "PERCENT"},
     {"FLT DECAY", 10, 8000, "MS", 100}, {"KEY TRACK", 0, 100, "PERCENT"},
     {"LFO RATE", 1, 2000, "HZ", 10}, {"PWM", 0, 100, "PERCENT"},
     {"VCF MOD", 0, 100, "PERCENT"}, {"VIBRATO", 0, 100, "CENTS"}},
    {{"SHAPE", 0, 100, "PERCENT"},
     {"COUPLE", 0, 100, "PERCENT"},
     {"DENSITY", 0, 100, "PERCENT"},
     {"SUB", 0, 100, "PERCENT"},
     {"CUTOFF", 40, 14000, "HZ", 100},
     {"RESONANCE", 0, 100, "PERCENT"},
     {"DRIVE", 0, 100, "PERCENT"},
     {"NOISE", 0, 100, "PERCENT"},
     {"DRIFT", 0, 30, "CENTS", 5},
     {"RATE", 1, 200, "HZ", 10},
     {"WANDER", 0, 100, "PERCENT"},
     {"BREATH", 0, 100, "PERCENT"},
     {"DETUNE", 0, 50, "CENTS", 5},
     {"INTERVAL", 0, 12, "SEMITONES", 1},
     {"SPREAD", 0, 100, "PERCENT"},
     {"GLIDE", 0, 3000, "MS", 100},
     {"ATTACK", 0, 15000, "MS", 100},
     {"RELEASE", 0, 20000, "MS", 100},
     {"GRIT", 0, 100, "PERCENT"},
     {"NOISE TONE", 0, 100, "DARK-BRIGHT"}},
    {{"INDEX", 0, 100, "PERCENT"},
     {"RATIO", 0, 15, "MOD:CARRIER", 1},
     {"FEEDBACK", 0, 100, "PERCENT"},
     {"DECAY", 50, 10000, "MS", 100},
     {"DETUNE", 0, 100, "PERCENT"},
     {"NOISE", 0, 100, "PERCENT"},
     {"ATTACK", 0, 5000, "MS", 100},
     {"BODY", 0, 100, "PERCENT"},
     {"PUNCH", -48, 48, "SEMITONES", 12}, {"SWEEP", 5, 3000, "MS", 100},
     {"CLICK", 0, 100, "PERCENT"}, {"DYNAMICS", 0, 100, "PERCENT"},
     {"MOD DECAY", 5, 15000, "MS", 100}, {"MOD HOLD", 0, 100, "PERCENT"},
     {"TONE", 0, 100, "DARK-BRIGHT"}, {"SPREAD", 0, 100, "PERCENT"},
     {"RATE", 1, 2000, "HZ", 10}, {"MOTION", 0, 100, "PERCENT"},
     {"RELEASE", 5, 15000, "MS", 100}, {"GLIDE", 0, 3000, "MS", 100}},
};
inline const char *synthLabel(int k, int s) {
  return SynthParameters[k - 1][s].label;
}
inline int synthMin(int k, int s) { return SynthParameters[k - 1][s].low; }
inline int synthMax(int k, int s) { return SynthParameters[k - 1][s].high; }
inline const char *synthUnit(int k, int s) {
  return SynthParameters[k - 1][s].unit;
}
inline const char *ratioName(int n) {
  return FmRatioNames[n];
}
inline int synthPresetCount(int kind){return kind==FmInstrument?FmPresetCount:4;}
inline const char *presetName(int kind, int n) {
  if(kind==FmInstrument)return FmPresetNames[n];
  const char *p[2][4] = {{"SIGNAL", "SOFT", "ACID", "SUB"},
                         {"ROOT", "BREATH", "TANGLE", "BARE"}};
  return p[kind - 1][n];
}
inline const int *presetValues(int kind, int n) {
  if(kind==OrganismInstrument)return OrganismPresets[n];
  if(kind==FmInstrument)return FmPresets[n];
  static const int p[4][SynthParameterCount] = {{1200,30,8,600,35,25,0,20,50,0,0,12,500,100,300,20,30,0,0,0},
                                  {700,20,350,1800,10,30,4,10,50,7,140,35,1800,80,2400,30,18,35,12,3},
                                  {220,88,3,350,80,20,0,48,50,0,85,8,260,50,220,45,110,0,0,0},
                                  {180,15,4,700,18,100,0,12,50,0,80,8,700,90,450,35,12,0,0,0}};
  return p[n];
}
inline const char *modeName(int kind, int mode) {
  return kind == MonoInstrument       ? (mode ? "PULSE" : "SAW")
         : kind == OrganismInstrument ? (mode ? "RING" : "PAIRS")
                                      : (mode ? "STACK" : "PAIRS");
}
} // namespace moss
