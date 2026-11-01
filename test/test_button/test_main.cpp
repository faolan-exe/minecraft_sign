#include <unity.h>

#include "button.h"

void setUp() {}
void tearDown() {}

namespace {

// Hält den Pegel von t bis t+duration und sammelt das erste Ereignis
ButtonEvent hold(Button& b, bool pressed, uint32_t& t, uint32_t duration) {
  ButtonEvent first = ButtonEvent::None;
  for (uint32_t elapsed = 0; elapsed < duration; elapsed += 5, t += 5) {
    ButtonEvent ev = b.update(pressed, t);
    if (ev != ButtonEvent::None && first == ButtonEvent::None) first = ev;
  }
  return first;
}

}  // namespace

void test_short_press_is_click_after_gap() {
  Button b;
  uint32_t t = 0;
  TEST_ASSERT_EQUAL(ButtonEvent::None, hold(b, true, t, 100));
  // direkt nach dem Loslassen noch kein Klick (könnte ein Doppelklick werden)
  TEST_ASSERT_EQUAL(ButtonEvent::None, hold(b, false, t, Button::DEBOUNCE_MS + Button::DOUBLE_GAP_MS - 10));
  TEST_ASSERT_EQUAL(ButtonEvent::Click, hold(b, false, t, 100));
}

void test_bounce_is_ignored() {
  Button b;
  uint32_t t = 0;
  // Prellen: kürzer als die Entprellzeit
  for (int i = 0; i < 4; i++) {
    TEST_ASSERT_EQUAL(ButtonEvent::None, b.update(true, t));
    t += 5;
    TEST_ASSERT_EQUAL(ButtonEvent::None, b.update(false, t));
    t += 5;
  }
  TEST_ASSERT_EQUAL(ButtonEvent::None, hold(b, false, t, 1000));
}

void test_double_click() {
  Button b;
  uint32_t t = 0;
  hold(b, true, t, 80);
  hold(b, false, t, 120);
  hold(b, true, t, 80);
  TEST_ASSERT_EQUAL(ButtonEvent::DoubleClick, hold(b, false, t, 60));
  // kein zusätzlicher Einzelklick hinterher
  TEST_ASSERT_EQUAL(ButtonEvent::None, hold(b, false, t, 1000));
}

void test_long_press_fires_while_held_and_only_once() {
  Button b;
  uint32_t t = 0;
  TEST_ASSERT_EQUAL(ButtonEvent::None, hold(b, true, t, Button::DEBOUNCE_MS + Button::LONG_MS - 10));
  TEST_ASSERT_EQUAL(ButtonEvent::LongPress, hold(b, true, t, 50));
  TEST_ASSERT_EQUAL(ButtonEvent::None, hold(b, true, t, 3000));
  // Loslassen nach langem Druck ist kein Klick
  TEST_ASSERT_EQUAL(ButtonEvent::None, hold(b, false, t, 1000));
}

void test_click_then_long_press_is_long_press_only() {
  Button b;
  uint32_t t = 0;
  hold(b, true, t, 80);
  hold(b, false, t, 100);
  TEST_ASSERT_EQUAL(ButtonEvent::LongPress, hold(b, true, t, 1000));
  TEST_ASSERT_EQUAL(ButtonEvent::None, hold(b, false, t, 1000));
}

void test_millis_overflow() {
  Button b;
  uint32_t t = 0xFFFFFFFFu - 200;
  hold(b, true, t, 100);
  TEST_ASSERT_EQUAL(ButtonEvent::Click, hold(b, false, t, 500));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_short_press_is_click_after_gap);
  RUN_TEST(test_bounce_is_ignored);
  RUN_TEST(test_double_click);
  RUN_TEST(test_long_press_fires_while_held_and_only_once);
  RUN_TEST(test_click_then_long_press_is_long_press_only);
  RUN_TEST(test_millis_overflow);
  return UNITY_END();
}
