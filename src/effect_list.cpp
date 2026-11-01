#include "effect_list.h"

#include <ctype.h>

namespace {

constexpr uint8_t C1 = FX_COLOR1;
constexpr uint8_t C12 = FX_COLOR1 | FX_COLOR2;

const EffectInfo EFFECTS[] = {
    {"solid", "Statisch", C1, nullptr},
    {"gradient", "Farbverlauf", C12, "Verschiebung"},
    {"rainbow", "Regenbogen", 0, "Streifenbreite"},
    {"rainbow_vertical", "Regenbogen vertikal", 0, "Streifenbreite"},
    {"rainbow_wheel", "Regenbogen-Rad", 0, "Wiederholungen"},
    {"letters", "Bunte Buchstaben", 0, "Farbabstand"},
    {"color_cycle", "Farbwechsel", 0, nullptr},
    {"breathe", "Atmen", C1, "Tiefe"},
    {"scanner", "Scanner", C1, "Schweiflänge"},
    {"typewriter", "Schreibmaschine", C1, "Pause"},
    {"letter_chase", "Buchstaben-Lauf", C12, "Nachleuchten"},
    {"trace", "Nachzeichnen", C1, "Schweiflänge"},
    {"theater", "Theater", C12, "Abstand"},
    {"twinkle", "Funkeln", C1, "Dichte"},
    {"confetti", "Konfetti", 0, "Dichte"},
    {"fire", "Feuer", 0, "Flammenhöhe"},
    {"plasma", "Plasma", 0, "Zoom"},
    {"ripple", "Wellen", C1, "Wellenlänge"},
    {"matrix", "Matrix", C1, "Dichte"},
    {"storm", "Gewitter", C1, "Blitzhäufigkeit"},
    {"grass_block", "Grasblock", 0, "Grashöhe"},
    {"enchanted", "Verzaubert", C1, "Glanzbreite"},
    {"redstone", "Redstone", 0, "Signallänge"},
    {"diamond", "Diamant", 0, "Glitzern"},
    {"creeper", "Creeper", 0, "Flecken"},
    {"nether_portal", "Nether-Portal", 0, "Zoom"},
    {"lava", "Lava", 0, "Zoom"},
    {"ocean", "Ozean", 0, "Zoom"},
    {"aurora", "Polarlicht", 0, "Bänder"},
    {"heartbeat", "Herzschlag", C1, "Nachglühen"},
};

constexpr uint8_t COUNT = sizeof(EFFECTS) / sizeof(EFFECTS[0]);
static_assert(COUNT <= MAX_EFFECTS, "zu viele Effekte für die Maske");

bool equalsIgnoreCase(const char* a, const char* b) {
  while (*a && *b) {
    if (tolower(static_cast<unsigned char>(*a)) != tolower(static_cast<unsigned char>(*b))) return false;
    a++;
    b++;
  }
  return *a == *b;
}

}  // namespace

namespace effects {

uint8_t count() { return COUNT; }

const EffectInfo& info(uint8_t index) { return EFFECTS[index < COUNT ? index : 0]; }

int find(const char* idOrName) {
  if (!idOrName) return -1;
  for (uint8_t i = 0; i < COUNT; i++) {
    if (equalsIgnoreCase(idOrName, EFFECTS[i].id) || equalsIgnoreCase(idOrName, EFFECTS[i].name)) return i;
  }
  return -1;
}

}  // namespace effects
