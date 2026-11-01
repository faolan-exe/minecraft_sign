#include "mqtt.h"

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include "app.h"
#include "config.h"
#include "effect_list.h"
#include "net.h"

namespace {

constexpr uint32_t RETRY_MS = 15000;

WiFiClient tcp;
PubSubClient client(tcp);
Settings cfg;
char deviceId[32];
char topicStatus[80];
char topicState[80];
char topicSet[80];
volatile bool isConnected = false;
uint32_t publishedState = UINT32_MAX;
uint32_t appliedSettings = UINT32_MAX;
uint32_t lastAttempt = 0;

void onMessage(char* topic, uint8_t* payload, unsigned int len) {
  if (strcmp(topic, topicSet) != 0) return;
  char* json = static_cast<char*>(malloc(len + 1));
  if (!json) return;
  memcpy(json, payload, len);
  json[len] = '\0';
  app::post(app::CommandType::Light, json);
  free(json);
}

void addDevice(JsonObject root) {
  JsonObject dev = root["dev"].to<JsonObject>();
  dev["ids"].to<JsonArray>().add(deviceId);
  dev["name"] = "Minecraft Sign";
  dev["mf"] = "DIY";
  dev["mdl"] = "Minecraft Sign (ESP32-C3)";
  dev["sw"] = FW_VERSION;
  dev["cu"] = String("http://") + net::ip() + "/";
}

void publishJson(const char* topic, const JsonDocument& doc) {
  String body;
  serializeJson(doc, body);
  client.publish(topic, body.c_str(), true);
}

void publishDiscovery() {
  char topic[128];
  {
    JsonDocument doc;
    doc["name"] = nullptr;  // Gerätename übernehmen
    doc["uniq_id"] = deviceId;
    doc["schema"] = "json";
    doc["cmd_t"] = topicSet;
    doc["stat_t"] = topicState;
    doc["avty_t"] = topicStatus;
    doc["brightness"] = true;
    doc["effect"] = true;
    JsonArray list = doc["effect_list"].to<JsonArray>();
    for (uint8_t i = 0; i < effects::count(); i++) list.add(effects::info(i).name);
    doc["supported_color_modes"].to<JsonArray>().add("rgb");
    addDevice(doc.as<JsonObject>());
    snprintf(topic, sizeof(topic), "homeassistant/light/%s/config", deviceId);
    publishJson(topic, doc);
  }
  // Speed und Intensität als Schieberegler
  const char* numbers[][2] = {{"speed", "Geschwindigkeit"}, {"intensity", "Intensität"}};
  for (auto& n : numbers) {
    JsonDocument doc;
    char uid[48];
    snprintf(uid, sizeof(uid), "%s_%s", deviceId, n[0]);
    doc["name"] = n[1];
    doc["uniq_id"] = uid;
    doc["cmd_t"] = topicSet;
    doc["cmd_tpl"] = String("{\"") + n[0] + "\": {{ value | int }}}";
    doc["stat_t"] = topicState;
    doc["val_tpl"] = String("{{ value_json.") + n[0] + " }}";
    doc["avty_t"] = topicStatus;
    doc["min"] = 0;
    doc["max"] = 255;
    doc["mode"] = "slider";
    addDevice(doc.as<JsonObject>());
    snprintf(topic, sizeof(topic), "homeassistant/number/%s/config", uid);
    publishJson(topic, doc);
  }
}

void publishState() {
  JsonDocument doc;
  const LightState s = app::state();
  light::toJson(s, doc.to<JsonObject>());
  // Home Assistant erwartet den Anzeigenamen aus effect_list
  doc["effect_id"] = doc["effect"];
  doc["effect"] = effects::info(s.effect).name;
  publishJson(topicState, doc);
}

void applySettings() {
  cfg = app::settings();
  uint8_t mac[6];
  WiFi.macAddress(mac);
  snprintf(deviceId, sizeof(deviceId), "minecraft_sign_%02x%02x%02x", mac[3], mac[4], mac[5]);
  snprintf(topicStatus, sizeof(topicStatus), "%s/status", cfg.mqttTopic);
  snprintf(topicState, sizeof(topicState), "%s/state", cfg.mqttTopic);
  snprintf(topicSet, sizeof(topicSet), "%s/set", cfg.mqttTopic);
  if (client.connected()) client.disconnect();
  isConnected = false;
  lastAttempt = 0;
}

void connect() {
  client.setServer(cfg.mqttHost, cfg.mqttPort);
  Serial.printf("[mqtt] verbinde mit %s:%u\n", cfg.mqttHost, cfg.mqttPort);
  const char* user = cfg.mqttUser[0] ? cfg.mqttUser : nullptr;
  const char* pass = cfg.mqttUser[0] ? cfg.mqttPass : nullptr;
  if (!client.connect(deviceId, user, pass, topicStatus, 0, true, "offline")) {
    Serial.printf("[mqtt] fehlgeschlagen (%d)\n", client.state());
    return;
  }
  client.publish(topicStatus, "online", true);
  client.subscribe(topicSet);
  if (cfg.mqttDiscovery) publishDiscovery();
  publishedState = UINT32_MAX;  // Zustand neu senden
  Serial.println("[mqtt] verbunden");
}

void task(void*) {
  client.setBufferSize(2048);
  client.setCallback(onMessage);
  client.setSocketTimeout(5);
  for (;;) {
    const uint32_t sv = app::settingsVersion();
    if (sv != appliedSettings) {
      appliedSettings = sv;
      applySettings();
    }

    if (!cfg.mqttEnabled || !cfg.mqttHost[0] || !net::connected()) {
      isConnected = false;
      vTaskDelay(pdMS_TO_TICKS(500));
      continue;
    }

    if (!client.connected()) {
      isConnected = false;
      const uint32_t now = millis();
      if (lastAttempt == 0 || now - lastAttempt > RETRY_MS) {
        lastAttempt = now;
        connect();
      }
    }

    if (client.connected()) {
      isConnected = true;
      client.loop();
      const uint32_t v = app::stateVersion();
      if (v != publishedState) {
        publishedState = v;
        publishState();
      }
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

}  // namespace

namespace mqtt {

void begin() { xTaskCreate(task, "mqtt", 6144, nullptr, 1, nullptr); }

bool connected() { return isConnected; }

}  // namespace mqtt
