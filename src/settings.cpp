#include "settings.h"

#include <ctype.h>
#include <string.h>

namespace {

// Kopiert, wenn der Text passt; gibt true bei Änderung zurück
template <size_t N>
bool setText(char (&field)[N], JsonVariantConst v) {
  if (!v.is<const char*>()) return false;
  const char* text = v.as<const char*>();
  if (strlen(text) >= N || strcmp(field, text) == 0) return false;
  strcpy(field, text);
  return true;
}

const char* powerOnName(PowerOnMode m) {
  switch (m) {
    case PowerOnMode::On: return "on";
    case PowerOnMode::Off: return "off";
    default: return "last";
  }
}

}  // namespace

namespace settings {

bool validHostname(const char* name) {
  const size_t len = strlen(name);
  if (len == 0 || len >= sizeof(Settings::hostname)) return false;
  if (name[0] == '-' || name[len - 1] == '-') return false;
  for (size_t i = 0; i < len; i++) {
    const char c = name[i];
    if (!(islower(static_cast<unsigned char>(c)) || isdigit(static_cast<unsigned char>(c)) || c == '-')) return false;
  }
  return true;
}

uint8_t applyJson(JsonVariantConst cmd, Settings& s) {
  if (!cmd.is<JsonObjectConst>()) return CHANGED_NONE;
  uint8_t changed = CHANGED_NONE;

  if (cmd["hostname"].is<const char*>()) {
    char lower[sizeof(s.hostname)] = "";
    const char* in = cmd["hostname"].as<const char*>();
    if (strlen(in) < sizeof(lower)) {
      for (size_t i = 0; in[i]; i++) lower[i] = static_cast<char>(tolower(static_cast<unsigned char>(in[i])));
      if (validHostname(lower) && strcmp(lower, s.hostname) != 0) {
        strcpy(s.hostname, lower);
        changed |= CHANGED_HOSTNAME;
      }
    }
  }

  if (cmd["maxMilliamps"].is<long>()) {
    long v = cmd["maxMilliamps"].as<long>();
    v = v < MIN_MILLIAMPS ? MIN_MILLIAMPS : (v > MAX_MILLIAMPS ? MAX_MILLIAMPS : v);
    if (v != s.maxMilliamps) {
      s.maxMilliamps = static_cast<uint16_t>(v);
      changed |= CHANGED_LIVE;
    }
  }

  if (cmd["powerOn"].is<const char*>()) {
    const char* m = cmd["powerOn"].as<const char*>();
    PowerOnMode mode = s.powerOn;
    if (strcmp(m, "last") == 0) mode = PowerOnMode::Last;
    else if (strcmp(m, "on") == 0) mode = PowerOnMode::On;
    else if (strcmp(m, "off") == 0) mode = PowerOnMode::Off;
    if (mode != s.powerOn) {
      s.powerOn = mode;
      changed |= CHANGED_LIVE;
    }
  }

  JsonVariantConst wifi = cmd["wifi"];
  if (wifi.is<JsonObjectConst>()) {
    const bool ssidChanged = setText(s.wifiSsid, wifi["ssid"]);
    bool passChanged = false;
    if (wifi["pass"].is<const char*>()) passChanged = setText(s.wifiPass, wifi["pass"]);
    else if (ssidChanged && s.wifiPass[0]) {
      s.wifiPass[0] = '\0';
      passChanged = true;
    }
    if (ssidChanged || passChanged) changed |= CHANGED_WIFI;
  }

  JsonVariantConst mqtt = cmd["mqtt"];
  if (mqtt.is<JsonObjectConst>()) {
    bool c = false;
    if (mqtt["enabled"].is<bool>() && mqtt["enabled"].as<bool>() != s.mqttEnabled) {
      s.mqttEnabled = mqtt["enabled"].as<bool>();
      c = true;
    }
    const bool hostChanged = setText(s.mqttHost, mqtt["host"]);
    c |= hostChanged;
    if (mqtt["port"].is<long>()) {
      const long p = mqtt["port"].as<long>();
      if (p >= 1 && p <= 65535 && p != s.mqttPort) {
        s.mqttPort = static_cast<uint16_t>(p);
        c = true;
      }
    }
    c |= setText(s.mqttUser, mqtt["user"]);
    if (mqtt["pass"].is<const char*>()) c |= setText(s.mqttPass, mqtt["pass"]);
    else if (hostChanged && s.mqttPass[0]) {
      s.mqttPass[0] = '\0';
      c = true;
    }
    if (mqtt["topic"].is<const char*>() && mqtt["topic"].as<const char*>()[0]) c |= setText(s.mqttTopic, mqtt["topic"]);
    if (mqtt["discovery"].is<bool>() && mqtt["discovery"].as<bool>() != s.mqttDiscovery) {
      s.mqttDiscovery = mqtt["discovery"].as<bool>();
      c = true;
    }
    if (c) changed |= CHANGED_MQTT;
  }

  return changed;
}

void toJson(const Settings& s, JsonObject out) {
  out["hostname"] = s.hostname;
  out["maxMilliamps"] = s.maxMilliamps;
  out["powerOn"] = powerOnName(s.powerOn);
  JsonObject wifi = out["wifi"].to<JsonObject>();
  wifi["ssid"] = s.wifiSsid;
  wifi["hasPass"] = s.wifiPass[0] != '\0';
  JsonObject mqtt = out["mqtt"].to<JsonObject>();
  mqtt["enabled"] = s.mqttEnabled;
  mqtt["host"] = s.mqttHost;
  mqtt["port"] = s.mqttPort;
  mqtt["user"] = s.mqttUser;
  mqtt["hasPass"] = s.mqttPass[0] != '\0';
  mqtt["topic"] = s.mqttTopic;
  mqtt["discovery"] = s.mqttDiscovery;
}

}  // namespace settings
