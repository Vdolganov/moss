#pragma once
namespace moss {
inline constexpr char Version[] = "0.8.0-dev";
// +33% monitor gain over the previous 135%; retained soft peak protection.
constexpr int MonitorGainPercent = 180;
constexpr unsigned AudioRate = 32000, AudioBlockFrames = 1024;
constexpr int Tracks = 4, Keys = 4, Projects = 32, Steps = 512;
constexpr int EffectTypes = 5, EffectParameters = 16, ActiveEffects = 2,
              ParametersPerPage = 4, FxMoveField = EffectParameters + 2;
constexpr int Inherit = -32768;
constexpr int ResetLock = -32767;
enum EventType { EmptyEvent, NoteEvent, LockEvent };
constexpr int StepRepeatField = 10;
constexpr int StepShiftField = 11, TrackSwingField = 5;
constexpr int SwingStraight = 50, SwingMaximum = 75, ShiftLimit = 50;
inline bool lockValue(int value) { return value != Inherit && value != ResetLock; }
inline bool repeatValid(int value) {
  return value == Inherit || value == 0 || value == 8 || value == 16 || value == 32 || value == 64;
}
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
