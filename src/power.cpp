#include "power.h"

#include "settings.h"

namespace {

// Summe der korrigierten Kanalwerte, skaliert mit 255 * 255 je Vollaussteuerung
uint64_t correctedSum(const uint8_t* rgb, uint16_t n) {
  uint64_t sum = 0;
  for (uint16_t i = 0; i < n; i++) {
    sum += static_cast<uint32_t>(rgb[i * 3]) * CORRECTION_R + static_cast<uint32_t>(rgb[i * 3 + 1]) * CORRECTION_G +
           static_cast<uint32_t>(rgb[i * 3 + 2]) * CORRECTION_B;
  }
  return sum;
}

}  // namespace

namespace power {

uint32_t estimateMilliamps(const uint8_t* rgb, uint16_t n, uint8_t brightness, const PowerModel& m) {
  const uint64_t ledMicroamps = correctedSum(rgb, n) * brightness * m.channelMicroamps / (255ULL * 255 * 255);
  return m.baseMilliamps + static_cast<uint32_t>(ledMicroamps / 1000);
}

uint8_t limitBrightness(const uint8_t* rgb, uint16_t n, uint8_t brightness, uint16_t maxMilliamps,
                        const PowerModel& m) {
  if (maxMilliamps == 0 || brightness == 0) return brightness;
  const uint32_t full = estimateMilliamps(rgb, n, brightness, m);
  if (full <= maxMilliamps) return brightness;
  if (maxMilliamps <= m.baseMilliamps) return 1;
  // Der LED-Anteil wächst linear mit der Helligkeit
  const uint32_t scaled = static_cast<uint32_t>(brightness) * (maxMilliamps - m.baseMilliamps) / (full - m.baseMilliamps);
  return scaled < 1 ? 1 : static_cast<uint8_t>(scaled);
}

bool calibrate(uint16_t offMilliamps, uint16_t whiteMilliamps, uint16_t n, PowerModel& out) {
  if (n == 0 || whiteMilliamps <= offMilliamps || offMilliamps > MAX_BASE_MILLIAMPS) return false;
  // Testbild: n LEDs, jeder Kanal CALIBRATION_LEVEL, Helligkeit 255
  const uint64_t perChannelUnits = static_cast<uint64_t>(n) * CALIBRATION_LEVEL * (CORRECTION_R + CORRECTION_G + CORRECTION_B);
  const uint64_t micro = static_cast<uint64_t>(whiteMilliamps - offMilliamps) * 1000 * 255 * 255 / perChannelUnits;
  if (micro < MIN_CHANNEL_MICROAMPS || micro > MAX_CHANNEL_MICROAMPS) return false;
  out.channelMicroamps = static_cast<uint16_t>(micro);
  out.baseMilliamps = offMilliamps;
  return true;
}

}  // namespace power
