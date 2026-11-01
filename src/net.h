#pragma once

#include "settings.h"

// WLAN: verbindet sich mit dem gespeicherten Netz. Ohne Zugangsdaten oder wenn
// die Verbindung nicht klappt, startet ein offener Access Point mit Captive Portal.
namespace net {

void begin(const Settings& s);
void loop();

bool apActive();
bool connected();
// Hostname/IP prüfen: gehört die Anfrage nicht zu uns, leitet das Captive Portal um
bool isOwnHost(const char* host);
const char* apSsid();
String ip();

}  // namespace net
