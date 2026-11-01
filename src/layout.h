#pragma once

#include <stdint.h>

// Geometrie des Schilds, abgeleitet aus layout.txt.
// Koordinaten in LED-Abständen: x wächst nach rechts, y nach oben (0 = unten, 9 = oben).

constexpr uint16_t NUM_LEDS = 182;
constexpr uint8_t NUM_LETTERS = 9;
constexpr float LAYOUT_HEIGHT = 9.0f;

struct LedPos {
  float x;
  float y;
  uint8_t letter;  // 0 = M … 8 = T
};

struct LetterInfo {
  char glyph;
  uint16_t first;  // Index der ersten LED in der Kette
  uint16_t count;
  float x0;        // linker Rand
  float x1;        // rechter Rand
};

namespace layout {

// Baut die Tabellen auf; muss einmal vor allem anderen aufgerufen werden.
void build();

const LedPos& pos(uint16_t i);
const LetterInfo& letter(uint8_t l);
float width();

// Auf 0..255 normierte Koordinaten, für FastLED-Mathe
uint8_t nx(uint16_t i);
uint8_t ny(uint16_t i);
// Abstand zur Schildmitte in LED-Abständen
float radius(uint16_t i);
// Winkel um die Schildmitte, 0..255
uint8_t angle(uint16_t i);

// Schreibreihenfolge: so, wie man den Buchstaben mit dem Stift von links nach
// rechts schreibt (die Kette läuft bei M, N, R, A und T von rechts).
// penRank: Rang der LED innerhalb ihres Buchstabens, 0 = erster Strich links
uint8_t penRank(uint16_t i);
// penOrder: k-te LED in Leserichtung über das ganze Schild (k = 0 … NUM_LEDS-1)
uint16_t penOrder(uint16_t k);

}  // namespace layout
