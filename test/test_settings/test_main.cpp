#include <string.h>

#include <string>
#include <unity.h>

#include "settings.h"

void setUp() {}
void tearDown() {}

namespace {

uint8_t apply(Settings& s, const char* json) {
  JsonDocument doc;
  deserializeJson(doc, json);
  return settings::applyJson(doc.as<JsonVariantConst>(), s);
}

}  // namespace

void test_defaults_have_no_credentials() {
  Settings s;
  TEST_ASSERT_EQUAL_STRING("", s.wifiSsid);
  TEST_ASSERT_EQUAL_STRING("", s.wifiPass);
  TEST_ASSERT_FALSE(s.mqttEnabled);
}

void test_wifi_set_with_password() {
  Settings s;
  TEST_ASSERT_EQUAL_UINT8(CHANGED_WIFI, apply(s, R"({"wifi":{"ssid":"Heim","pass":"geheim123"}})"));
  TEST_ASSERT_EQUAL_STRING("Heim", s.wifiSsid);
  TEST_ASSERT_EQUAL_STRING("geheim123", s.wifiPass);
}

void test_wifi_password_kept_when_not_sent() {
  Settings s;
  apply(s, R"({"wifi":{"ssid":"Heim","pass":"geheim123"}})");
  TEST_ASSERT_EQUAL_UINT8(CHANGED_NONE, apply(s, R"({"wifi":{"ssid":"Heim"}})"));
  TEST_ASSERT_EQUAL_STRING("geheim123", s.wifiPass);
}

void test_wifi_password_cleared_when_ssid_changes_without_password() {
  Settings s;
  apply(s, R"({"wifi":{"ssid":"Heim","pass":"geheim123"}})");
  TEST_ASSERT_EQUAL_UINT8(CHANGED_WIFI, apply(s, R"({"wifi":{"ssid":"Gast"}})"));
  TEST_ASSERT_EQUAL_STRING("Gast", s.wifiSsid);
  TEST_ASSERT_EQUAL_STRING("", s.wifiPass);
}

void test_wifi_too_long_values_are_ignored() {
  Settings s;
  // 33 Zeichen SSID, 65 Zeichen Passwort
  apply(s, R"({"wifi":{"ssid":"123456789012345678901234567890123","pass":"x"}})");
  TEST_ASSERT_EQUAL_STRING("", s.wifiSsid);
  apply(s, R"({"wifi":{"ssid":"12345678901234567890123456789012"}})");
  TEST_ASSERT_EQUAL_UINT(32, strlen(s.wifiSsid));
  char json[128];
  snprintf(json, sizeof(json), R"({"wifi":{"pass":"%s"}})", "12345678901234567890123456789012345678901234567890123456789012345");
  apply(s, json);
  TEST_ASSERT_EQUAL_STRING("", s.wifiPass);
}

void test_wifi_empty_ssid_clears_network() {
  Settings s;
  apply(s, R"({"wifi":{"ssid":"Heim","pass":"geheim123"}})");
  TEST_ASSERT_EQUAL_UINT8(CHANGED_WIFI, apply(s, R"({"wifi":{"ssid":""}})"));
  TEST_ASSERT_EQUAL_STRING("", s.wifiSsid);
  TEST_ASSERT_EQUAL_STRING("", s.wifiPass);
}

void test_hostname_rules() {
  Settings s;
  TEST_ASSERT_EQUAL_UINT8(CHANGED_HOSTNAME, apply(s, R"({"hostname":"Schild-1"})"));
  TEST_ASSERT_EQUAL_STRING("schild-1", s.hostname);
  apply(s, R"({"hostname":"-falsch"})");
  apply(s, R"({"hostname":"falsch-"})");
  apply(s, R"({"hostname":"mit leer"})");
  apply(s, R"({"hostname":"umläut"})");
  apply(s, R"({"hostname":""})");
  apply(s, R"({"hostname":"abcdefghijabcdefghijabcdefghijab"})");  // 32 Zeichen
  TEST_ASSERT_EQUAL_STRING("schild-1", s.hostname);
  TEST_ASSERT_TRUE(settings::validHostname("abcdefghijabcdefghijabcdefghija"));  // 31 Zeichen
}

void test_milliamps_clamped() {
  Settings s;
  apply(s, R"({"maxMilliamps":50})");
  TEST_ASSERT_EQUAL_UINT16(MIN_MILLIAMPS, s.maxMilliamps);
  apply(s, R"({"maxMilliamps":99999})");
  TEST_ASSERT_EQUAL_UINT16(MAX_MILLIAMPS, s.maxMilliamps);
  TEST_ASSERT_EQUAL_UINT8(CHANGED_NONE, apply(s, R"({"maxMilliamps":5000})"));
}

void test_power_on_mode() {
  Settings s;
  TEST_ASSERT_EQUAL_UINT8(CHANGED_LIVE, apply(s, R"({"powerOn":"off"})"));
  TEST_ASSERT_TRUE(s.powerOn == PowerOnMode::Off);
  TEST_ASSERT_EQUAL_UINT8(CHANGED_NONE, apply(s, R"({"powerOn":"quatsch"})"));
  TEST_ASSERT_TRUE(s.powerOn == PowerOnMode::Off);
}

void test_mqtt_settings() {
  Settings s;
  TEST_ASSERT_EQUAL_UINT8(CHANGED_MQTT,
                          apply(s, R"({"mqtt":{"enabled":true,"host":"broker","port":1884,"user":"u","pass":"p"}})"));
  TEST_ASSERT_TRUE(s.mqttEnabled);
  TEST_ASSERT_EQUAL_UINT16(1884, s.mqttPort);
  TEST_ASSERT_EQUAL_STRING("p", s.mqttPass);
  // ungültiger Port und leeres Topic werden ignoriert
  TEST_ASSERT_EQUAL_UINT8(CHANGED_NONE, apply(s, R"({"mqtt":{"port":0,"topic":""}})"));
  TEST_ASSERT_EQUAL_UINT8(CHANGED_NONE, apply(s, R"({"mqtt":{"port":70000}})"));
  TEST_ASSERT_EQUAL_STRING("minecraft_sign", s.mqttTopic);
  // neuer Server ohne Passwort: altes Passwort weg
  apply(s, R"({"mqtt":{"host":"anderer"}})");
  TEST_ASSERT_EQUAL_STRING("", s.mqttPass);
}

void test_json_output_hides_passwords() {
  Settings s;
  apply(s, R"({"wifi":{"ssid":"Heim","pass":"geheim123"},"mqtt":{"host":"b","pass":"p"}})");
  JsonDocument doc;
  settings::toJson(s, doc.to<JsonObject>());
  std::string out;
  serializeJson(doc, out);
  TEST_ASSERT_NULL(strstr(out.c_str(), "geheim123"));
  TEST_ASSERT_TRUE(doc["wifi"]["hasPass"].as<bool>());
  TEST_ASSERT_TRUE(doc["mqtt"]["hasPass"].as<bool>());
}

void test_power_model_values_clamped() {
  Settings s;
  TEST_ASSERT_EQUAL_UINT16(5000, s.power.channelMicroamps);
  apply(s, R"({"channelMicroamps":10,"baseMilliamps":-5})");
  TEST_ASSERT_EQUAL_UINT16(MIN_CHANNEL_MICROAMPS, s.power.channelMicroamps);
  TEST_ASSERT_EQUAL_UINT16(0, s.power.baseMilliamps);
  apply(s, R"({"channelMicroamps":99999,"baseMilliamps":5000})");
  TEST_ASSERT_EQUAL_UINT16(MAX_CHANNEL_MICROAMPS, s.power.channelMicroamps);
  TEST_ASSERT_EQUAL_UINT16(MAX_BASE_MILLIAMPS, s.power.baseMilliamps);
}

void test_power_calibration() {
  Settings s;
  TEST_ASSERT_EQUAL_UINT8(CHANGED_LIVE, apply(s, R"({"calibration":{"offMa":70,"whiteMa":1000}})"));
  TEST_ASSERT_EQUAL_UINT16(70, s.power.baseMilliamps);
  const uint16_t ch = s.power.channelMicroamps;
  // unplausibel: bleibt unverändert
  TEST_ASSERT_EQUAL_UINT8(CHANGED_NONE, apply(s, R"({"calibration":{"offMa":900,"whiteMa":100}})"));
  TEST_ASSERT_EQUAL_UINT8(CHANGED_NONE, apply(s, R"({"calibration":{"offMa":-1,"whiteMa":100}})"));
  TEST_ASSERT_EQUAL_UINT8(CHANGED_NONE, apply(s, R"({"calibration":{"offMa":70}})"));
  TEST_ASSERT_EQUAL_UINT16(ch, s.power.channelMicroamps);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_defaults_have_no_credentials);
  RUN_TEST(test_wifi_set_with_password);
  RUN_TEST(test_wifi_password_kept_when_not_sent);
  RUN_TEST(test_wifi_password_cleared_when_ssid_changes_without_password);
  RUN_TEST(test_wifi_too_long_values_are_ignored);
  RUN_TEST(test_wifi_empty_ssid_clears_network);
  RUN_TEST(test_hostname_rules);
  RUN_TEST(test_milliamps_clamped);
  RUN_TEST(test_power_on_mode);
  RUN_TEST(test_mqtt_settings);
  RUN_TEST(test_json_output_hides_passwords);
  RUN_TEST(test_power_model_values_clamped);
  RUN_TEST(test_power_calibration);
  return UNITY_END();
}
