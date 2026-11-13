#pragma once
namespace moss {
inline constexpr char Version[] = "0.6.3-dev";
// +33% monitor gain over the previous 135%; retained soft peak protection.
constexpr int MonitorGainPercent = 180;
constexpr unsigned AudioRate = 32000, AudioBlockFrames = 1024;
constexpr int Tracks = 4, Keys = 4, Projects = 32, Steps = 512;
constexpr int EffectTypes = 5, EffectParameters = 8, ActiveEffects = 2,
              ParametersPerPage = 4, FxMoveField = EffectParameters + 2;
constexpr int Inherit = -32768;
enum HitField {
  HitOn,
  HitVelocity,
  HitFadeIn,
  HitFadeOut,
  HitProbability,
  HitLock0,
  HitLock1,
  HitLock2,
  HitLock3,
  HitLength,
  HitFieldCount
};
constexpr int hitField(int key, int field) {
  return key * HitFieldCount + field;
}
} // namespace moss
