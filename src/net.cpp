#include <Arduino.h>
#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <WiFi.h>

#include "net.h"

namespace {

constexpr uint32_t CONNECT_TIMEOUT_MS = 20000;   // so lange beim Start warten, dann AP
constexpr uint32_t LOST_TIMEOUT_MS = 60000;      // Verbindung verloren → AP dazuschalten
constexpr uint32_t RETRY_INTERVAL_MS = 180000;   // im AP-Modus erneut versuchen …
constexpr uint32_t RETRY_WINDOW_MS = 15000;      // … und so lange suchen lassen
constexpr uint32_t CLIENT_QUIET_MS = 180000;     // so lange nach dem letzten Setup-Client nicht suchen
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
uint32_t lastClientSeen = 0;
bool retrying = false;

void applyTxPower() {
#ifdef WIFI_LOW_TX_POWER
  // Manche C3-Boards (z. B. SuperMini) haben eine schwache Antennenanpassung und
  // funktionieren mit voller Sendeleistung schlechter
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
#endif
}

// Laufenden Verbindungsversuch abbrechen: Die Suche springt über alle Kanäle und
// nimmt den Access Point mit, dann kommt kein Handy hinein.
void stopStaSearch() {
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(false);
  retrying = false;
}

void startAp() {
  if (ap) return;
  stopStaSearch();
  // STA-Teil bleibt an, damit die WLAN-Suche in der Oberfläche funktioniert
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(apName);
  applyTxPower();
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
  WiFi.setAutoReconnect(true);
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
  applyTxPower();
  WiFi.begin(ssid, pass);
  disconnectedSince = millis();
  Serial.printf("[net] verbinde mit \"%s\"\n", ssid);
}

void loop() {
  const uint32_t now = millis();
  if (ap) {
    dns.processNextRequest();
    if (WiFi.softAPgetStationNum() > 0) lastClientSeen = now;
  }
  if (servicesStarted) ArduinoOTA.handle();
  if (!hasCredentials) return;

  if (WiFi.status() == WL_CONNECTED) {
    if (disconnectedSince) Serial.printf("[net] verbunden, IP %s\n", WiFi.localIP().toString().c_str());
    disconnectedSince = 0;
    retrying = false;
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
  if (!ap) return;
  if (retrying) {
    // Suche erfolglos oder jemand will ins Setup-Netz: abbrechen
    if (now - lastRetry > RETRY_WINDOW_MS || WiFi.softAPgetStationNum() > 0) stopStaSearch();
    return;
  }
  // Neuer Versuch nur selten und nur, wenn länger niemand im Setup-Netz war
  const bool quiet = lastClientSeen == 0 || now - lastClientSeen > CLIENT_QUIET_MS;
  if (now - lastRetry > RETRY_INTERVAL_MS && quiet && WiFi.softAPgetStationNum() == 0) {
    lastRetry = now;
    retrying = true;
    Serial.printf("[net] neuer Versuch mit \"%s\"\n", ssid);
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
