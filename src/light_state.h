#pragma once

#include <ArduinoJson.h>
#include <stdint.h>

#include "button.h"

// Alles, was das Leuchtbild bestimmt. Wird im Flash gespeichert und über
// Weboberfläche, MQTT und Taster mit denselben Regeln geändert.

enum class PowerOnMode : uint8_t { Last = 0, On = 1, Off = 2 };

struct LightState {
  bool on = true;
  uint8_t effect = 2;  // Regenbogen
  uint8_t brightness = 128;
  uint8_t speed = 128;
  uint8_t intensity = 128;
  uint32_t color1 = 0xFF6A00;  // RGB
  uint32_t color2 = 0x0040FF;
  uint32_t cycleMask = 0;      // Effekte für das Durchschalten per Taster; 0 = alle
  uint16_t autoCycleSec = 0;   // automatischer Wechsel, 0 = aus
};

bool operator==(const LightState& a, const LightState& b);

// Helligkeitsstufen für den Doppelklick
constexpr uint8_t BRIGHTNESS_STEPS[] = {32, 80, 160, 255};
// Untergrenze, damit „an“ nie dunkel aussieht
constexpr uint8_t MIN_BRIGHTNESS = 1;

namespace light {

// Nächster Effekt in der Durchschalt-Liste (zyklisch). Ist die Maske leer oder
// enthält sie keinen gültigen Effekt, wird durch alle Effekte geschaltet.
uint8_t nextEffect(const LightState& s);

// Nächste Helligkeitsstufe über der aktuellen; nach der höchsten wieder die niedrigste.
uint8_t nextBrightnessStep(uint8_t current);

// Taster-Ereignis anwenden. Gibt true zurück, wenn der Zustand sofort
// gespeichert werden soll (beim Ausschalten).
bool handleButton(ButtonEvent ev, LightState& s);

// Zustand nach dem Start abhängig von der Einstellung „Beim Einschalten“
void applyPowerOn(PowerOnMode mode, LightState& s);

// Wendet ein JSON-Kommando an (Web-API, MQTT inkl. Home-Assistant-Format).
// Unbekannte Felder werden ignoriert, Werte begrenzt. Gibt true bei Änderung zurück.
bool applyJson(JsonVariantConst cmd, LightState& s);

void toJson(const LightState& s, JsonObject out);

// "#rrggbb" ⇄ 0xRRGGBB; parseColor gibt false bei ungültigem Text zurück
bool parseColor(const char* text, uint32_t& out);
void formatColor(uint32_t rgb, char out[8]);

}  // namespace light
