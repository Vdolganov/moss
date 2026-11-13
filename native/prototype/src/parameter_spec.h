#pragma once
namespace moss {
struct ParameterSpec {
  const char *label;
  int low, high;
  const char *unit;
  int coarse = 10;
};
} // namespace moss
