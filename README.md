# minecraft_sign

Firmware für ein LED-Schild mit dem Schriftzug **MINECRAFT**: ESP32-C3, 182 WS2812B-LEDs, ein Taster, Weboberfläche, MQTT/Home Assistant und OTA-Updates.

Die LEDs sind buchstabenweise verkettet, innerhalb der Buchstaben aber nicht in Lesereihenfolge (siehe [`layout.txt`](layout.txt)). Die Firmware kennt die Position jeder LED. Dadurch laufen Regenbogen, Wellen, Feuer und Scanner räumlich sauber über das Schild.

## Hardware

| | |
|---|---|
| Controller | ESP32-C3 (4 MB Flash, z. B. SuperMini) |
| LEDs | 182 × WS2812B, Farbreihenfolge GRB, Daten an **GPIO 20** |
| Taster | gegen GND an **GPIO 21** (interner Pull-up) |
| Versorgung | USB 5 V; Strom wird per Software begrenzt (Standard 1800 mA, einstellbar) |

GPIO 20/21 sind die UART0-Pins. Die serielle Konsole läuft über USB-CDC, deshalb sind sie frei. Andere Pins: `-DLED_PIN=…` / `-DBUTTON_PIN=…` in `build_flags`.

Manche C3-Boards (z. B. SuperMini) haben eine schwache Antenne und funken mit voller Leistung schlechter. Deshalb ist die Sendeleistung in `platformio.ini` per `-DWIFI_TX_POWER=WIFI_POWER_11dBm` reduziert. Ist das Setup-Netz nicht sichtbar, auf `WIFI_POWER_8_5dBm` gehen; bei Boards mit guter Antenne kann der Eintrag für mehr Reichweite entfernt werden.

## Erste Inbetriebnahme

1. Flashen: `pio run -e esp32c3 -t upload`
2. Das Schild öffnet das WLAN **Minecraft-Sign-XXXX**. Mit dem Handy verbinden, die Einrichtungsseite öffnet sich von selbst (sonst http://192.168.4.1).
3. Unter *Einstellungen → WLAN* das Netz suchen, Passwort eintragen, speichern. Das Schild startet neu.
4. Danach erreichbar unter **http://minecraft-sign.local**.

Wenn das gespeicherte WLAN nicht erreichbar ist, kommt das Setup-Netz automatisch wieder.

## Bedienung am Taster

| Zustand | Drücken | Wirkung |
|---|---|---|
| aus | kurz | an (letzter Zustand) |
| an | kurz | nächster Effekt |
| an | doppelt | Helligkeitsstufe |
| an | lang | aus, Zustand wird gespeichert |

Welche Effekte beim Durchschalten vorkommen, lässt sich im Tab *Modi* festlegen.

## Effekte

Statisch · Farbverlauf · Regenbogen · Regenbogen vertikal · Regenbogen-Rad · Bunte Buchstaben · Farbwechsel · Atmen · Scanner · Schreibmaschine · Buchstaben-Lauf · Nachzeichnen · Theater · Funkeln · Konfetti · Feuer · Plasma · Wellen · Matrix · Gewitter · Grasblock · Verzaubert · Redstone · Diamant · Creeper · Nether-Portal · Lava · Ozean · Polarlicht · Herzschlag

Jeder Effekt hat Geschwindigkeit und, je nach Effekt, Intensität sowie eine oder zwei Farben. Effektwechsel werden weich überblendet.

## JSON-API

| Methode | Pfad | |
|---|---|---|
| GET | `/api/state` | aktueller Zustand |
| POST | `/api/state` | Zustand ändern (Teilobjekt reicht) |
| GET | `/api/effects` | Effektliste mit Parametern |
| GET | `/api/layout` | LED-Koordinaten |
| GET | `/api/frame` | aktuelles Bild als Hex |
| GET | `/api/info` | Version, IP, WLAN, MQTT |
| GET/POST | `/api/settings` | Geräte-, WLAN- und MQTT-Einstellungen (ohne Passwörter) |
| GET | `/api/scan` | WLAN-Suche |
| POST | `/api/update` | Firmware hochladen (multipart) |
| POST | `/api/reboot`, `/api/factory-reset` | |

Beispiel:

```sh
curl -X POST http://minecraft-sign.local/api/state \
  -H 'Content-Type: application/json' \
  -d '{"on":true,"effect":"fire","brightness":120,"speed":160}'
```

Felder: `on`, `state` (`ON`/`OFF`/`TOGGLE`), `toggle`, `effect` (ID, Name oder Index), `next`, `brightness`, `speed`, `intensity` (0–255), `color1`/`color2` (`"#rrggbb"` oder `{r,g,b}`), `color` (Home-Assistant-Format für Farbe 1), `cycle` (Liste von Effekt-IDs), `autoCycle` (Sekunden, 0 = aus).

Die API hat keine Anmeldung; das Schild gehört in ein vertrauenswürdiges Netz.

## MQTT / Home Assistant

In den Einstellungen Server und Zugangsdaten eintragen. Mit aktivierter Discovery erscheint das Schild in Home Assistant automatisch als Licht mit Effektliste sowie mit Reglern für Geschwindigkeit und Intensität.

| Topic | |
|---|---|
| `<topic>/set` | Kommando, gleiches JSON wie `POST /api/state` |
| `<topic>/state` | Zustand (retained) |
| `<topic>/status` | `online`/`offline` |

Standard-Topic: `minecraft_sign`.

## OTA-Update

- Über die Weboberfläche: *Einstellungen → Firmware*, `firmware.bin` hochladen (liegt nach dem Build unter `.pio/build/esp32c3/` und als Artefakt jeder CI-Ausführung bereit).
- Über PlatformIO: `pio run -e esp32c3_ota -t upload`

## Entwicklung

Siehe [CONTRIBUTING.md](CONTRIBUTING.md) und die Entscheidungen in [docs/adr](docs/adr).
