#pragma once
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
inline const char *synthLabel(int kind, int slot) {
  const char *n[3][8] = {{"CUTOFF", "RESONANCE", "ATTACK", "RELEASE", "ENV AMT",
                          "SUB", "NOISE", "DRIVE"},
                         {"TENSION", "DRIFT", "CHAOS", "DENSITY", "COLOR",
                          "GRAIN", "ATTACK", "RELEASE"},
                         {"INDEX", "RATIO", "FEEDBACK", "DECAY", "DETUNE",
                          "NOISE", "ATTACK", "BODY"}};
  return n[kind - 1][slot];
}
inline int synthMin(int kind, int slot) {
  return kind == MonoInstrument && slot == 0 ? 20
         : kind == FmInstrument && slot == 3 ? 50
                                             : 0;
}
inline int synthMax(int kind, int slot) {
  if (slot >= 4) {
    if ((kind == OrganismInstrument || kind == FmInstrument) && slot == 6)
      return 5000;
    if (kind == OrganismInstrument && slot == 7)
      return 10000;
    return 100;
  }
  if (kind == MonoInstrument) {
    const int v[] = {12000, 100, 5000, 10000};
    return v[slot];
  }
  if (kind == FmInstrument) {
    const int v[] = {100, 7, 100, 10000};
    return v[slot];
  }
  return 100;
}
inline const char *synthUnit(int kind, int slot) {
  if (slot >= 4)
    return ((kind == OrganismInstrument && slot >= 6) ||
            (kind == FmInstrument && slot == 6))
               ? "MS"
               : "PERCENT";
  return kind == MonoInstrument ? (slot == 0  ? "HZ"
                                   : slot > 1 ? "MS"
                                              : "PERCENT")
         : kind == FmInstrument ? (slot == 1   ? "MOD:CARRIER"
                                   : slot == 3 ? "MS"
                                               : "PERCENT")
                                : "PERCENT";
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
