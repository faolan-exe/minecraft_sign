#include <unity.h>

#include "layout.h"

void setUp() { layout::build(); }
void tearDown() {}

// Erwartung direkt aus layout.txt: Buchstabe und LED-Anzahl in Kettenreihenfolge
void test_letters_match_layout_txt() {
  const char glyphs[] = "MINECRAFT";
  const uint16_t counts[] = {26, 10, 24, 22, 18, 24, 26, 18, 14};
  uint16_t first = 0;
  for (uint8_t l = 0; l < NUM_LETTERS; l++) {
    TEST_ASSERT_EQUAL_CHAR(glyphs[l], layout::letter(l).glyph);
    TEST_ASSERT_EQUAL_UINT16(first, layout::letter(l).first);
    TEST_ASSERT_EQUAL_UINT16(counts[l], layout::letter(l).count);
    first += counts[l];
  }
  TEST_ASSERT_EQUAL_UINT16(NUM_LEDS, first);
}

void test_every_led_belongs_to_its_letter() {
  for (uint8_t l = 0; l < NUM_LETTERS; l++) {
    const LetterInfo& info = layout::letter(l);
    for (uint16_t i = info.first; i < info.first + info.count; i++) {
      TEST_ASSERT_EQUAL_UINT8(l, layout::pos(i).letter);
      TEST_ASSERT_TRUE(layout::pos(i).x >= info.x0 - 0.01f);
      TEST_ASSERT_TRUE(layout::pos(i).x <= info.x1 + 0.01f);
    }
  }
}

void test_letters_are_left_to_right_without_overlap() {
  for (uint8_t l = 1; l < NUM_LETTERS; l++) {
    TEST_ASSERT_TRUE(layout::letter(l).x0 > layout::letter(l - 1).x1);
  }
}

void test_coordinates_inside_bounds() {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    TEST_ASSERT_TRUE(layout::pos(i).y >= 0 && layout::pos(i).y <= LAYOUT_HEIGHT);
    TEST_ASSERT_TRUE(layout::pos(i).x >= 0 && layout::pos(i).x <= layout::width());
  }
}

void test_no_two_leds_share_a_position() {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    for (uint16_t j = i + 1; j < NUM_LEDS; j++) {
      const float dx = layout::pos(i).x - layout::pos(j).x;
      const float dy = layout::pos(i).y - layout::pos(j).y;
      TEST_ASSERT_TRUE_MESSAGE(dx * dx + dy * dy > 0.25f, "zwei LEDs liegen aufeinander");
    }
  }
}

// Stichproben der Kettenreihenfolge: LED 1 (Index 0) jedes Buchstabens
void test_first_led_positions() {
  // M: LED 1 unten rechts
  TEST_ASSERT_EQUAL_FLOAT(10.0f, layout::pos(0).x);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, layout::pos(0).y);
  // E: LED 1 unten links
  const LetterInfo& e = layout::letter(3);
  TEST_ASSERT_EQUAL_FLOAT(e.x0, layout::pos(e.first).x);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, layout::pos(e.first).y);
  // A: LED 15 (Index 16) oben links, letzte LED unten links
  const LetterInfo& a = layout::letter(6);
  TEST_ASSERT_EQUAL_FLOAT(a.x0, layout::pos(a.first + 16).x);
  TEST_ASSERT_EQUAL_FLOAT(9.0f, layout::pos(a.first + 16).y);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, layout::pos(a.first + a.count - 1).y);
  // T: LED 10 ist die Mitte des Querbalkens
  const LetterInfo& t = layout::letter(8);
  TEST_ASSERT_EQUAL_FLOAT(9.0f, layout::pos(t.first + 9).y);
  TEST_ASSERT_EQUAL_FLOAT(t.x0 + 2, layout::pos(t.first + 9).x);
}

void test_normalised_extremes() {
  uint8_t minX = 255, maxX = 0, minY = 255, maxY = 0;
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    if (layout::nx(i) < minX) minX = layout::nx(i);
    if (layout::nx(i) > maxX) maxX = layout::nx(i);
    if (layout::ny(i) < minY) minY = layout::ny(i);
    if (layout::ny(i) > maxY) maxY = layout::ny(i);
  }
  TEST_ASSERT_EQUAL_UINT8(0, minX);
  TEST_ASSERT_EQUAL_UINT8(255, maxX);
  TEST_ASSERT_EQUAL_UINT8(0, minY);
  TEST_ASSERT_EQUAL_UINT8(255, maxY);
}

// Schreibreihenfolge: Permutation je Buchstabe, Buchstaben nacheinander
void test_pen_order_is_permutation_per_letter() {
  bool seen[NUM_LEDS] = {};
  uint16_t k = 0;
  for (uint8_t l = 0; l < NUM_LETTERS; l++) {
    const LetterInfo& info = layout::letter(l);
    for (uint16_t r = 0; r < info.count; r++, k++) {
      const uint16_t i = layout::penOrder(k);
      TEST_ASSERT_EQUAL_UINT8(l, layout::pos(i).letter);
      TEST_ASSERT_EQUAL_UINT8(r, layout::penRank(i));
      TEST_ASSERT_FALSE_MESSAGE(seen[i], "LED doppelt in der Schreibreihenfolge");
      seen[i] = true;
    }
  }
  TEST_ASSERT_EQUAL_UINT16(NUM_LEDS, k);
}

// Jeder Buchstabe beginnt am linken Rand und endet rechts der Mitte oder in der Mitte
void test_pen_order_goes_left_to_right() {
  for (uint8_t l = 0; l < NUM_LETTERS; l++) {
    const LetterInfo& info = layout::letter(l);
    uint16_t first = 0, last = 0;
    for (uint16_t i = info.first; i < info.first + info.count; i++) {
      if (layout::penRank(i) == 0) first = i;
      if (layout::penRank(i) == info.count - 1) last = i;
    }
    TEST_ASSERT_EQUAL_FLOAT(info.x0, layout::pos(first).x);
    TEST_ASSERT_TRUE_MESSAGE(layout::pos(last).x >= (info.x0 + info.x1) / 2, "Schreibreihenfolge endet links");
  }
}

// Stichproben aus layout.txt
void test_pen_order_samples() {
  // M: erst LED 26 (links unten), zuletzt LED 1 (rechts unten)
  TEST_ASSERT_EQUAL_UINT16(25, layout::penOrder(0));
  TEST_ASSERT_EQUAL_UINT16(0, layout::penOrder(25));
  // T: erst LED 14 (Balken links), dann 13, 10, 11, 12, dann Stamm von oben
  const LetterInfo& t = layout::letter(8);
  const uint16_t tStart = NUM_LEDS - t.count;
  TEST_ASSERT_EQUAL_UINT16(t.first + 13, layout::penOrder(tStart));
  TEST_ASSERT_EQUAL_UINT16(t.first + 9, layout::penOrder(tStart + 2));
  TEST_ASSERT_EQUAL_UINT16(t.first + 8, layout::penOrder(tStart + 5));
  // E: Kettenreihenfolge
  const LetterInfo& e = layout::letter(3);
  TEST_ASSERT_EQUAL_UINT8(0, layout::penRank(e.first));
  TEST_ASSERT_EQUAL_UINT8(21, layout::penRank(e.first + 21));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_letters_match_layout_txt);
  RUN_TEST(test_every_led_belongs_to_its_letter);
  RUN_TEST(test_letters_are_left_to_right_without_overlap);
  RUN_TEST(test_coordinates_inside_bounds);
  RUN_TEST(test_no_two_leds_share_a_position);
  RUN_TEST(test_first_led_positions);
  RUN_TEST(test_normalised_extremes);
  RUN_TEST(test_pen_order_is_permutation_per_letter);
  RUN_TEST(test_pen_order_goes_left_to_right);
  RUN_TEST(test_pen_order_samples);
  return UNITY_END();
}
