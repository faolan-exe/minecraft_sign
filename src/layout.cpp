#include "layout.h"

#include <math.h>

// Jeder Buchstabe wird in Kettenreihenfolge beschrieben, genau wie in layout.txt.
// Lokale Koordinaten beginnen bei x = 0; der Versatz ergibt sich aus der Breite
// der vorherigen Buchstaben plus Abstand.

namespace {

constexpr float LETTER_GAP = 2.5f;

LedPos leds[NUM_LEDS];
LetterInfo letters[NUM_LETTERS];
uint8_t normX[NUM_LEDS];
uint8_t normY[NUM_LEDS];
float rad[NUM_LEDS];
uint8_t ang[NUM_LEDS];
uint8_t rank[NUM_LEDS];
uint8_t order[NUM_LEDS];
float totalWidth = 0;

uint16_t count = 0;
uint8_t curLetter = 0;
float offset = 0;

void add(float x, float y) {
  if (count >= NUM_LEDS) return;
  leds[count++] = {offset + x, y, curLetter};
}

// Senkrechte Reihe von n LEDs, von yFrom in Richtung yTo (Schritt 1)
void column(float x, int yFrom, int yTo) {
  int step = yTo >= yFrom ? 1 : -1;
  for (int y = yFrom;; y += step) {
    add(x, static_cast<float>(y));
    if (y == yTo) break;
  }
}

void beginLetter(char glyph, float width) {
  letters[curLetter] = {glyph, count, 0, offset, offset + width};
}

void endLetter() {
  LetterInfo& l = letters[curLetter];
  l.count = count - l.first;
  offset = l.x1 + LETTER_GAP;
  curLetter++;
}

// M hat nur 9 Zeilen; sie werden auf die volle Höhe gestreckt.
float mLine(int line) { return (8 - line) * (LAYOUT_HEIGHT / 8.0f); }

void buildM() {
  beginLetter('M', 10);
  for (int line = 8; line >= 1; line--) add(10, mLine(line));  // 1–8 rechts hoch
  add(9.5f, mLine(0));                                          // 9 Ecke oben rechts
  add(8.8f, mLine(1));                                          // 10–12 Diagonale
  add(7.7f, mLine(2));
  add(6.5f, mLine(3));
  add(5.5f, mLine(3));                                          // 13, 14 Spitze
  add(4.5f, mLine(3));
  add(3.5f, mLine(3));                                          // 15–17 Diagonale
  add(2.3f, mLine(2));
  add(1.2f, mLine(1));
  add(0.5f, mLine(0));                                          // 18 Ecke oben links
  for (int line = 1; line <= 8; line++) add(0, mLine(line));   // 19–26 links runter
  endLetter();
}

void buildI() {
  beginLetter('I', 0);
  column(0, 0, 9);
  endLetter();
}

void buildN() {
  beginLetter('N', 5);
  column(5, 0, 9);  // 1–10 rechts hoch
  add(4, 4);        // 11–14 Diagonale nach oben links
  add(3, 5);
  add(2, 7);
  add(1, 8);
  column(0, 9, 0);  // 15–24 links runter
  endLetter();
}

// Linke Senkrechte mit Querbalken (E, C, F)
void bar(int y) {
  for (int x = 1; x <= 4; x++) add(static_cast<float>(x), static_cast<float>(y));
}

void buildE() {
  beginLetter('E', 4);
  column(0, 0, 9);  // 1–10
  bar(9);           // 11–14
  bar(5);           // 15–18
  bar(0);           // 19–22
  endLetter();
}

void buildC() {
  beginLetter('C', 4);
  column(0, 0, 9);  // 1–10
  bar(9);           // 11–14
  bar(0);           // 15–18
  endLetter();
}

void buildR() {
  beginLetter('R', 5);
  column(5, 0, 9);   // 1–10 rechts hoch
  add(3.33f, 9);     // 11, 12 oben
  add(1.67f, 9);
  add(3.33f, 5);     // 13, 14 Mitte
  add(1.67f, 5);
  column(0, 9, 0);   // 15–24 links runter
  endLetter();
}

void buildA() {
  beginLetter('A', 5);
  column(5, 0, 9);   // 1–10 rechts hoch
  add(4, 9);         // 11, 12, 12.5, 12.6 oben
  add(3, 9);
  add(2, 9);
  add(1, 9);
  add(3.33f, 5);     // 13, 14 Mitte
  add(1.67f, 5);
  column(0, 9, 0);   // 15–24 links runter
  endLetter();
}

void buildF() {
  beginLetter('F', 4);
  column(0, 0, 9);  // 1–10
  bar(9);           // 11–14
  bar(5);           // 15–18
  endLetter();
}

void buildT() {
  beginLetter('T', 4);
  column(2, 0, 9);  // 1–10 Stamm, 10 = Mitte des Balkens
  add(3, 9);        // 11, 12 rechts
  add(4, 9);
  add(1, 9);        // 13, 14 links
  add(0, 9);
  endLetter();
}

// Schreibreihenfolge je Buchstabe als 0-basierte Kettenindizes (LED n aus layout.txt = n-1).
// Buchstaben, deren Kette schon links beginnt (I, E, C, F), nutzen die Kettenreihenfolge.
const uint8_t PEN_M[] = {25, 24, 23, 22, 21, 20, 19, 18, 17,  // 26–18 links hoch
                         16, 15, 14, 13, 12, 11, 10, 9,       // 17–10 V
                         8,  7,  6,  5,  4,  3,  2,  1,  0};  // 9–1 rechts runter
const uint8_t PEN_N[] = {23, 22, 21, 20, 19, 18, 17, 16, 15, 14,  // 24–15 links hoch
                         13, 12, 11, 10,                          // 14–11 Diagonale runter
                         0,  1,  2,  3,  4,  5,  6,  7,  8,  9};  // 1–10 rechts hoch
const uint8_t PEN_R[] = {23, 22, 21, 20, 19, 18, 17, 16, 15, 14,  // 24–15 links hoch
                         11, 10,                                  // 12, 11 oben
                         9,  8,  7,  6,  5,  4,  3,  2,  1,  0,   // 10–1 rechts runter
                         13, 12};                                 // 14, 13 Mitte
const uint8_t PEN_A[] = {25, 24, 23, 22, 21, 20, 19, 18, 17, 16,  // 24–15 links hoch
                         13, 12, 11, 10,                          // 12.6, 12.5, 12, 11 oben
                         9,  8,  7,  6,  5,  4,  3,  2,  1,  0,   // 10–1 rechts runter
                         15, 14};                                 // 14, 13 Mitte
const uint8_t PEN_T[] = {13, 12, 9, 10, 11,           // 14, 13, 10, 11, 12 Balken
                         8,  7,  6, 5,  4, 3, 2, 1, 0};  // 9–1 Stamm runter

struct PenOrder {
  const uint8_t* seq;  // nullptr = Kettenreihenfolge
  uint8_t len;
};

const PenOrder PEN[NUM_LETTERS] = {
    {PEN_M, sizeof(PEN_M)}, {nullptr, 0}, {PEN_N, sizeof(PEN_N)}, {nullptr, 0}, {nullptr, 0},
    {PEN_R, sizeof(PEN_R)}, {PEN_A, sizeof(PEN_A)}, {nullptr, 0}, {PEN_T, sizeof(PEN_T)},
};

void buildPenOrder() {
  uint16_t k = 0;
  for (uint8_t l = 0; l < NUM_LETTERS; l++) {
    const LetterInfo& info = letters[l];
    for (uint16_t r = 0; r < info.count; r++) {
      const uint16_t local = (PEN[l].seq && r < PEN[l].len) ? PEN[l].seq[r] : r;
      const uint16_t i = info.first + local;
      rank[i] = static_cast<uint8_t>(r);
      order[k++] = static_cast<uint8_t>(i);
    }
  }
}

}  // namespace

namespace layout {

void build() {
  count = 0;
  curLetter = 0;
  offset = 0;
  buildM();
  buildI();
  buildN();
  buildE();
  buildC();
  buildR();
  buildA();
  buildF();
  buildT();
  totalWidth = letters[NUM_LETTERS - 1].x1;

  const float cx = totalWidth / 2;
  const float cy = LAYOUT_HEIGHT / 2;
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    normX[i] = static_cast<uint8_t>(lroundf(leds[i].x / totalWidth * 255));
    normY[i] = static_cast<uint8_t>(lroundf(leds[i].y / LAYOUT_HEIGHT * 255));
    const float dx = leds[i].x - cx;
    const float dy = leds[i].y - cy;
    rad[i] = sqrtf(dx * dx + dy * dy);
    const float a = atan2f(dy, dx);  // -pi..pi
    ang[i] = static_cast<uint8_t>(static_cast<int>(lroundf((a + static_cast<float>(M_PI)) / (2 * static_cast<float>(M_PI)) * 256)) & 0xFF);
  }
  buildPenOrder();
}

const LedPos& pos(uint16_t i) { return leds[i]; }
const LetterInfo& letter(uint8_t l) { return letters[l]; }
float width() { return totalWidth; }
uint8_t nx(uint16_t i) { return normX[i]; }
uint8_t ny(uint16_t i) { return normY[i]; }
float radius(uint16_t i) { return rad[i]; }
uint8_t angle(uint16_t i) { return ang[i]; }
uint8_t penRank(uint16_t i) { return rank[i]; }
uint16_t penOrder(uint16_t k) { return order[k]; }

}  // namespace layout
