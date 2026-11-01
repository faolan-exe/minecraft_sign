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

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_letters_match_layout_txt);
  RUN_TEST(test_every_led_belongs_to_its_letter);
  RUN_TEST(test_letters_are_left_to_right_without_overlap);
  RUN_TEST(test_coordinates_inside_bounds);
  RUN_TEST(test_no_two_leds_share_a_position);
  RUN_TEST(test_first_led_positions);
  RUN_TEST(test_normalised_extremes);
  return UNITY_END();
}
