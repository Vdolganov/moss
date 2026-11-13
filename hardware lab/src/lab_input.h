#ifndef MOSS_LAB_INPUT_H
#define MOSS_LAB_INPUT_H
#include "lab_core.h"

namespace lab {
// Physical labels follow this device's GarlicOS RG35XX Gamepad.cfg:
// SDL buttons 0,1,2,3,5,6,7,8,9 -> A,B,X,Y,L1,R1,SELECT,START,MENU.
// Linux button codes are sparse, so SDL indices must not be used as evdev codes.
enum class InputProfile { Unknown, LegacyKeys, GarlicGamepad };
inline const char *profileName(InputProfile p) {
  return p == InputProfile::GarlicGamepad ? "GARLIC GAMEPAD"
         : p == InputProfile::LegacyKeys  ? "LEGACY KEYS"
                                          : "UNMAPPED";
}
inline int gamepadButton(int code) {
  switch (code) {
  case 304:
    return A;
  case 305:
    return B;
  case 307:
    return X;
  case 308:
    return Y;
  case 310:
    return L1;
  case 311:
    return R1;
  case 314:
    return Select;
  case 315:
    return Start;
  case 316:
    return Menu;
  default:
    return -1;
  }
}
struct TriggerRange {
  int minimum = 0, maximum = 255;
  bool known = false, pressed = false;
  void configure(int lo, int hi) {
    minimum = lo;
    maximum = hi;
    known = hi > lo;
    pressed = false;
  }
  bool apply(int value) {
    if (!known)
      return false; // Never guess a device's trigger resting value.
    int64_t width = int64_t(maximum) - minimum;
    int64_t position = int64_t(value) - minimum;
    if (!pressed && position * 3 >= width * 2)
      pressed = true;
    else if (pressed && position * 3 <= width)
      pressed = false;
    return true;
  }
};
struct RawInputState {
  InputProfile profile = InputProfile::Unknown;
  uint32_t keys = 0, hat = 0;
  TriggerRange triggers[2];
  uint32_t held() const {
    return keys | hat | (triggers[0].pressed ? bit(L2) : 0) | (triggers[1].pressed ? bit(R2) : 0);
  }
  void clear() {
    keys = hat = 0;
    triggers[0].pressed = triggers[1].pressed = false;
  }
  int mapKey(int code) const {
    return profile == InputProfile::GarlicGamepad ? gamepadButton(code)
           : profile == InputProfile::LegacyKeys  ? rawButton(code)
                                                  : -1;
  }
  bool key(int code, int value) {
    int button = mapKey(code);
    if (button < 0)
      return false;
    if (value == 2)
      return true; // Kernel repeat is not a new edge.
    if (value != 0 && value != 1)
      return false;
    if (value)
      keys |= bit(button);
    else
      keys &= ~bit(button);
    return true;
  }
  bool axis(int code, int value) {
    if (profile != InputProfile::GarlicGamepad)
      return false;
    if (code == 16) {
      hat &= ~(bit(Left) | bit(Right));
      if (value < 0)
        hat |= bit(Left);
      if (value > 0)
        hat |= bit(Right);
      return true;
    }
    if (code == 17) {
      hat &= ~(bit(Up) | bit(Down));
      if (value < 0)
        hat |= bit(Up);
      if (value > 0)
        hat |= bit(Down);
      return true;
    }
    if (code == 2 || code == 5)
      return triggers[code == 2 ? 0 : 1].apply(value);
    return false;
  }
};
} // namespace lab
#endif
