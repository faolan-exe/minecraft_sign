#include <string.h>
#include <unity.h>

#include "layout.h"
#include "power.h"

void setUp() {}
void tearDown() {}

namespace {

uint8_t frame[NUM_LEDS * 3];
const PowerModel MODEL{5000, 100};

void fill(uint8_t r, uint8_t g, uint8_t b) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    frame[i * 3] = r;
    frame[i * 3 + 1] = g;
    frame[i * 3 + 2] = b;
  }
}

}  // namespace

void test_black_is_base_load() {
  fill(0, 0, 0);
  TEST_ASSERT_EQUAL_UINT32(100, power::estimateMilliamps(frame, NUM_LEDS, 255, MODEL));
}

void test_full_white_uses_correction() {
  fill(255, 255, 255);
  // je LED (255 + 176 + 240) / 255 Kanäle à 5 mA
  const uint32_t expected = 100 + NUM_LEDS * 5000ULL * (255 + 176 + 240) / 255 / 1000;
  TEST_ASSERT_UINT32_WITHIN(1, expected, power::estimateMilliamps(frame, NUM_LEDS, 255, MODEL));
}

void test_brightness_scales_led_part_only() {
  fill(255, 0, 0);
  const uint32_t full = power::estimateMilliamps(frame, NUM_LEDS, 255, MODEL);
  const uint32_t half = power::estimateMilliamps(frame, NUM_LEDS, 128, MODEL);
  TEST_ASSERT_UINT32_WITHIN(2, 100 + (full - 100) * 128 / 255, half);
}

void test_limit_keeps_brightness_below_limit() {
  fill(255, 0, 0);
  TEST_ASSERT_EQUAL_UINT8(200, power::limitBrightness(frame, NUM_LEDS, 200, 1800, MODEL));
}

void test_limit_reduces_to_limit() {
  fill(255, 255, 255);
  const uint8_t b = power::limitBrightness(frame, NUM_LEDS, 255, 1000, MODEL);
  TEST_ASSERT_TRUE(b < 255);
  TEST_ASSERT_TRUE(power::estimateMilliamps(frame, NUM_LEDS, b, MODEL) <= 1000);
  TEST_ASSERT_TRUE(power::estimateMilliamps(frame, NUM_LEDS, b + 2, MODEL) > 1000);
}

void test_limit_edge_cases() {
  fill(255, 255, 255);
  TEST_ASSERT_EQUAL_UINT8(255, power::limitBrightness(frame, NUM_LEDS, 255, 0, MODEL));  // 0 = ohne Limit
  TEST_ASSERT_EQUAL_UINT8(1, power::limitBrightness(frame, NUM_LEDS, 255, 100, MODEL));  // Limit ≤ Grundlast
  TEST_ASSERT_EQUAL_UINT8(0, power::limitBrightness(frame, NUM_LEDS, 0, 1000, MODEL));
}

void test_calibration_roundtrip() {
  PowerModel m{};
  TEST_ASSERT_TRUE(power::calibrate(80, 900, NUM_LEDS, m));
  TEST_ASSERT_EQUAL_UINT16(80, m.baseMilliamps);
  fill(CALIBRATION_LEVEL, CALIBRATION_LEVEL, CALIBRATION_LEVEL);
  TEST_ASSERT_UINT32_WITHIN(2, 900, power::estimateMilliamps(frame, NUM_LEDS, 255, m));
}

void test_calibration_rejects_implausible_values() {
  PowerModel m{1234, 56};
  TEST_ASSERT_FALSE(power::calibrate(500, 500, NUM_LEDS, m));   // weiß = aus
  TEST_ASSERT_FALSE(power::calibrate(500, 400, NUM_LEDS, m));   // weiß < aus
  TEST_ASSERT_FALSE(power::calibrate(100, 110, NUM_LEDS, m));   // zu wenig Strom je Kanal
  TEST_ASSERT_FALSE(power::calibrate(100, 60000, NUM_LEDS, m)); // zu viel
  TEST_ASSERT_FALSE(power::calibrate(1500, 3000, NUM_LEDS, m)); // Grundlast zu hoch
  TEST_ASSERT_FALSE(power::calibrate(100, 900, 0, m));
  TEST_ASSERT_EQUAL_UINT16(1234, m.channelMicroamps);
  TEST_ASSERT_EQUAL_UINT16(56, m.baseMilliamps);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_black_is_base_load);
  RUN_TEST(test_full_white_uses_correction);
  RUN_TEST(test_brightness_scales_led_part_only);
  RUN_TEST(test_limit_keeps_brightness_below_limit);
  RUN_TEST(test_limit_reduces_to_limit);
  RUN_TEST(test_limit_edge_cases);
  RUN_TEST(test_calibration_roundtrip);
  RUN_TEST(test_calibration_rejects_implausible_values);
  return UNITY_END();
}
