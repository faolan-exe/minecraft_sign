#include "effects.h"

#include "effect_list.h"

// Effekte arbeiten mit der virtuellen Zeit vt (ms, durch Speed skaliert) und den
// Koordinaten aus layout.h. Effekte mit Zustand (Funken, Tropfen …) rechnen in
// festen Schritten von TICK_MS virtueller Zeit, damit sie unabhängig von der
// Bildrate gleich schnell laufen.

namespace {

constexpr uint32_t TICK_MS = 16;
constexpr uint8_t MAX_DROPS = 24;
constexpr uint8_t MAX_COLUMNS = 64;

// LED-Position in 1/16 LED-Abstand, für Ganzzahl-Rechnung (der C3 hat keine FPU)
int16_t px[NUM_LEDS];
int16_t py[NUM_LEDS];
uint8_t radius16[NUM_LEDS];  // Abstand zur Mitte in 1/4 LED-Abstand
uint8_t localPos[NUM_LEDS];  // Position innerhalb des Buchstabens, 0..255
int16_t columns[MAX_COLUMNS];
uint8_t numColumns = 0;
int16_t widthPx = 0;

// Gemeinsamer Zustand der Effekte; reset() setzt alles zurück
uint8_t level[NUM_LEDS];
uint8_t letterLevel[NUM_LETTERS];
struct Drop {
  int16_t x;
  int16_t y;  // Kopf, 1/16 LED
  bool active;
};
Drop drops[MAX_DROPS];
uint32_t vt = 0;
uint32_t lastNow = 0;
uint32_t tickAcc = 0;
uint8_t flash = 0;
int16_t flashX = 0;
uint8_t flashRepeats = 0;
uint32_t nextFlashAt = 0;

uint32_t ticksDue() {
  uint32_t n = tickAcc / TICK_MS;
  tickAcc -= n * TICK_MS;
  return n > 8 ? 8 : n;  // nach langer Pause nicht nachholen
}

CRGB scaled(CRGB c, uint8_t v) { return c.nscale8_video(v); }

// --- einfache Farbeffekte ---------------------------------------------------

void solid(CRGB* leds, const FxParams& p) { fill_solid(leds, NUM_LEDS, p.color1); }

void gradient(CRGB* leds, const FxParams& p) {
  const uint8_t shift = p.intensity ? static_cast<uint8_t>((vt / 24) * p.intensity / 255) : 0;
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = blend(p.color1, p.color2, triwave8((layout::nx(i) >> 1) + shift));
  }
}

void rainbowAlong(CRGB* leds, const FxParams& p, bool vertical) {
  const uint16_t stretch = 16 + p.intensity;
  const uint8_t base = vt >> 3;
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const uint8_t c = vertical ? layout::ny(i) : layout::nx(i);
    leds[i] = CHSV(static_cast<uint8_t>(((c * stretch) >> 7) - base), 255, 255);
  }
}

void rainbowWheel(CRGB* leds, const FxParams& p) {
  const uint8_t reps = 1 + p.intensity / 64;
  const uint8_t base = vt >> 3;
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(static_cast<uint8_t>(layout::angle(i) * reps + base), 255, 255);
  }
}

void letters(CRGB* leds, const FxParams& p) {
  const uint8_t spread = 8 + p.intensity / 4;
  const uint8_t base = vt >> 4;
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(static_cast<uint8_t>(base + layout::pos(i).letter * spread), 255, 255);
  }
}

void colorCycle(CRGB* leds) { fill_solid(leds, NUM_LEDS, CHSV(vt >> 5, 255, 255)); }

void breathe(CRGB* leds, const FxParams& p) {
  const uint8_t low = 255 - p.intensity;
  const uint8_t wave = cubicwave8(vt >> 4);
  fill_solid(leds, NUM_LEDS, scaled(p.color1, low + scale8(wave, 255 - low)));
}

// --- Bewegung über die Fläche -----------------------------------------------

void scanner(CRGB* leds, const FxParams& p) {
  const uint32_t ticks = ticksDue();
  const uint8_t fade = 90 - scale8(p.intensity, 84);
  for (uint32_t t = 0; t < ticks; t++) fadeToBlackBy(leds, NUM_LEDS, fade);
  // Dreieckswelle über die ganze Breite plus etwas Überstand
  const int32_t span = widthPx + 48;
  const uint16_t phase = static_cast<uint16_t>(vt * 20);
  const int32_t head = static_cast<int32_t>(triwave8(phase >> 8)) * span / 255 - 24;
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    if (abs(px[i] - head) < 14) leds[i] = p.color1;
  }
}

void typewriter(CRGB* leds, const FxParams& p) {
  constexpr uint32_t STEP = 350;
  const uint32_t pause = 2 + p.intensity / 32;
  const uint32_t total = NUM_LETTERS + pause + 2;
  const uint32_t k = (vt / STEP) % total;
  const uint8_t frac = static_cast<uint8_t>((vt % STEP) * 255 / STEP);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  if (k >= NUM_LETTERS + pause) {
    // Ausblenden vor dem Neustart
    const uint8_t v = k == NUM_LETTERS + pause ? 255 - frac : 0;
    fill_solid(leds, NUM_LEDS, scaled(p.color1, v));
    return;
  }
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const uint8_t l = layout::pos(i).letter;
    if (l < k) leds[i] = p.color1;
    else if (l == k && localPos[i] <= frac) leds[i] = p.color1;
  }
}

void letterChase(CRGB* leds, const FxParams& p) {
  const uint32_t ticks = ticksDue();
  const uint8_t active = (vt / 300) % NUM_LETTERS;
  const uint8_t decay = 40 - scale8(p.intensity, 36);
  for (uint32_t t = 0; t < ticks; t++) {
    for (uint8_t l = 0; l < NUM_LETTERS; l++) letterLevel[l] = qsub8(letterLevel[l], decay);
  }
  letterLevel[active] = 255;
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = blend(p.color1, p.color2, letterLevel[layout::pos(i).letter]);
  }
}

// Ein Komet läuft entlang der Verkabelung jedes Buchstabens und zeichnet ihn nach
void trace(CRGB* leds, const FxParams& p) {
  const uint8_t head = vt / 8;
  const uint8_t tail = 16 + scale8(p.intensity, 200);
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const uint8_t behind = head - localPos[i];
    uint8_t v = 18;
    if (behind < tail) v = qadd8(v, 255 - behind * 255 / tail);
    leds[i] = scaled(p.color1, dim8_video(v));
  }
}

void theater(CRGB* leds, const FxParams& p) {
  const uint8_t spacing = 3 + p.intensity / 64;
  const uint32_t offset = vt / 120;
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const LetterInfo& l = layout::letter(layout::pos(i).letter);
    const uint32_t local = i - l.first;
    leds[i] = ((local + offset) % spacing == 0) ? p.color1 : p.color2;
  }
}

void twinkle(CRGB* leds, const FxParams& p) {
  const uint32_t ticks = ticksDue();
  for (uint32_t t = 0; t < ticks; t++) {
    for (uint16_t i = 0; i < NUM_LEDS; i++) level[i] = qsub8(level[i], 6);
    if (random8() < 20 + p.intensity / 2) level[random16(NUM_LEDS)] = 255;
  }
  const CRGB base = scaled(p.color1, 50);
  const CRGB sparkle = blend(p.color1, CRGB::White, 140);
  for (uint16_t i = 0; i < NUM_LEDS; i++) leds[i] = blend(base, sparkle, ease8InOutQuad(level[i]));
}

void confetti(CRGB* leds, const FxParams& p) {
  const uint32_t ticks = ticksDue();
  for (uint32_t t = 0; t < ticks; t++) {
    fadeToBlackBy(leds, NUM_LEDS, 8);
    if (random8() < 30 + p.intensity / 2) {
      leds[random16(NUM_LEDS)] += CHSV(static_cast<uint8_t>((vt >> 5) + random8(64)), 200, 255);
    }
  }
}

// --- Rauschen und Paletten --------------------------------------------------

void fire(CRGB* leds, const FxParams& p) {
  // Rauschen steigt nach oben, wird mit der Höhe abgeschwächt
  const uint8_t cut = 255 - p.intensity / 2;
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    uint8_t heat = inoise8(px[i] * 6, py[i] * 6 - vt / 2, vt / 8);
    heat = qadd8(heat, 40);
    heat = qsub8(heat, scale8(layout::ny(i), cut));
    leds[i] = HeatColor(scale8(heat, 240));
  }
}

void noisePalette(CRGB* leds, const CRGBPalette16& pal, uint8_t zoomParam, uint8_t drift) {
  const uint16_t zoom = 4 + zoomParam / 6;
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const uint8_t n = inoise8(px[i] * zoom, py[i] * zoom, vt / 6);
    leds[i] = ColorFromPalette(pal, n + (vt >> drift), 255, LINEARBLEND);
  }
}

void plasma(CRGB* leds, const FxParams& p) {
  const uint16_t zoom = 4 + p.intensity / 6;
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const uint8_t hue = inoise8(px[i] * zoom, py[i] * zoom, vt / 5);
    const uint8_t val = inoise8(px[i] * zoom + 5000, py[i] * zoom, vt / 7);
    leds[i] = CHSV(hue * 2 + (vt >> 6), 240, qadd8(scale8(val, 160), 95));
  }
}

void ripple(CRGB* leds, const FxParams& p) {
  const uint8_t k = 8 + p.intensity / 6;
  const uint8_t phase = vt >> 2;
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const uint8_t v = sin8(static_cast<uint8_t>(radius16[i] * k / 4 - phase));
    leds[i] = scaled(p.color1, dim8_video(v));
  }
}

void matrix(CRGB* leds, const FxParams& p) {
  const uint32_t ticks = ticksDue();
  for (uint32_t t = 0; t < ticks; t++) {
    fadeToBlackBy(leds, NUM_LEDS, 28);
    if (random8() < 6 + p.intensity / 5) {
      for (Drop& d : drops) {
        if (d.active) continue;
        d = {columns[random8(numColumns)], static_cast<int16_t>(LAYOUT_HEIGHT * 16 + 16), true};
        break;
      }
    }
    for (Drop& d : drops) {
      if (!d.active) continue;
      d.y -= 3;
      if (d.y < -16) d.active = false;
    }
  }
  const CRGB head = blend(p.color1, CRGB::White, 160);
  for (const Drop& d : drops) {
    if (!d.active) continue;
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      if (abs(px[i] - d.x) < 6 && abs(py[i] - d.y) < 10) leds[i] = head;
      else if (abs(px[i] - d.x) < 6 && py[i] > d.y && py[i] - d.y < 24) leds[i] |= p.color1;
    }
  }
}

void storm(CRGB* leds, const FxParams& p) {
  const uint32_t ticks = ticksDue();
  for (uint32_t t = 0; t < ticks; t++) {
    flash = qsub8(flash, 22);
    if (flash == 0 && flashRepeats > 0 && random8() < 90) {
      flash = 160 + random8(95);
      flashRepeats--;
    }
  }
  if (vt >= nextFlashAt && flashRepeats == 0 && flash == 0) {
    flashX = random16(widthPx);
    flashRepeats = 1 + random8(4);
    const uint32_t gap = 6000 - p.intensity * 20;
    nextFlashAt = vt + gap / 2 + random16(gap);
  }
  const CRGB base = scaled(p.color1, 30);
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const int16_t d = abs(px[i] - flashX);
    const uint8_t v = scale8(flash, d < 255 ? 255 - d : 0);
    leds[i] = blend(base, CRGB(200, 210, 255), v);
  }
}

// --- Minecraft --------------------------------------------------------------

void grassBlock(CRGB* leds, const FxParams& p) {
  const int16_t grassFrom = static_cast<int16_t>((LAYOUT_HEIGHT - 1 - p.intensity / 64) * 16);
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const uint8_t n = inoise8(px[i] * 40, py[i] * 40, vt / 20);
    if (py[i] >= grassFrom) {
      leds[i] = CHSV(72 + n / 16, 200, 140 + n / 3);
    } else {
      // Erde, vereinzelt Kies
      leds[i] = n > 200 ? CRGB(90, 90, 90) : CRGB(110 + n / 6, 60 + n / 10, 25);
    }
  }
}

void enchanted(CRGB* leds, const FxParams& p) {
  const uint8_t width = 20 + p.intensity / 3;
  const CRGB glint(170, 90, 255);
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    // Zwei Glanzstreifen in unterschiedlichen Winkeln, wie bei verzauberten Gegenständen
    const uint16_t d1 = layout::nx(i) * 2 + layout::ny(i);
    const uint16_t d2 = layout::nx(i) * 2 - layout::ny(i) + 255;
    const uint8_t a = static_cast<uint8_t>(d1 - vt / 5);
    const uint8_t b = static_cast<uint8_t>(d2 + vt / 7);
    uint8_t v = 0;
    if (a < width) v = qadd8(v, sin8(a * 128 / width));
    if (b < width) v = qadd8(v, scale8(sin8(b * 128 / width), 180));
    leds[i] = blend(scaled(p.color1, 120), glint, v);
  }
}

// Ein Signal läuft wie über Redstone-Staub durch die gesamte Kette
void redstone(CRGB* leds, const FxParams& p) {
  const uint16_t len = 8 + p.intensity / 6;
  const uint16_t head = (vt / 20) % (NUM_LEDS + len);
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    uint8_t v = 35 + scale8(inoise8(i * 60, vt / 3), 25);
    if (i <= head && head - i < len) v = qadd8(v, 255 - (head - i) * 200 / len);
    leds[i] = CRGB(v, v > 200 ? (v - 200) : 0, 0);
  }
}

void diamond(CRGB* leds, const FxParams& p) {
  const uint32_t ticks = ticksDue();
  for (uint32_t t = 0; t < ticks; t++) {
    for (uint16_t i = 0; i < NUM_LEDS; i++) level[i] = qsub8(level[i], 10);
    if (random8() < 10 + p.intensity / 3) level[random16(NUM_LEDS)] = 255;
  }
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const uint8_t n = inoise8(px[i] * 30, py[i] * 30, vt / 10);
    const CRGB base = blend(CRGB(20, 160, 150), CRGB(120, 240, 230), n);
    leds[i] = blend(base, CRGB::White, level[i]);
  }
}

void creeper(CRGB* leds, const FxParams& p) {
  static const CRGB SHADES[] = {CRGB(10, 90, 10), CRGB(30, 140, 25), CRGB(80, 190, 70), CRGB(160, 220, 150)};
  const uint16_t zoom = 30 + p.intensity / 2;
  // Alle 16 s zischt er: Blinken, dann Explosion
  constexpr uint32_t CYCLE = 16000;
  const uint32_t c = vt % CYCLE;
  uint8_t white = 0;
  if (c > CYCLE - 2600 && c < CYCLE - 800) {
    const uint32_t rel = c - (CYCLE - 2600);
    const uint32_t period = 500 - rel / 5;
    white = (rel % period) < period / 2 ? 170 : 0;
  } else if (c >= CYCLE - 800) {
    white = 255 - (c - (CYCLE - 800)) * 255 / 800;
  }
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const uint8_t n = inoise8(px[i] * zoom, py[i] * zoom, vt / 40);
    leds[i] = blend(SHADES[n / 64], CRGB::White, white);
  }
}

const CRGBPalette16 PORTAL_PAL(CRGB(20, 0, 40), CRGB(60, 0, 120), CRGB(130, 30, 220), CRGB(220, 150, 255));

void netherPortal(CRGB* leds, const FxParams& p) {
  const uint16_t zoom = 10 + p.intensity / 4;
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    // Verwirbeltes Rauschen: Winkel und Abstand zur Mitte statt x/y
    const uint8_t n = inoise8(layout::angle(i) * 8 + vt / 4, radius16[i] * zoom / 2 - vt / 3, vt / 9);
    leds[i] = ColorFromPalette(PORTAL_PAL, n, 255, LINEARBLEND);
  }
}

void aurora(CRGB* leds, const FxParams& p) {
  const uint16_t zoom = 6 + p.intensity / 8;
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const uint8_t band = inoise8(px[i] * zoom, vt / 4);
    const uint8_t hue = 96 + scale8(inoise8(px[i] * 3 + 9000, vt / 9), 110);
    // Vorhang von oben, unten ausgeblendet
    const uint8_t curtain = qsub8(layout::ny(i), 255 - band);
    leds[i] = CHSV(hue, 230, dim8_video(qadd8(curtain, scale8(band, 60))));
  }
}

void heartbeat(CRGB* leds, const FxParams& p) {
  constexpr uint32_t PERIOD = 1100;
  const uint8_t decay = 3 + (255 - p.intensity) / 32;
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    // Puls breitet sich von der Mitte nach außen aus
    const uint32_t local = (vt + PERIOD * 4 - radius16[i] * 6) % PERIOD;
    uint16_t v = 0;
    if (local < 600) v += 255 * 60 / (60 + local * decay);
    if (local >= 220 && local < 800) v += 200 * 60 / (60 + (local - 220) * decay);
    leds[i] = scaled(p.color1, v > 255 ? 255 : static_cast<uint8_t>(v));
  }
}

}  // namespace

namespace fx {

void init() {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    const LedPos& lp = layout::pos(i);
    px[i] = static_cast<int16_t>(lp.x * 16);
    py[i] = static_cast<int16_t>(lp.y * 16);
    radius16[i] = static_cast<uint8_t>(layout::radius(i) * 4);
    const LetterInfo& l = layout::letter(lp.letter);
    localPos[i] = static_cast<uint8_t>((i - l.first) * 255 / (l.count - 1));
    // Spalten für Matrix sammeln
    bool known = false;
    for (uint8_t c = 0; c < numColumns; c++) known |= abs(columns[c] - px[i]) < 4;
    if (!known && numColumns < MAX_COLUMNS) columns[numColumns++] = px[i];
  }
  widthPx = static_cast<int16_t>(layout::width() * 16);
}

void reset() {
  memset(level, 0, sizeof(level));
  memset(letterLevel, 0, sizeof(letterLevel));
  for (Drop& d : drops) d.active = false;
  flash = 0;
  flashRepeats = 0;
  nextFlashAt = vt;
}

void render(uint8_t effect, CRGB* leds, const FxParams& p, uint32_t now) {
  // Speed 0..255 → Faktor 1/8 … 8 (exponentiell, 128 = Echtzeit), in 1/256
  static const uint16_t FACTOR[] = {32, 44, 61, 84, 116, 161, 222, 256, 307, 425, 588, 813, 1124, 1554, 2048};
  const uint8_t idx = p.speed * 14 / 255;
  const uint32_t dt = now - lastNow;
  lastNow = now;
  const uint32_t step = (dt > 200 ? 16 : dt) * FACTOR[idx] / 256;
  vt += step;
  tickAcc += step;

  switch (effect) {
    case 0: solid(leds, p); break;
    case 1: gradient(leds, p); break;
    case 2: rainbowAlong(leds, p, false); break;
    case 3: rainbowAlong(leds, p, true); break;
    case 4: rainbowWheel(leds, p); break;
    case 5: letters(leds, p); break;
    case 6: colorCycle(leds); break;
    case 7: breathe(leds, p); break;
    case 8: scanner(leds, p); break;
    case 9: typewriter(leds, p); break;
    case 10: letterChase(leds, p); break;
    case 11: trace(leds, p); break;
    case 12: theater(leds, p); break;
    case 13: twinkle(leds, p); break;
    case 14: confetti(leds, p); break;
    case 15: fire(leds, p); break;
    case 16: plasma(leds, p); break;
    case 17: ripple(leds, p); break;
    case 18: matrix(leds, p); break;
    case 19: storm(leds, p); break;
    case 20: grassBlock(leds, p); break;
    case 21: enchanted(leds, p); break;
    case 22: redstone(leds, p); break;
    case 23: diamond(leds, p); break;
    case 24: creeper(leds, p); break;
    case 25: netherPortal(leds, p); break;
    case 26: noisePalette(leds, LavaColors_p, p.intensity, 7); break;
    case 27: noisePalette(leds, OceanColors_p, p.intensity, 7); break;
    case 28: aurora(leds, p); break;
    case 29: heartbeat(leds, p); break;
    default: solid(leds, p); break;
  }
}

}  // namespace fx
