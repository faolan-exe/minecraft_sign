#include <unity.h>

#include "effect_list.h"
#include "light_state.h"

void setUp() {}
void tearDown() {}

namespace {

bool apply(LightState& s, const char* json) {
  JsonDocument doc;
  deserializeJson(doc, json);
  return light::applyJson(doc.as<JsonVariantConst>(), s);
}

}  // namespace

// --- Taster ---

void test_click_when_off_turns_on_and_keeps_effect() {
  LightState s;
  s.on = false;
  s.effect = 7;
  TEST_ASSERT_FALSE(light::handleButton(ButtonEvent::Click, s));
  TEST_ASSERT_TRUE(s.on);
  TEST_ASSERT_EQUAL_UINT8(7, s.effect);
}

void test_double_click_when_off_turns_on_without_brightness_change() {
  LightState s;
  s.on = false;
  s.brightness = 80;
  light::handleButton(ButtonEvent::DoubleClick, s);
  TEST_ASSERT_TRUE(s.on);
  TEST_ASSERT_EQUAL_UINT8(80, s.brightness);
}

void test_long_press_when_off_does_nothing() {
  LightState s;
  s.on = false;
  TEST_ASSERT_FALSE(light::handleButton(ButtonEvent::LongPress, s));
  TEST_ASSERT_FALSE(s.on);
}

void test_click_when_on_switches_effect() {
  LightState s;
  s.effect = 0;
  light::handleButton(ButtonEvent::Click, s);
  TEST_ASSERT_EQUAL_UINT8(1, s.effect);
}

void test_long_press_when_on_turns_off_and_requests_save() {
  LightState s;
  s.effect = 5;
  TEST_ASSERT_TRUE(light::handleButton(ButtonEvent::LongPress, s));
  TEST_ASSERT_FALSE(s.on);
  TEST_ASSERT_EQUAL_UINT8(5, s.effect);
}

void test_double_click_steps_brightness() {
  LightState s;
  s.brightness = 100;
  light::handleButton(ButtonEvent::DoubleClick, s);
  TEST_ASSERT_EQUAL_UINT8(160, s.brightness);
  light::handleButton(ButtonEvent::DoubleClick, s);
  TEST_ASSERT_EQUAL_UINT8(255, s.brightness);
  light::handleButton(ButtonEvent::DoubleClick, s);
  TEST_ASSERT_EQUAL_UINT8(32, s.brightness);
}

void test_brightness_step_boundaries() {
  TEST_ASSERT_EQUAL_UINT8(32, light::nextBrightnessStep(0));
  TEST_ASSERT_EQUAL_UINT8(80, light::nextBrightnessStep(32));
  TEST_ASSERT_EQUAL_UINT8(32, light::nextBrightnessStep(255));
}

// --- Durchschalten ---

void test_next_effect_wraps_around() {
  LightState s;
  s.effect = effects::count() - 1;
  TEST_ASSERT_EQUAL_UINT8(0, light::nextEffect(s));
}

void test_next_effect_respects_cycle_mask() {
  LightState s;
  s.cycleMask = (1u << 3) | (1u << 10);
  s.effect = 3;
  TEST_ASSERT_EQUAL_UINT8(10, light::nextEffect(s));
  s.effect = 10;
  TEST_ASSERT_EQUAL_UINT8(3, light::nextEffect(s));
  // aktueller Effekt nicht in der Liste: nächster aus der Liste
  s.effect = 5;
  TEST_ASSERT_EQUAL_UINT8(10, light::nextEffect(s));
}

void test_next_effect_single_entry_stays() {
  LightState s;
  s.cycleMask = 1u << 4;
  s.effect = 4;
  TEST_ASSERT_EQUAL_UINT8(4, light::nextEffect(s));
}

void test_next_effect_mask_with_only_invalid_bits_uses_all() {
  LightState s;
  s.cycleMask = 1u << 31;
  s.effect = 0;
  TEST_ASSERT_EQUAL_UINT8(1, light::nextEffect(s));
}

// --- Einschaltverhalten ---

void test_power_on_modes() {
  LightState s;
  s.on = false;
  light::applyPowerOn(PowerOnMode::Last, s);
  TEST_ASSERT_FALSE(s.on);
  light::applyPowerOn(PowerOnMode::On, s);
  TEST_ASSERT_TRUE(s.on);
  light::applyPowerOn(PowerOnMode::Off, s);
  TEST_ASSERT_FALSE(s.on);
}

// --- JSON ---

void test_json_effect_by_id_name_and_index() {
  LightState s;
  apply(s, R"({"effect":"fire"})");
  TEST_ASSERT_EQUAL_STRING("fire", effects::info(s.effect).id);
  apply(s, R"({"effect":"Regenbogen-Rad"})");
  TEST_ASSERT_EQUAL_STRING("rainbow_wheel", effects::info(s.effect).id);
  apply(s, R"({"effect":0})");
  TEST_ASSERT_EQUAL_UINT8(0, s.effect);
}

void test_json_unknown_effect_is_ignored() {
  LightState s;
  s.effect = 4;
  TEST_ASSERT_FALSE(apply(s, R"({"effect":"gibtsnicht"})"));
  TEST_ASSERT_FALSE(apply(s, R"({"effect":99})"));
  TEST_ASSERT_FALSE(apply(s, R"({"effect":-1})"));
  TEST_ASSERT_EQUAL_UINT8(4, s.effect);
}

void test_json_values_are_clamped() {
  LightState s;
  apply(s, R"({"brightness":999,"speed":-5,"intensity":300})");
  TEST_ASSERT_EQUAL_UINT8(255, s.brightness);
  TEST_ASSERT_EQUAL_UINT8(0, s.speed);
  TEST_ASSERT_EQUAL_UINT8(255, s.intensity);
  apply(s, R"({"brightness":0})");
  TEST_ASSERT_EQUAL_UINT8(MIN_BRIGHTNESS, s.brightness);
  apply(s, R"({"autoCycle":99999})");
  TEST_ASSERT_EQUAL_UINT16(3600, s.autoCycleSec);
}

void test_json_home_assistant_format() {
  LightState s;
  s.on = false;
  apply(s, R"({"state":"ON","color":{"r":255,"g":0,"b":16},"brightness":40})");
  TEST_ASSERT_TRUE(s.on);
  TEST_ASSERT_EQUAL_HEX32(0xFF0010, s.color1);
  TEST_ASSERT_EQUAL_UINT8(40, s.brightness);
  apply(s, R"({"state":"TOGGLE"})");
  TEST_ASSERT_FALSE(s.on);
}

void test_json_colors_and_invalid_colors() {
  LightState s;
  apply(s, R"({"color1":"#00ff80","color2":"123ABC"})");
  TEST_ASSERT_EQUAL_HEX32(0x00FF80, s.color1);
  TEST_ASSERT_EQUAL_HEX32(0x123ABC, s.color2);
  TEST_ASSERT_FALSE(apply(s, R"({"color1":"#12345","color2":"#gg0000"})"));
}

void test_json_cycle_list_and_change_detection() {
  LightState s;
  TEST_ASSERT_TRUE(apply(s, R"({"cycle":["solid",5,"unbekannt"]})"));
  TEST_ASSERT_EQUAL_HEX32((1u << 0) | (1u << 5), s.cycleMask);
  TEST_ASSERT_FALSE(apply(s, R"({"cycle":["solid",5]})"));
  TEST_ASSERT_FALSE(apply(s, "[1,2]"));
}

void test_json_roundtrip() {
  LightState a;
  a.on = false;
  a.effect = 12;
  a.color1 = 0xABCDEF;
  a.cycleMask = 0b1011;
  a.autoCycleSec = 30;
  JsonDocument doc;
  light::toJson(a, doc.to<JsonObject>());
  LightState b;
  apply(b, "{}");
  light::applyJson(doc.as<JsonVariantConst>(), b);
  TEST_ASSERT_TRUE(a == b);
}

void test_effect_ids_are_unique() {
  for (uint8_t i = 0; i < effects::count(); i++) {
    TEST_ASSERT_EQUAL_INT(i, effects::find(effects::info(i).id));
    TEST_ASSERT_EQUAL_INT(i, effects::find(effects::info(i).name));
  }
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_click_when_off_turns_on_and_keeps_effect);
  RUN_TEST(test_double_click_when_off_turns_on_without_brightness_change);
  RUN_TEST(test_long_press_when_off_does_nothing);
  RUN_TEST(test_click_when_on_switches_effect);
  RUN_TEST(test_long_press_when_on_turns_off_and_requests_save);
  RUN_TEST(test_double_click_steps_brightness);
  RUN_TEST(test_brightness_step_boundaries);
  RUN_TEST(test_next_effect_wraps_around);
  RUN_TEST(test_next_effect_respects_cycle_mask);
  RUN_TEST(test_next_effect_single_entry_stays);
  RUN_TEST(test_next_effect_mask_with_only_invalid_bits_uses_all);
  RUN_TEST(test_power_on_modes);
  RUN_TEST(test_json_effect_by_id_name_and_index);
  RUN_TEST(test_json_unknown_effect_is_ignored);
  RUN_TEST(test_json_values_are_clamped);
  RUN_TEST(test_json_home_assistant_format);
  RUN_TEST(test_json_colors_and_invalid_colors);
  RUN_TEST(test_json_cycle_list_and_change_detection);
  RUN_TEST(test_json_roundtrip);
  RUN_TEST(test_effect_ids_are_unique);
  return UNITY_END();
}
