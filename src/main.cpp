#include <Arduino.h>
#include <FastLED.h>

#include "app.h"
#include "button.h"
#include "config.h"
#include "effect_list.h"
#include "effects.h"
#include "layout.h"
#include "mqtt.h"
#include "net.h"
#include "power.h"
#include "storage.h"
#include "web.h"

namespace {

struct Command {
  app::CommandType type;
  char* json;  // auf dem Heap, wird in der Hauptschleife freigegeben
};

QueueHandle_t queue;
SemaphoreHandle_t mutex;

CRGB fxBuf[NUM_LEDS];     // Arbeitsspeicher der Effekte (manche bauen auf dem letzten Bild auf)
CRGB fadeFrom[NUM_LEDS];  // letztes Bild des alten Effekts für die Überblendung
CRGB out[NUM_LEDS];       // was an die LEDs geht

LightState state;
Settings cfg;
Button button;
uint32_t stateVer = 0;
uint32_t settingsVer = 0;

uint8_t shownEffect = 255;
uint32_t fadeStart = 0;
uint8_t powerLevel = 0;   // 0 = aus, 255 = an; wird weich nachgeführt
uint32_t saveAt = 0;
uint32_t lastFrame = 0;
uint32_t lastAutoCycle = 0;
uint32_t rebootAt = 0;
volatile uint32_t estimateMa = 0;

// Messmodus für die Stromkalibrierung; endet von selbst nach CALIBRATION_TIMEOUT_MS
enum class CalMode : uint8_t { None, Off, White };
volatile CalMode calMode = CalMode::None;
uint32_t calUntil = 0;
constexpr uint32_t CALIBRATION_TIMEOUT_MS = 180000;

// Zustand ändern: Version erhöhen, Speichern vormerken
void stateChanged(bool saveNow) {
  xSemaphoreTake(mutex, portMAX_DELAY);
  stateVer++;
  xSemaphoreGive(mutex);
  if (saveNow) {
    storage::saveState(state);
    saveAt = 0;
  } else {
    saveAt = millis() + SAVE_DELAY_MS;
    if (saveAt == 0) saveAt = 1;
  }
}

void handleCommand(const Command& cmd) {
  switch (cmd.type) {
    case app::CommandType::Light: {
      JsonDocument doc;
      if (deserializeJson(doc, cmd.json)) return;
      xSemaphoreTake(mutex, portMAX_DELAY);
      const bool changed = light::applyJson(doc.as<JsonVariantConst>(), state);
      xSemaphoreGive(mutex);
      if (changed) stateChanged(false);
      break;
    }
    case app::CommandType::Settings: {
      JsonDocument doc;
      if (deserializeJson(doc, cmd.json)) return;
      xSemaphoreTake(mutex, portMAX_DELAY);
      const uint8_t changed = settings::applyJson(doc.as<JsonVariantConst>(), cfg);
      if (changed) settingsVer++;
      xSemaphoreGive(mutex);
      if (!changed) return;
      storage::saveSettings(cfg);
      // WLAN und Hostname brauchen einen sauberen Neustart
      if (changed & (CHANGED_WIFI | CHANGED_HOSTNAME)) rebootAt = millis() + 1500;
      break;
    }
    case app::CommandType::Reboot:
      rebootAt = millis() + 500;
      break;
    case app::CommandType::Calibrate: {
      JsonDocument doc;
      if (deserializeJson(doc, cmd.json)) return;
      const char* mode = doc["mode"] | "end";
      calMode = strcmp(mode, "off") == 0 ? CalMode::Off : (strcmp(mode, "white") == 0 ? CalMode::White : CalMode::None);
      calUntil = millis() + CALIBRATION_TIMEOUT_MS;
      break;
    }
    case app::CommandType::FactoryReset:
      storage::clearAll();
      rebootAt = millis() + 500;
      break;
  }
}

void processQueue() {
  Command cmd;
  while (xQueueReceive(queue, &cmd, 0) == pdTRUE) {
    handleCommand(cmd);
    free(cmd.json);
  }
}

void processButton(uint32_t now) {
  const ButtonEvent ev = button.update(digitalRead(BUTTON_PIN) == LOW, now);
  if (ev == ButtonEvent::None) return;
  xSemaphoreTake(mutex, portMAX_DELAY);
  const bool saveNow = light::handleButton(ev, state);
  xSemaphoreGive(mutex);
  stateChanged(saveNow);
}

void autoCycle(uint32_t now) {
  if (!state.on || state.autoCycleSec == 0) {
    lastAutoCycle = now;
    return;
  }
  if (now - lastAutoCycle < state.autoCycleSec * 1000UL) return;
  lastAutoCycle = now;
  xSemaphoreTake(mutex, portMAX_DELAY);
  state.effect = light::nextEffect(state);
  stateVer++;  // nicht speichern: sonst schreibt der Auto-Wechsel ständig in den Flash
  xSemaphoreGive(mutex);
}

void renderFrame(uint32_t now) {
  const uint32_t dt = now - lastFrame;
  lastFrame = now;

  // Ein-/Ausblenden
  const uint8_t target = state.on ? 255 : 0;
  const uint32_t stepLevel = dt * 255 / POWER_FADE_MS + 1;
  if (powerLevel < target) powerLevel = powerLevel + stepLevel > target ? target : powerLevel + stepLevel;
  if (powerLevel > target) powerLevel = powerLevel < target + stepLevel ? target : powerLevel - stepLevel;

  if (state.effect != shownEffect) {
    memcpy(fadeFrom, out, sizeof(out));
    fill_solid(fxBuf, NUM_LEDS, CRGB::Black);
    fx::reset();
    fadeStart = now;
    shownEffect = state.effect;
  }

  if (powerLevel > 0) {
    const FxParams p{state.speed, state.intensity, CRGB(state.color1), CRGB(state.color2)};
    fx::render(state.effect, fxBuf, p, now);
  }

  const uint32_t since = now - fadeStart;
  const uint8_t mix = since >= CROSSFADE_MS ? 255 : since * 255 / CROSSFADE_MS;
  xSemaphoreTake(mutex, portMAX_DELAY);
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    out[i] = mix == 255 ? fxBuf[i] : blend(fadeFrom[i], fxBuf[i], mix);
    out[i].nscale8_video(powerLevel);
  }
  xSemaphoreGive(mutex);

  uint8_t brightness = state.brightness;
  if (calMode != CalMode::None) {
    // Testbild ohne Begrenzung, damit die Messung zum Modell passt
    if (static_cast<int32_t>(now - calUntil) >= 0) calMode = CalMode::None;
    const uint8_t v = calMode == CalMode::White ? CALIBRATION_LEVEL : 0;
    fill_solid(out, NUM_LEDS, CRGB(v, v, v));
    brightness = 255;
  } else {
    brightness = power::limitBrightness(reinterpret_cast<const uint8_t*>(out), NUM_LEDS, brightness,
                                        cfg.maxMilliamps, cfg.power);
  }
  estimateMa = power::estimateMilliamps(reinterpret_cast<const uint8_t*>(out), NUM_LEDS, brightness, cfg.power);

  FastLED.setBrightness(brightness);
  FastLED.show();
}

}  // namespace

namespace app {

bool post(CommandType type, const char* json) {
  Command cmd{type, strdup(json ? json : "")};
  if (!cmd.json) return false;
  if (xQueueSend(queue, &cmd, 0) != pdTRUE) {
    free(cmd.json);
    return false;
  }
  return true;
}

LightState state() {
  xSemaphoreTake(mutex, portMAX_DELAY);
  LightState copy = ::state;
  xSemaphoreGive(mutex);
  return copy;
}

Settings settings() {
  xSemaphoreTake(mutex, portMAX_DELAY);
  Settings copy = cfg;
  xSemaphoreGive(mutex);
  return copy;
}

void copyFrame(uint8_t* dst) {
  xSemaphoreTake(mutex, portMAX_DELAY);
  memcpy(dst, out, sizeof(out));
  xSemaphoreGive(mutex);
}

uint32_t stateVersion() {
  xSemaphoreTake(mutex, portMAX_DELAY);
  const uint32_t v = stateVer;
  xSemaphoreGive(mutex);
  return v;
}

uint32_t estimatedMilliamps() { return estimateMa; }

const char* calibrationMode() {
  switch (calMode) {
    case CalMode::Off: return "off";
    case CalMode::White: return "white";
    default: return "";
  }
}

uint32_t settingsVersion() {
  xSemaphoreTake(mutex, portMAX_DELAY);
  const uint32_t v = settingsVer;
  xSemaphoreGive(mutex);
  return v;
}

}  // namespace app

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.printf("\n[main] Minecraft Sign %s\n", FW_VERSION);

  queue = xQueueCreate(16, sizeof(Command));
  mutex = xSemaphoreCreateMutex();

  layout::build();
  fx::init();
  storage::loadSettings(cfg);
  storage::loadState(state);
  light::applyPowerOn(cfg.powerOn, state);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Strombegrenzung übernimmt power.h, nicht FastLED (falsches Modell für 2020-LEDs)
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(out, NUM_LEDS).setCorrection(CRGB(CORRECTION_R, CORRECTION_G, CORRECTION_B));
  FastLED.clear(true);

  net::begin(cfg);
  web::begin();
  mqtt::begin();
}

void loop() {
  const uint32_t now = millis();
  processQueue();
  processButton(now);
  autoCycle(now);
  net::loop();

  if (saveAt && static_cast<int32_t>(now - saveAt) >= 0) {
    storage::saveState(state);
    saveAt = 0;
  }
  if (rebootAt && static_cast<int32_t>(now - rebootAt) >= 0) {
    if (saveAt) storage::saveState(state);
    ESP.restart();
  }

  if (now - lastFrame >= 1000 / FPS) renderFrame(now);
  delay(1);
}
