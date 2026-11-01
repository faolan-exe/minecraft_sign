# 0003: Eigene Strombegrenzung für 2020-LEDs

- Status: angenommen
- Datum: 2026-11-01

## Kontext

Verbaut sind WS2812 im 2020-SMD-Gehäuse (5-mm-Streifen). Gemessen am Labornetzteil: höchstens etwa 1 A bei vollem Regenbogen über alle 182 LEDs, ohne Begrenzung in WLED. FastLEDs `setMaxPowerInVoltsAndMilliamps` rechnet fest mit 5050-WS2812B (16/11/15 mA je Kanal, 1 mA Ruhestrom) und hätte das Schild bei einem Limit von 1800 mA schon weit unterhalb des echten Verbrauchs gedimmt.

## Entscheidung

- Eigene Schätzung in `src/power.cpp`: Grundlast + Σ (Kanalwert × Farbkorrektur × Helligkeit / 255³) × Strom je Kanal. Die Farbkorrektur ist dieselbe, die FastLED anwendet.
- Voreinstellung: 5 mA je Kanal (Datenblatt WS2812C-2020; ergibt für einen vollen Regenbogen rund 0,9 A und passt zur Messung), Grundlast 100 mA (Annahme für ESP mit WLAN und Ruhestrom der LEDs).
- Kalibrierung mit zwei Messungen am Netzteil: alle LEDs aus (= Grundlast) und alle Kanäle auf 128 bei voller Helligkeit. Daraus wird der Strom je Kanal berechnet. Unplausible Werte werden verworfen (500–30 000 µA je Kanal sind erlaubt, Grundlast bis 1000 mA).
- Limit: Standard 1800 mA (2-A-USB-Netzteil mit Reserve), einstellbar 200–5000 mA.
- Das Testbild läuft ohne Begrenzung und endet nach 3 min von selbst.

## Verworfene Alternativen

- FastLED-Begrenzung mit hochgerechnetem Limit: Rot, Grün und Blau sind in FastLED unterschiedlich gewichtet, bei 2020-LEDs aber etwa gleich; die Schätzung wäre je nach Farbe falsch.
- Nur Datenblattwert ohne Kalibrierung: Streifen verschiedener Hersteller weichen voneinander ab.

## Folgen

- Ohne Kalibrierung ist die Grundlast nur geschätzt; die Abweichung beträgt einige 10 mA.
- Ein anderer LED-Typ braucht eine neue Kalibrierung oder andere Voreinstellungen.
