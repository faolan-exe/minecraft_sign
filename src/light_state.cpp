#include "light_state.h"

#include <stdio.h>
#include <string.h>

#include "effect_list.h"

namespace {

uint8_t clampByte(long v) { return static_cast<uint8_t>(v < 0 ? 0 : (v > 255 ? 255 : v)); }

bool readColor(JsonVariantConst v, uint32_t& out) {
  if (v.is<const char*>()) return light::parseColor(v.as<const char*>(), out);
  if (v.is<JsonObjectConst>()) {
    JsonObjectConst o = v.as<JsonObjectConst>();
    if (!o["r"].is<long>() || !o["g"].is<long>() || !o["b"].is<long>()) return false;
    out = (static_cast<uint32_t>(clampByte(o["r"].as<long>())) << 16) |
          (static_cast<uint32_t>(clampByte(o["g"].as<long>())) << 8) | clampByte(o["b"].as<long>());
    return true;
  }
  return false;
}

int readEffect(JsonVariantConst v) {
  if (v.is<long>()) {
    long i = v.as<long>();
    return (i >= 0 && i < effects::count()) ? static_cast<int>(i) : -1;
  }
  if (v.is<const char*>()) return effects::find(v.as<const char*>());
  return -1;
}

}  // namespace

bool operator==(const LightState& a, const LightState& b) {
  return a.on == b.on && a.effect == b.effect && a.brightness == b.brightness && a.speed == b.speed &&
         a.intensity == b.intensity && a.color1 == b.color1 && a.color2 == b.color2 &&
         a.cycleMask == b.cycleMask && a.autoCycleSec == b.autoCycleSec;
}

namespace light {

uint8_t nextEffect(const LightState& s) {
  const uint8_t n = effects::count();
  const uint32_t all = n >= 32 ? 0xFFFFFFFFu : ((1u << n) - 1);
  const uint32_t mask = (s.cycleMask & all) ? (s.cycleMask & all) : all;
  for (uint8_t step = 1; step <= n; step++) {
    const uint8_t candidate = static_cast<uint8_t>((s.effect + step) % n);
    if (mask & (1u << candidate)) return candidate;
  }
  return s.effect;
}

uint8_t nextBrightnessStep(uint8_t current) {
  for (uint8_t step : BRIGHTNESS_STEPS) {
    if (step > current) return step;
  }
  return BRIGHTNESS_STEPS[0];
}

bool handleButton(ButtonEvent ev, LightState& s) {
  if (!s.on) {
    // Aus: jeder Klick schaltet mit dem gespeicherten Zustand ein
    if (ev == ButtonEvent::Click || ev == ButtonEvent::DoubleClick) s.on = true;
    return false;
  }
  switch (ev) {
    case ButtonEvent::Click:
      s.effect = nextEffect(s);
      return false;
    case ButtonEvent::DoubleClick:
      s.brightness = nextBrightnessStep(s.brightness);
      return false;
    case ButtonEvent::LongPress:
      s.on = false;
      return true;
    default:
      return false;
  }
}

void applyPowerOn(PowerOnMode mode, LightState& s) {
  if (mode == PowerOnMode::On) s.on = true;
  if (mode == PowerOnMode::Off) s.on = false;
}

bool applyJson(JsonVariantConst cmd, LightState& s) {
  if (!cmd.is<JsonObjectConst>()) return false;
  const LightState before = s;

  if (cmd["on"].is<bool>()) s.on = cmd["on"].as<bool>();
  if (cmd["state"].is<const char*>()) {
    const char* st = cmd["state"].as<const char*>();
    if (strcmp(st, "ON") == 0) s.on = true;
    else if (strcmp(st, "OFF") == 0) s.on = false;
    else if (strcmp(st, "TOGGLE") == 0) s.on = !s.on;
  }
  if (cmd["toggle"].as<bool>()) s.on = !s.on;

  int fx = readEffect(cmd["effect"]);
  if (fx >= 0) s.effect = static_cast<uint8_t>(fx);
  if (cmd["next"].as<bool>()) s.effect = nextEffect(s);

  if (cmd["brightness"].is<long>()) {
    uint8_t b = clampByte(cmd["brightness"].as<long>());
    s.brightness = b < MIN_BRIGHTNESS ? MIN_BRIGHTNESS : b;
  }
  if (cmd["speed"].is<long>()) s.speed = clampByte(cmd["speed"].as<long>());
  if (cmd["intensity"].is<long>()) s.intensity = clampByte(cmd["intensity"].as<long>());

  uint32_t c;
  if (readColor(cmd["color"], c)) s.color1 = c;  // Home-Assistant-Format
  if (readColor(cmd["color1"], c)) s.color1 = c;
  if (readColor(cmd["color2"], c)) s.color2 = c;

  if (cmd["cycle"].is<JsonArrayConst>()) {
    uint32_t mask = 0;
    for (JsonVariantConst v : cmd["cycle"].as<JsonArrayConst>()) {
      int i = readEffect(v);
      if (i >= 0) mask |= 1u << i;
    }
    s.cycleMask = mask;
  }
  if (cmd["autoCycle"].is<long>()) {
    long v = cmd["autoCycle"].as<long>();
    s.autoCycleSec = static_cast<uint16_t>(v < 0 ? 0 : (v > 3600 ? 3600 : v));
  }

  return !(before == s);
}

void toJson(const LightState& s, JsonObject out) {
  char buf[8];
  out["on"] = s.on;
  out["state"] = s.on ? "ON" : "OFF";
  out["effect"] = effects::info(s.effect).id;
  out["brightness"] = s.brightness;
  out["speed"] = s.speed;
  out["intensity"] = s.intensity;
  formatColor(s.color1, buf);
  out["color1"] = buf;
  formatColor(s.color2, buf);
  out["color2"] = buf;
  JsonObject color = out["color"].to<JsonObject>();
  color["r"] = (s.color1 >> 16) & 0xFF;
  color["g"] = (s.color1 >> 8) & 0xFF;
  color["b"] = s.color1 & 0xFF;
  out["color_mode"] = "rgb";
  JsonArray cycle = out["cycle"].to<JsonArray>();
  for (uint8_t i = 0; i < effects::count(); i++) {
    if (s.cycleMask & (1u << i)) cycle.add(effects::info(i).id);
  }
  out["autoCycle"] = s.autoCycleSec;
}

bool parseColor(const char* text, uint32_t& out) {
  if (!text) return false;
  if (*text == '#') text++;
  if (strlen(text) != 6) return false;
  uint32_t v = 0;
  for (int i = 0; i < 6; i++) {
    char ch = text[i];
    uint32_t d;
    if (ch >= '0' && ch <= '9') d = ch - '0';
    else if (ch >= 'a' && ch <= 'f') d = ch - 'a' + 10;
    else if (ch >= 'A' && ch <= 'F') d = ch - 'A' + 10;
    else return false;
    v = (v << 4) | d;
  }
  out = v;
  return true;
}

void formatColor(uint32_t rgb, char out[8]) {
  snprintf(out, 8, "#%06lx", static_cast<unsigned long>(rgb & 0xFFFFFF));
}

}  // namespace light
