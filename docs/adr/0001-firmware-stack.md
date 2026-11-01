# 0001: Firmware-Stack und Aufbau

- Status: angenommen
- Datum: 2026-11-01

## Kontext

Die LEDs des Schilds sind buchstabenweise in einer Kette verdrahtet, innerhalb eines Buchstabens aber in einer Reihenfolge, die nicht der Lage auf dem Schild entspricht (siehe `layout.txt`). Effekte aus WLED, die auf dem Kettenindex basieren, sehen deshalb unruhig aus. Hardware: ESP32-C3 (einkernig, ohne FPU), 182 WS2812B, Versorgung über USB.

## Entscheidung

- Eigene Firmware mit PlatformIO und Arduino-Framework.
- **Koordinatentabelle** (`src/layout.cpp`): Jede LED bekommt x/y in LED-Abständen und ihren Buchstaben, Buchstabe für Buchstabe aus `layout.txt` abgeschrieben. Effekte rechnen räumlich (x, y, Winkel, Radius) oder entlang der Verdrahtung pro Buchstabe, wo das gewollt ist („Nachzeichnen“, „Redstone“).
- **FastLED** für Ausgabe, Farb-Mathe, Rauschen, Paletten und Strombegrenzung.
- **ESPAsyncWebServer** für Weboberfläche und JSON-API; die Seite ist in die Firmware eingebettet, damit sie auch im Setup-Netz ohne Internet funktioniert.
- **PubSubClient** für MQTT in einem eigenen FreeRTOS-Task, damit ein hängender Verbindungsaufbau die Animation nicht anhält.
- **Eine Befehlsqueue**: Webserver und MQTT ändern den Zustand nie direkt, sondern schicken JSON an die Hauptschleife. Web-API, MQTT und Taster nutzen dieselben Regeln (`light::applyJson`, `light::handleButton`).
- Fachlogik (Layout, Taster, Zustand, Einstellungen) ohne Hardwarezugriff, damit sie auf dem Rechner getestet werden kann (`pio test -e native`).
- Effektliste mit stabilen IDs; der Index wird gespeichert, neue Effekte werden nur hinten angehängt.

## Verworfene Alternativen

- WLED mit 2D-Mapping: Das Layout ist keine Matrix. Ein Mapping mit Lücken ginge zwar, aber Effekte wie „Nachzeichnen“ oder Buchstaben-Effekte wären nicht möglich.
- NeoPixelBus: Ausgabe gleichwertig, aber ohne die Mathe- und Rauschfunktionen von FastLED.
- Weboberfläche im LittleFS: zusätzlicher Upload-Schritt; die Seite ist klein genug für den Programmspeicher.

## Folgen

- Änderungen an der Verdrahtung erfordern eine Anpassung von `src/layout.cpp` und dem Layout-Test.
- Einstellungen werden nur über die Queue geändert; Antworten der API sind deshalb asynchron (202), die Oberfläche lädt danach neu.
