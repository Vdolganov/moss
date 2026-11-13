#pragma once
#include "../../../hardware lab/src/lab_ui.h"
namespace moss {
inline constexpr int ParameterButtons[] = {lab::Y, lab::B, lab::A, lab::X};
inline int parameterSlot(int button) {
  for (int i = 0; i < 4; ++i)
    if (ParameterButtons[i] == button)
      return i;
  return -1;
}
inline void bankTabs(lab::Canvas &c, const char *const *labels, int count,
                     int selected) {
  using namespace lab;
  for (int i = 0; i < count; ++i) {
    int x = 18 + i * 180;
    text(c, x, 108, labels[i], i == selected ? TEXT : DIM);
    if (i == selected)
      c.rect(x, 128, 140, 1, TEXT);
  }
  text(c, 478, 108, "L1/R1 BANK", DIM);
}
inline void parameterField(lab::Canvas &c, const lab::View &v, int col,
                           const char *label, const char *value,
                           const char *unit) {
  using namespace lab;
  int x = 18 + col * 153, button = ParameterButtons[col];
  uint32_t color = slots[button - A];
  bool held = (v.buttons.held & bit(button)) != 0;
  if (held)
    c.box(x, 346, 142, 89, color);
  c.rect(x, 346, 142, held ? 3 : 1, held ? color : LINE);
  text(c, x + 4, 357, names[button], color);
  text(c, x + 22, 357, label, color, 2, 10);
  text(c, x + 4, 385, value, TEXT, 3, 7);
  text(c, x + 4, 416, unit, held ? color : DIM, 2, 11);
}
} // namespace moss
