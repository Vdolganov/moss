#pragma once
#include "parameter_spec.h"
namespace moss {
// Integer persistence/UI units: SPEED hundredths, scan coordinates tenths %.
inline constexpr int GranularParameterCount = 16, GranularBankCount = 4;
inline constexpr ParameterSpec GranularSpecs[GranularParameterCount] = {
    {"SPEED", -300, 300, "X", 10}, {"PITCH", -24, 24, "SEMITONES", 12},
    {"SIZE", 1, 8000, "MS", 100}, {"DENSITY", 0, 128, "GRAINS/S", 10},
    {"POSITION", 0, 1000, "PERCENT", 10}, {"START", 0, 999, "PERCENT", 10},
    {"END", 1, 1000, "PERCENT", 10}, {"SCATTER", 0, 100, "PERCENT", 10},
    {"STEREO", 0, 100, "PERCENT", 10}, {"JUMP", 0, 100, "PERCENT", 10},
    {"CUTOFF", 20, 14000, "HZ", 100}, {"LEVEL", 0, 200, "PERCENT", 10},
    {"GRAIN IN", 0, 100, "PERCENT", 10}, {"GRAIN OUT", 0, 100, "PERCENT", 10},
    {"NOTE IN", 0, 9000, "MS", 100}, {"NOTE OUT", 0, 9000, "MS", 100}};
inline constexpr const char *GranularBanks[] = {"PLAY", "SCAN", "TEXTURE", "ENVELOPE"};
inline constexpr int GranularRootNote = 48;
struct GranularPatch {
  int asset = -1, freeze = 0;
  int values[GranularParameterCount] = {12, -12, 2400, 8, 360, 0, 1000, 60, 65, 35, 7600, 100, 50, 50, 700, 700};
};
} // namespace moss
