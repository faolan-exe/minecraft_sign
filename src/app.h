#pragma once

#include <Arduino.h>

#include "light_state.h"
#include "settings.h"

// Schnittstelle zwischen Hauptschleife und den anderen Tasks (Webserver, MQTT).
// Änderungen laufen als JSON-Kommando über eine Queue in die Hauptschleife;
// gelesen wird über Kopien unter einem Mutex.
namespace app {

// Calibrate: {"mode":"off"|"white"|"end"} – Testbild für die Strommessung
enum class CommandType : uint8_t { Light, Settings, Reboot, FactoryReset, Calibrate };

// json wird kopiert; false, wenn die Queue voll ist
bool post(CommandType type, const char* json = "");

LightState state();
Settings settings();
// Aktuelles Bild als RGB-Bytes, NUM_LEDS * 3
void copyFrame(uint8_t* out);
// Zählt bei jeder Zustandsänderung hoch (MQTT veröffentlicht dann neu)
uint32_t stateVersion();
uint32_t settingsVersion();
// Geschätzter Strom des letzten Bilds in mA
uint32_t estimatedMilliamps();
// "", "off" oder "white"
const char* calibrationMode();

}  // namespace app
