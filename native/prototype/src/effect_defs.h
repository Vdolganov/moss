#pragma once
#include "config.h"
namespace moss {
struct Effect {
  int type = 0, enabled = 0;
  int values[4][4] = {
      {3, 35, 25, 60}, {60, 60, 45, 25}, {20, 12, 2, 25}, {20, 30, 10, 35}};
  int enabledTypes[4] = {}, order[4] = {0, 1, 2, 3};
  bool active(int id) const {
    return id == type ? enabled != 0 : enabledTypes[id] != 0;
  }
  int count() const {
    int n = 0;
    for (int i = 0; i < EffectTypes; ++i)
      n += active(i);
    return n;
  }
  bool enable(bool on) {
    if (on && !enabled && count() >= ActiveEffects)
      return false;
    enabled = enabledTypes[type] = on;
    return true;
  }
  void constrain() {
    for (int id : order)
      if (id < 0 || id >= EffectTypes)
        return;
    int n = 0;
    for (int id : order)
      if (active(id) && ++n > ActiveEffects) {
        enabledTypes[id] = 0;
        if (id == type)
          enabled = 0;
      }
  }
  void select(int id) {
    enabledTypes[type] = enabled;
    type = id;
    enabled = enabledTypes[id];
  }
  void move(int id, int direction) {
    for (int i = 0; i < 4; i++)
      if (order[i] == id) {
        int to = i + direction;
        if (to >= 0 && to < 4) {
          int old = order[to];
          order[to] = id;
          order[i] = old;
        }
        break;
      }
  }
};
inline int fxLow(int type, int slot) {
  return type == 2 && slot == 1 ? 2 : type == 2 && slot == 2 ? 1 : 0;
}
inline int fxHigh(int type, int slot) {
  return type == 0 && slot == 0   ? 7
         : type == 0 && slot == 1 ? 95
         : type == 2 && slot == 1 ? 16
         : type == 2 && slot == 2 ? 32
                                  : 100;
}
inline int fxCoarseStep(int type, int slot) {
  if (type == 0 && slot == 0)
    return 1; // note divisions are choices
  if (type == 2 && slot == 1)
    return 2;
  if (type == 2 && slot == 2)
    return 4;
  return 10;
}
inline const char *fxName(int type) {
  const char *n[] = {"ORBIT", "POND", "CRUSH", "REEL"};
  return n[type];
}
inline const char *fxLabel(int type, int slot) {
  const char *n[4][4] = {{"TIME", "FEEDBACK", "MIX", "SPREAD"},
                         {"SIZE", "DECAY", "DAMP", "MIX"},
                         {"DRIVE", "BITS", "RATE", "MIX"},
                         {"WOBBLE", "WEAR", "NOISE", "MIX"}};
  return n[type][slot];
}
} // namespace moss
