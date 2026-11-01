#pragma once

#include "light_state.h"
#include "settings.h"

// Speichert Zustand und Einstellungen im NVS-Flash (Preferences).
namespace storage {

void loadState(LightState& s);
void saveState(const LightState& s);
void loadSettings(Settings& s);
void saveSettings(const Settings& s);
// Löscht alles (Werkseinstellungen)
void clearAll();

}  // namespace storage
