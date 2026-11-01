#pragma once

#include <ArduinoJson.h>
#include <stdint.h>

#include "light_state.h"
#include "power.h"

// Geräte-Einstellungen. Werden nur über die Weboberfläche gesetzt und im Flash
// gespeichert; nichts davon steht im Code.

struct Settings {
  char hostname[32] = "minecraft-sign";
  uint16_t maxMilliamps = 1800;
  // Voreinstellung für 2020-WS2812: 5 mA je Kanal (Datenblatt WS2812C-2020, passt zur
  // Messung ≤ 1 A bei vollem Regenbogen); Grundlast ist eine Annahme bis zur Kalibrierung
  PowerModel power{5000, 100};
  PowerOnMode powerOn = PowerOnMode::Last;

  char wifiSsid[33] = "";
  char wifiPass[65] = "";

  bool mqttEnabled = false;
  char mqttHost[64] = "";
  uint16_t mqttPort = 1883;
  char mqttUser[64] = "";
  char mqttPass[64] = "";
  char mqttTopic[64] = "minecraft_sign";
  bool mqttDiscovery = true;
};

constexpr uint16_t MIN_MILLIAMPS = 200;
constexpr uint16_t MAX_MILLIAMPS = 5000;
constexpr uint16_t MIN_CHANNEL_MICROAMPS = 500;
constexpr uint16_t MAX_CHANNEL_MICROAMPS = 30000;
constexpr uint16_t MAX_BASE_MILLIAMPS = 1000;

// Was sich geändert hat, damit der Aufrufer weiß, ob neu verbunden werden muss
enum SettingsChange : uint8_t {
  CHANGED_NONE = 0,
  CHANGED_LIVE = 1 << 0,  // sofort wirksam (Strom, Einschaltverhalten)
  CHANGED_WIFI = 1 << 1,  // Neustart nötig
  CHANGED_MQTT = 1 << 2,  // MQTT neu verbinden
  CHANGED_HOSTNAME = 1 << 3,
};

namespace settings {

// Regeln:
// - Passwörter werden nur übernommen, wenn das Feld mitgeschickt wird.
// - Ändert sich SSID bzw. MQTT-Host ohne Passwort, wird das alte Passwort gelöscht
//   (es gehört zum alten Netz/Server).
// - Hostname: nur a–z, 0–9 und '-', nicht am Anfang/Ende, wird kleingeschrieben;
//   ungültige Namen werden ignoriert.
// - Zu lange Texte werden ignoriert statt abgeschnitten.
// - Strommodell: channelMicroamps 500–30000, baseMilliamps 0–1000 (begrenzt);
//   calibration {offMa, whiteMa} setzt beides, unplausible Messungen werden ignoriert.
uint8_t applyJson(JsonVariantConst cmd, Settings& s);

// Ohne Passwörter, stattdessen hasPass-Felder
void toJson(const Settings& s, JsonObject out);

bool validHostname(const char* name);

}  // namespace settings
