#include <Arduino.h>
#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <WiFi.h>

#include "net.h"

namespace {

constexpr uint32_t CONNECT_TIMEOUT_MS = 20000;   // so lange beim Start warten, dann AP
constexpr uint32_t LOST_TIMEOUT_MS = 60000;      // Verbindung verloren → AP dazuschalten
constexpr uint32_t RETRY_INTERVAL_MS = 30000;    // im AP-Modus erneut versuchen
const IPAddress AP_IP(192, 168, 4, 1);

DNSServer dns;
char apName[32];
char hostname[32];
char ssid[33];
char pass[65];
bool hasCredentials = false;
bool ap = false;
bool servicesStarted = false;
uint32_t disconnectedSince = 0;
uint32_t lastRetry = 0;

void startAp() {
  if (ap) return;
  WiFi.mode(hasCredentials ? WIFI_AP_STA : WIFI_AP);
  WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(apName);
  // Alle Namen auf uns auflösen, damit Handys die Anmeldeseite öffnen
  dns.setErrorReplyCode(DNSReplyCode::NoError);
  dns.start(53, "*", AP_IP);
  ap = true;
  Serial.printf("[net] Access Point \"%s\" auf %s\n", apName, AP_IP.toString().c_str());
}

void stopAp() {
  if (!ap) return;
  dns.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  ap = false;
  Serial.println("[net] Access Point aus");
}

void startServices() {
  if (servicesStarted) return;
  servicesStarted = true;
  if (MDNS.begin(hostname)) MDNS.addService("http", "tcp", 80);
  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.begin();
}

}  // namespace

namespace net {

void begin(const Settings& s) {
  strlcpy(hostname, s.hostname, sizeof(hostname));
  strlcpy(ssid, s.wifiSsid, sizeof(ssid));
  strlcpy(pass, s.wifiPass, sizeof(pass));
  hasCredentials = ssid[0] != '\0';

  uint8_t mac[6];
  WiFi.macAddress(mac);
  snprintf(apName, sizeof(apName), "Minecraft-Sign-%02X%02X", mac[4], mac[5]);

  WiFi.persistent(false);  // Zugangsdaten verwaltet storage, nicht der WLAN-Stack
  WiFi.setHostname(hostname);
  WiFi.setAutoReconnect(true);

  if (!hasCredentials) {
    startAp();
    startServices();
    return;
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  disconnectedSince = millis();
  Serial.printf("[net] verbinde mit \"%s\"\n", ssid);
}

void loop() {
  const uint32_t now = millis();
  if (ap) dns.processNextRequest();
  if (servicesStarted) ArduinoOTA.handle();
  if (!hasCredentials) return;

  if (WiFi.status() == WL_CONNECTED) {
    if (disconnectedSince) Serial.printf("[net] verbunden, IP %s\n", WiFi.localIP().toString().c_str());
    disconnectedSince = 0;
    startServices();
    // Setup-Netz abschalten, sobald niemand mehr daran hängt
    if (ap && WiFi.softAPgetStationNum() == 0) stopAp();
    return;
  }

  if (!disconnectedSince) disconnectedSince = now;
  const uint32_t limit = servicesStarted ? LOST_TIMEOUT_MS : CONNECT_TIMEOUT_MS;
  if (!ap && now - disconnectedSince > limit) {
    startAp();
    startServices();
    lastRetry = now;
  }
  // Neuer Versuch nur, wenn gerade niemand im Setup-Netz ist (Kanalwechsel stört)
  if (ap && now - lastRetry > RETRY_INTERVAL_MS && WiFi.softAPgetStationNum() == 0) {
    lastRetry = now;
    WiFi.begin(ssid, pass);
  }
}

bool apActive() { return ap; }
bool connected() { return WiFi.status() == WL_CONNECTED; }

bool isOwnHost(const char* host) {
  if (!host || !host[0]) return true;
  char local[40];
  snprintf(local, sizeof(local), "%s.local", hostname);
  return strcmp(host, AP_IP.toString().c_str()) == 0 || strcasecmp(host, local) == 0 ||
         strcasecmp(host, hostname) == 0 || strcmp(host, WiFi.localIP().toString().c_str()) == 0;
}

const char* apSsid() { return apName; }

String ip() { return connected() ? WiFi.localIP().toString() : (ap ? AP_IP.toString() : String("")); }

}  // namespace net
