#include "button.h"

ButtonEvent Button::update(bool pressed, uint32_t now) {
  if (pressed != rawLast_) {
    rawLast_ = pressed;
    rawSince_ = now;
  }

  // Entprellen: Pegel erst übernehmen, wenn er DEBOUNCE_MS stabil war
  if (pressed != stable_ && now - rawSince_ >= DEBOUNCE_MS) {
    stable_ = pressed;
    if (stable_) {
      pressStart_ = now;
      longFired_ = false;
    } else {
      releaseAt_ = now;
      if (!longFired_) clicks_++;
      if (clicks_ >= 2) {
        clicks_ = 0;
        return ButtonEvent::DoubleClick;
      }
    }
  }

  // Langer Druck feuert schon beim Halten, nicht erst beim Loslassen
  if (stable_ && !longFired_ && now - pressStart_ >= LONG_MS) {
    longFired_ = true;
    clicks_ = 0;
    return ButtonEvent::LongPress;
  }

  // Einzelklick erst, wenn kein zweiter Druck mehr kommt
  if (!stable_ && clicks_ == 1 && now - releaseAt_ >= DOUBLE_GAP_MS) {
    clicks_ = 0;
    return ButtonEvent::Click;
  }

  return ButtonEvent::None;
}
