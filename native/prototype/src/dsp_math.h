#pragma once
namespace moss {
// DSP state and validated parameters are finite. Unlike libm fmin/fmax these
// comparisons inline on ARMv7, without enabling unsafe global fast-math.
inline float minimum(float a, float b) { return a < b ? a : b; }
inline float maximum(float a, float b) { return a > b ? a : b; }
} // namespace moss
