#pragma once
#include "parameter_spec.h"
namespace moss {
// Integer persistence/UI units: SPEED hundredths, scan coordinates tenths %.
inline constexpr ParameterSpec GranularSpecs[12] = {
    {"SPEED", -300, 300, "X", 10}, {"PITCH", -24, 24, "SEMITONES", 12},
    {"SIZE", 1, 500, "MS", 100}, {"DENSITY", 0, 128, "GRAINS/S", 10},
    {"POSITION", 0, 1000, "PERCENT", 10}, {"START", 0, 999, "PERCENT", 10},
    {"END", 1, 1000, "PERCENT", 10}, {"JITTER", 0, 500, "MS", 100},
    {"SPREAD", 0, 100, "PERCENT", 10}, {"IRREGULAR", 0, 100, "PERCENT", 10},
    {"CUTOFF", 20, 14000, "HZ", 100}, {"FADE", 0, 9000, "MS", 100}};
inline constexpr const char *GranularBanks[] = {"PLAY", "SCAN", "TEXTURE"};
inline constexpr int GranularRootNote = 48;
struct GranularPatch {
  int asset = -1, freeze = 0;
  int values[12] = {12, -12, 440, 14, 360, 80, 940, 180, 65, 25, 7600, 700};
};
} // namespace moss
