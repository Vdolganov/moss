#pragma once
#include "parameter_spec.h"
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
inline int synthCount(int kind) { return kind==MonoInstrument?20:8; }
inline int synthBanks(int kind) { return synthCount(kind)/4; }
inline const char *synthBankName(int kind,int bank) {
  const char *mono[]={"CORE","COLOR","OSC","ENV","MOTION"};
  return kind==MonoInstrument?mono[bank]:bank?"CHARACTER":"CORE";
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
    {{"TENSION", 0, 100, "PERCENT"},
     {"DRIFT", 0, 100, "PERCENT"},
     {"CHAOS", 0, 100, "PERCENT"},
     {"DENSITY", 0, 100, "PERCENT"},
     {"COLOR", 0, 100, "PERCENT"},
     {"GRAIN", 0, 100, "PERCENT"},
     {"ATTACK", 0, 5000, "MS", 100},
     {"RELEASE", 0, 10000, "MS", 100}},
    {{"INDEX", 0, 100, "PERCENT"},
     {"RATIO", 0, 7, "MOD:CARRIER", 1},
     {"FEEDBACK", 0, 100, "PERCENT"},
     {"DECAY", 50, 10000, "MS", 100},
     {"DETUNE", 0, 100, "PERCENT"},
     {"NOISE", 0, 100, "PERCENT"},
     {"ATTACK", 0, 5000, "MS", 100},
     {"BODY", 0, 100, "PERCENT"}},
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
  const char *r[] = {"0.5:1", "1:1", "1.5:1", "2:1",
                     "3:1",   "4:1", "5:1",   "7:1"};
  return r[n];
}
inline const char *presetName(int kind, int n) {
  const char *p[3][4] = {{"SIGNAL", "SOFT", "ACID", "SUB"},
                         {"ROOT", "BREATH", "TANGLE", "BARE"},
                         {"GLASS", "BELL", "HAZE", "METAL"}};
  return p[kind - 1][n];
}
inline const int *presetValues(int kind, int n) {
  static const int p[3][4][SynthParameterCount] = {{{1200,30,8,600,35,25,0,20,50,0,0,12,500,100,300,20,30,0,0,0},
                                  {700,20,350,1800,10,30,4,10,50,7,140,35,1800,80,2400,30,18,35,12,3},
                                  {220,88,3,350,80,20,0,48,50,0,85,8,260,50,220,45,110,0,0,0},
                                  {180,15,4,700,18,100,0,12,50,0,80,8,700,90,450,35,12,0,0,0}},
                                 {{30, 35, 15, 65, 55, 8, 450, 2500},
                                  {15, 60, 8, 85, 40, 35, 1600, 6500},
                                  {75, 55, 65, 90, 85, 50, 100, 4000},
                                  {10, 20, 5, 20, 25, 0, 600, 5000}},
                                 {{45, 4, 12, 2200, 4, 0, 3, 0},
                                  {70, 5, 20, 4000, 12, 5, 3, 0},
                                  {25, 1, 5, 900, 25, 18, 350, 65},
                                  {85, 6, 65, 1600, 60, 30, 5, 15}}};
  return p[kind - 1][n];
}
inline const char *modeName(int kind, int mode) {
  return kind == MonoInstrument       ? (mode ? "PULSE" : "SAW")
         : kind == OrganismInstrument ? (mode ? "RING" : "PAIRS")
                                      : (mode ? "STACK" : "PAIRS");
}
} // namespace moss
