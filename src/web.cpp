#include "web.h"

#include <Arduino.h>
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>
#include <Update.h>
#include <WiFi.h>

#include "app.h"
#include "config.h"
#include "effect_list.h"
#include "layout.h"
#include "mqtt.h"
#include "net.h"
#include "web_ui.h"

namespace {

AsyncWebServer server(80);

void sendJson(AsyncWebServerRequest* req, const JsonDocument& doc, int code = 200) {
  String body;
  serializeJson(doc, body);
  req->send(code, "application/json", body);
}

void sendOk(AsyncWebServerRequest* req, int code = 200) { req->send(code, "application/json", "{\"ok\":true}"); }

// JSON-Body an die Hauptschleife weiterreichen
void addCommandRoute(const char* path, app::CommandType type) {
  auto* handler = new AsyncCallbackJsonWebHandler(path, [type](AsyncWebServerRequest* req, JsonVariant& json) {
    String body;
    serializeJson(json, body);
    if (app::post(type, body.c_str())) sendOk(req, 202);
    else req->send(503, "application/json", "{\"error\":\"busy\"}");
  });
  handler->setMethod(HTTP_POST);
  server.addHandler(handler);
}

void handleState(AsyncWebServerRequest* req) {
  JsonDocument doc;
  light::toJson(app::state(), doc.to<JsonObject>());
  sendJson(req, doc);
}

void handleEffects(AsyncWebServerRequest* req) {
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (uint8_t i = 0; i < effects::count(); i++) {
    const EffectInfo& e = effects::info(i);
    JsonObject o = arr.add<JsonObject>();
    o["id"] = e.id;
    o["name"] = e.name;
    o["color1"] = (e.flags & FX_COLOR1) != 0;
    o["color2"] = (e.flags & FX_COLOR2) != 0;
    if (e.intensityLabel) o["intensity"] = e.intensityLabel;
  }
  sendJson(req, doc);
}

void handleLayout(AsyncWebServerRequest* req) {
  JsonDocument doc;
  doc["width"] = layout::width();
  doc["height"] = LAYOUT_HEIGHT;
  JsonArray leds = doc["leds"].to<JsonArray>();
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    JsonArray p = leds.add<JsonArray>();
    p.add(roundf(layout::pos(i).x * 100) / 100);
    p.add(roundf(layout::pos(i).y * 100) / 100);
  }
  sendJson(req, doc);
}

// Aktuelles Bild als Hex-Text (rrggbb je LED), für die Vorschau
void handleFrame(AsyncWebServerRequest* req) {
  static const char HEX_DIGITS[] = "0123456789abcdef";
  uint8_t frame[NUM_LEDS * 3];
  app::copyFrame(frame);
  String body;
  body.reserve(sizeof(frame) * 2);
  for (uint8_t b : frame) {
    body += HEX_DIGITS[b >> 4];
    body += HEX_DIGITS[b & 0xF];
  }
  req->send(200, "text/plain", body);
}

void handleInfo(AsyncWebServerRequest* req) {
  JsonDocument doc;
  doc["version"] = FW_VERSION;
  doc["hostname"] = app::settings().hostname;
  doc["ip"] = net::ip();
  doc["wifi"] = net::connected() ? WiFi.SSID() : "";
  doc["rssi"] = net::connected() ? WiFi.RSSI() : 0;
  doc["setupMode"] = net::apActive();
  doc["apSsid"] = net::apSsid();
  doc["mqtt"] = mqtt::connected();
  doc["uptime"] = millis() / 1000;
  doc["heap"] = ESP.getFreeHeap();
  doc["leds"] = NUM_LEDS;
  doc["currentMa"] = app::estimatedMilliamps();
  doc["calibration"] = app::calibrationMode();
  sendJson(req, doc);
}

void handleSettings(AsyncWebServerRequest* req) {
  JsonDocument doc;
  settings::toJson(app::settings(), doc.to<JsonObject>());
  sendJson(req, doc);
}

void handleScan(AsyncWebServerRequest* req) {
  JsonDocument doc;
  const int16_t n = WiFi.scanComplete();
  if (n == WIFI_SCAN_FAILED) {
    WiFi.scanNetworks(true);
    doc["scanning"] = true;
  } else if (n == WIFI_SCAN_RUNNING) {
    doc["scanning"] = true;
  } else {
    JsonArray list = doc["networks"].to<JsonArray>();
    for (int16_t i = 0; i < n; i++) {
      if (WiFi.SSID(i).isEmpty()) continue;
      JsonObject o = list.add<JsonObject>();
      o["ssid"] = WiFi.SSID(i);
      o["rssi"] = WiFi.RSSI(i);
      o["open"] = WiFi.encryptionType(i) == WIFI_AUTH_OPEN;
    }
    WiFi.scanDelete();
  }
  sendJson(req, doc);
}

void handleUpdateDone(AsyncWebServerRequest* req) {
  const bool ok = !Update.hasError();
  req->send(ok ? 200 : 500, "application/json", ok ? "{\"ok\":true}" : "{\"error\":\"update failed\"}");
  if (ok) app::post(app::CommandType::Reboot);
}

void handleUpdateUpload(AsyncWebServerRequest*, const String& filename, size_t index, uint8_t* data, size_t len,
                        bool final) {
  if (index == 0) {
    Serial.printf("[web] OTA-Update %s\n", filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) Update.printError(Serial);
  }
  if (Update.isRunning() && Update.write(data, len) != len) Update.printError(Serial);
  if (final && !Update.end(true)) Update.printError(Serial);
}

void handleNotFound(AsyncWebServerRequest* req) {
  // Captive Portal: fremde Hosts (Verbindungstests der Handys) auf die Startseite
  if (net::apActive() && !net::isOwnHost(req->host().c_str())) {
    req->redirect("http://192.168.4.1/");
    return;
  }
  if (req->method() == HTTP_GET && !req->url().startsWith("/api/")) {
    req->redirect("/");
    return;
  }
  req->send(404, "application/json", "{\"error\":\"not found\"}");
}

}  // namespace

namespace web {

void begin() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    AsyncWebServerResponse* res = req->beginResponse(200, "text/html", reinterpret_cast<const uint8_t*>(INDEX_HTML),
                                                     strlen(INDEX_HTML));
    res->addHeader("Cache-Control", "no-cache");
    req->send(res);
  });
  server.on("/api/state", HTTP_GET, handleState);
  server.on("/api/effects", HTTP_GET, handleEffects);
  server.on("/api/layout", HTTP_GET, handleLayout);
  server.on("/api/frame", HTTP_GET, handleFrame);
  server.on("/api/info", HTTP_GET, handleInfo);
  server.on("/api/settings", HTTP_GET, handleSettings);
  server.on("/api/scan", HTTP_GET, handleScan);
  server.on("/api/reboot", HTTP_POST, [](AsyncWebServerRequest* req) {
    app::post(app::CommandType::Reboot);
    sendOk(req);
  });
  server.on("/api/factory-reset", HTTP_POST, [](AsyncWebServerRequest* req) {
    app::post(app::CommandType::FactoryReset);
    sendOk(req);
  });
  server.on("/api/update", HTTP_POST, handleUpdateDone, handleUpdateUpload);
  addCommandRoute("/api/state", app::CommandType::Light);
  addCommandRoute("/api/settings", app::CommandType::Settings);
  addCommandRoute("/api/calibrate", app::CommandType::Calibrate);
  server.onNotFound(handleNotFound);
  server.begin();
}

}  // namespace web
