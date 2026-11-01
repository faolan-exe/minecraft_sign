#pragma once

#include <stdint.h>

// Metadaten aller Effekte. Die Reihenfolge ist die Reihenfolge beim Durchschalten
// und der Index, der im Flash gespeichert wird: neue Effekte nur hinten anhängen.

enum EffectFlags : uint8_t {
  FX_COLOR1 = 1 << 0,
  FX_COLOR2 = 1 << 1,
};

struct EffectInfo {
  const char* id;              // stabil, für API und MQTT
  const char* name;            // Anzeigename
  uint8_t flags;
  const char* intensityLabel;  // nullptr, wenn der Effekt keine Intensität nutzt
};

constexpr uint8_t MAX_EFFECTS = 32;  // passt in die 32-Bit-Maske für das Durchschalten

namespace effects {

uint8_t count();
const EffectInfo& info(uint8_t index);
// Sucht nach id oder Anzeigename (Groß-/Kleinschreibung egal); -1, wenn unbekannt
int find(const char* idOrName);

}  // namespace effects
