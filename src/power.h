#pragma once

#include <stdint.h>

// Eigene Strombegrenzung. FastLED rechnet fest mit 5050-WS2812B (16/11/15 mA je
// Kanal); die verbauten 2020-LEDs brauchen deutlich weniger.
//
// Modell: Strom = Grundlast + Σ (Kanalwert nach Farbkorrektur und Helligkeit / 255)
//         × Strom je Kanal bei Vollaussteuerung

// Farbkorrektur, die auch an FastLED übergeben wird (TypicalLEDStrip)
constexpr uint8_t CORRECTION_R = 255;
constexpr uint8_t CORRECTION_G = 176;
constexpr uint8_t CORRECTION_B = 240;

// Testbild für die Kalibrierung: alle LEDs weiß mit diesem Wert, Helligkeit 255
constexpr uint8_t CALIBRATION_LEVEL = 128;

struct PowerModel {
  uint16_t channelMicroamps;  // Strom eines Kanals bei 255, in µA
  uint16_t baseMilliamps;     // ESP + Ruhestrom aller LEDs
};

namespace power {

// rgb: n LEDs als R,G,B-Bytes (vor Helligkeit und Korrektur)
uint32_t estimateMilliamps(const uint8_t* rgb, uint16_t n, uint8_t brightness, const PowerModel& m);

// Größte Helligkeit ≤ brightness, bei der das Limit eingehalten wird (mindestens 1).
// maxMilliamps = 0 bedeutet ohne Limit.
uint8_t limitBrightness(const uint8_t* rgb, uint16_t n, uint8_t brightness, uint16_t maxMilliamps,
                        const PowerModel& m);

// Aus zwei Messungen am Netzteil (alle aus / Testbild) das Modell bestimmen.
// false, wenn die Werte unplausibel sind (weiß ≤ aus oder Ergebnis außerhalb der Grenzen).
bool calibrate(uint16_t offMilliamps, uint16_t whiteMilliamps, uint16_t n, PowerModel& out);

}  // namespace power
