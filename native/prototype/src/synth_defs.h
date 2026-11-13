#pragma once
#include "parameter_spec.h"
namespace moss {
enum Instrument {
  SampleInstrument,
  MonoInstrument,
  OrganismInstrument,
  FmInstrument
};
inline const char *instrumentName(int i) {
  const char *n[] = {"SAMPLER", "MONO", "ORGANISM", "FM"};
  return n[i];
}
inline constexpr ParameterSpec SynthParameters[3][8] = {
    {{"CUTOFF", 20, 12000, "HZ", 100},
     {"RESONANCE", 0, 100, "PERCENT"},
     {"ATTACK", 0, 5000, "MS", 100},
     {"RELEASE", 0, 10000, "MS", 100},
     {"ENV AMT", 0, 100, "PERCENT"},
     {"SUB", 0, 100, "PERCENT"},
     {"NOISE", 0, 100, "PERCENT"},
     {"DRIVE", 0, 100, "PERCENT"}},
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
  static const int p[3][4][8] = {{{1200, 30, 8, 600, 35, 25, 0, 20},
                                  {700, 20, 350, 1800, 10, 30, 8, 5},
                                  {400, 85, 3, 700, 80, 10, 5, 60},
                                  {200, 25, 3, 400, 15, 85, 0, 25}},
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
