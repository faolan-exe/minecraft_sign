#pragma once

// Hardware. Pins lassen sich per build_flags überschreiben, z. B. -DLED_PIN=6
#ifndef LED_PIN
#define LED_PIN 20     // Datenleitung der WS2812B (U0RXD, frei dank USB-CDC)
#endif
#ifndef BUTTON_PIN
#define BUTTON_PIN 21  // Taster gegen GND, interner Pull-up (U0TXD)
#endif

#define FW_VERSION "1.0.0"

constexpr uint8_t FPS = 60;
constexpr uint32_t SAVE_DELAY_MS = 5000;   // Änderungen gesammelt speichern, schont den Flash
constexpr uint32_t CROSSFADE_MS = 600;     // Überblendung beim Effektwechsel
constexpr uint32_t POWER_FADE_MS = 400;    // Ein-/Ausblenden
