#include "storage.h"

#include <Preferences.h>

#include "effect_list.h"

namespace {

constexpr const char* NS_STATE = "state";
constexpr const char* NS_SETTINGS = "settings";

template <size_t N>
void loadText(Preferences& p, const char* key, char (&field)[N]) {
  if (p.isKey(key)) p.getString(key, field, N);
}

}  // namespace

namespace storage {

void loadState(LightState& s) {
  Preferences p;
  if (!p.begin(NS_STATE, true)) return;
  s.on = p.getBool("on", s.on);
  s.effect = p.getUChar("effect", s.effect);
  if (s.effect >= effects::count()) s.effect = 0;
  s.brightness = p.getUChar("bri", s.brightness);
  if (s.brightness < MIN_BRIGHTNESS) s.brightness = MIN_BRIGHTNESS;
  s.speed = p.getUChar("speed", s.speed);
  s.intensity = p.getUChar("int", s.intensity);
  s.color1 = p.getULong("c1", s.color1);
  s.color2 = p.getULong("c2", s.color2);
  s.cycleMask = p.getULong("cycle", s.cycleMask);
  s.autoCycleSec = p.getUShort("auto", s.autoCycleSec);
  p.end();
}

void saveState(const LightState& s) {
  Preferences p;
  if (!p.begin(NS_STATE, false)) return;
  // put* schreibt nur, wenn sich der Wert unterscheidet: schont den Flash
  p.putBool("on", s.on);
  p.putUChar("effect", s.effect);
  p.putUChar("bri", s.brightness);
  p.putUChar("speed", s.speed);
  p.putUChar("int", s.intensity);
  p.putULong("c1", s.color1);
  p.putULong("c2", s.color2);
  p.putULong("cycle", s.cycleMask);
  p.putUShort("auto", s.autoCycleSec);
  p.end();
}

void loadSettings(Settings& s) {
  Preferences p;
  if (!p.begin(NS_SETTINGS, true)) return;
  loadText(p, "host", s.hostname);
  if (!settings::validHostname(s.hostname)) strcpy(s.hostname, Settings().hostname);
  s.maxMilliamps = p.getUShort("mA", s.maxMilliamps);
  if (s.maxMilliamps < MIN_MILLIAMPS || s.maxMilliamps > MAX_MILLIAMPS) s.maxMilliamps = Settings().maxMilliamps;
  s.powerOn = static_cast<PowerOnMode>(p.getUChar("powerOn", static_cast<uint8_t>(s.powerOn)));
  if (static_cast<uint8_t>(s.powerOn) > 2) s.powerOn = PowerOnMode::Last;
  loadText(p, "ssid", s.wifiSsid);
  loadText(p, "pass", s.wifiPass);
  s.mqttEnabled = p.getBool("mqOn", s.mqttEnabled);
  loadText(p, "mqHost", s.mqttHost);
  s.mqttPort = p.getUShort("mqPort", s.mqttPort);
  loadText(p, "mqUser", s.mqttUser);
  loadText(p, "mqPass", s.mqttPass);
  loadText(p, "mqTopic", s.mqttTopic);
  s.mqttDiscovery = p.getBool("mqDisc", s.mqttDiscovery);
  p.end();
}

void saveSettings(const Settings& s) {
  Preferences p;
  if (!p.begin(NS_SETTINGS, false)) return;
  p.putString("host", s.hostname);
  p.putUShort("mA", s.maxMilliamps);
  p.putUChar("powerOn", static_cast<uint8_t>(s.powerOn));
  p.putString("ssid", s.wifiSsid);
  p.putString("pass", s.wifiPass);
  p.putBool("mqOn", s.mqttEnabled);
  p.putString("mqHost", s.mqttHost);
  p.putUShort("mqPort", s.mqttPort);
  p.putString("mqUser", s.mqttUser);
  p.putString("mqPass", s.mqttPass);
  p.putString("mqTopic", s.mqttTopic);
  p.putBool("mqDisc", s.mqttDiscovery);
  p.end();
}

void clearAll() {
  Preferences p;
  if (p.begin(NS_STATE, false)) {
    p.clear();
    p.end();
  }
  if (p.begin(NS_SETTINGS, false)) {
    p.clear();
    p.end();
  }
}

}  // namespace storage
