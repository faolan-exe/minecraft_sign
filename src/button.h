#pragma once

#include <stdint.h>

// Erkennt Klick, Doppelklick und langen Druck aus dem entprellten Pegel.
// Kein Hardwarezugriff: update() bekommt Pegel und Zeit, damit es testbar bleibt.

enum class ButtonEvent : uint8_t { None, Click, DoubleClick, LongPress };

class Button {
 public:
  static constexpr uint32_t DEBOUNCE_MS = 30;
  static constexpr uint32_t LONG_MS = 800;
  static constexpr uint32_t DOUBLE_GAP_MS = 300;

  // pressed: true, solange der Taster gedrückt ist
  ButtonEvent update(bool pressed, uint32_t now);

 private:
  bool rawLast_ = false;
  uint32_t rawSince_ = 0;
  bool stable_ = false;
  uint32_t pressStart_ = 0;
  uint32_t releaseAt_ = 0;
  bool longFired_ = false;
  uint8_t clicks_ = 0;
};
