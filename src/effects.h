#pragma once

#include <FastLED.h>

#include "layout.h"

struct FxParams {
  uint8_t speed;
  uint8_t intensity;
  CRGB color1;
  CRGB color2;
};

namespace fx {

void init();
// Nach einem Effektwechsel aufrufen, damit Effekte mit Zustand neu beginnen
void reset();
// Zeichnet ein Bild. now = millis(); die Geschwindigkeit wird intern auf eine
// virtuelle Zeit umgerechnet, damit Änderungen am Speed nicht springen.
void render(uint8_t effect, CRGB* leds, const FxParams& p, uint32_t now);

}  // namespace fx
