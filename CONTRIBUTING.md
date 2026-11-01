# Mitarbeit an minecraft_sign

Firmware für ein LED-Schild mit dem Schriftzug „MINECRAFT“ (182 WS2812B-LEDs, ESP32-C3). Eigene Effekte, die die Geometrie der Buchstaben kennen, Bedienung über einen Taster, eine Weboberfläche und MQTT.

## Einrichten nach dem Klonen

```sh
git config core.hooksPath .githooks
chmod +x .githooks/pre-push
```

Der Hook blockt direkte Pushes auf `main` lokal.

## Harte Regeln

- Die Verkabelung der LEDs ist fest und in [`layout.txt`](layout.txt) beschrieben. Die Koordinatentabelle in `src/layout.cpp` muss genau dazu passen; Änderungen nur zusammen mit `layout.txt` und dem Test `test/test_layout`.
- Keine WLAN-Zugangsdaten oder MQTT-Passwörter im Code. Alles wird zur Laufzeit über die Weboberfläche gesetzt und im NVS-Flash gespeichert.

## Stack und Befehle

C++ (Arduino-Framework) mit [PlatformIO](https://platformio.org/).

| Zweck | Befehl |
|---|---|
| Firmware bauen | `pio run -e esp32c3` |
| Flashen per USB | `pio run -e esp32c3 -t upload` |
| Flashen per OTA | `pio run -e esp32c3_ota -t upload` |
| Unit-Tests (Host) | `pio test -e native` |
| Lint (cppcheck) | `pio check -e esp32c3 --fail-on-defect=high` |
| Serieller Monitor | `pio device monitor` |

## Code-Regeln

- Kleine, fokussierte Änderungen. Kein Refactoring nebenbei, das nicht zur Aufgabe gehört.
- Jede Fachregel bekommt einen Test, der ihre Grenzfälle abdeckt.
- Keine Secrets im Code. Konfiguration über Umgebungsvariablen, `.env` ist in `.gitignore`.

## Git

- Nie direkt auf `main` pushen, nie force-pushen auf geteilte Branches.
- Branch pro Aufgabe: `feat/<nr>-<kurz>`, `fix/<nr>-<kurz>`, `docs/<kurz>`, `chore/<kurz>`.
- Commits nach Conventional Commits: `feat:`, `fix:`, `docs:`, `refactor:`, `test:`, `chore:`.
- PR nach `main`, Squash-Merge, sobald die CI (Tests + Lint) grün ist. Review bei Bedarf auch nachträglich.
- Vor dem PR: Tests und Lint lokal grün.

## Arbeitsweise

- Zu Beginn: `git pull`, zugehöriges Issue lesen. Offene Arbeit steht in GitHub-Issues.
- Eine Architektur- oder Fachentscheidung getroffen → ADR in `docs/adr/` anlegen (Vorlage `0000-vorlage.md`, Status `vorgeschlagen`, bis alle Beteiligten zugestimmt haben).
