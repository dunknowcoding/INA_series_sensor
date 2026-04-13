/**
 * @file InaJsonlProtocol.h
 * @brief Shared JSON Lines helpers for the INA Monitor desktop application serial protocol.
 *
 * All string payloads match the legacy bridge output byte-for-byte (same field names and order).
 * Use these helpers when adding new bridge classes so the UI and tooling stay compatible.
 */
#pragma once

#include <Arduino.h>

namespace InaJsonl {

/** Strip CR/LF quirks from a serial line (host may send CRLF). */
inline void normalizeCmd(String& l) {
  l.replace("\r", "");
  l.trim();
}

/** Interval between samples when streaming at the given nominal rate (Hz). */
inline uint32_t sampleIntervalMs(int sampleHz) {
  if (sampleHz <= 0) sampleHz = 1;
  uint32_t ms = 1000u / static_cast<uint32_t>(sampleHz);
  return ms ? ms : 1u;
}

/** Microsecond period for streaming (supports rates > 1 kHz where 1 ms ticks are too coarse). */
inline uint32_t sampleIntervalMicros(int sampleHz) {
  if (sampleHz <= 0) sampleHz = 1;
  const uint32_t hz = static_cast<uint32_t>(sampleHz);
  if (hz > 2000000u) return 1u;
  return 1000000u / hz;
}

/** Max nominal streaming Hz for I²C bridges (INA219/226/228/3221/Ch1/Unknown). Match NiusRobotLab_INA_monitor I²C presets. */
constexpr int kStreamRateCapI2c = 400;
/** Max nominal streaming Hz for SPI INA229 bridge. Match NiusRobotLab_INA_monitor SPI presets. */
constexpr int kStreamRateCapSpi = 2000;

inline int clampStreamRateI2c(int hz) {
  if (hz < 1) hz = 1;
  if (hz > kStreamRateCapI2c) hz = kStreamRateCapI2c;
  return hz;
}

inline int clampStreamRateSpi(int hz) {
  if (hz < 1) hz = 1;
  if (hz > kStreamRateCapSpi) hz = kStreamRateCapSpi;
  return hz;
}

inline void pong() { Serial.println(F("{\"v\":1,\"type\":\"PONG\"}")); }

inline void ackStart() {
  Serial.println(F("{\"v\":1,\"type\":\"ACK\",\"cmd\":\"START\"}"));
  Serial.flush();
}

inline void ackStop() {
  Serial.println(F("{\"v\":1,\"type\":\"ACK\",\"cmd\":\"STOP\"}"));
  Serial.flush();
}

inline void ackSr(int hz) {
  Serial.print(F("{\"v\":1,\"type\":\"ACK\",\"cmd\":\"SR\",\"hz\":"));
  Serial.print(hz);
  Serial.println('}');
  Serial.flush();
}

inline void ackImax(float a) {
  Serial.print(F("{\"v\":1,\"type\":\"ACK\",\"cmd\":\"IMAX\",\"A\":"));
  Serial.print(a, 6);
  Serial.println('}');
}

inline void ackRshunt(float ohm) {
  Serial.print(F("{\"v\":1,\"type\":\"ACK\",\"cmd\":\"RSHUNT\",\"ohm\":"));
  Serial.print(ohm, 6);
  Serial.println('}');
}

inline void errUnknownCmd(const String& line) {
  Serial.print(F("{\"v\":1,\"type\":\"ERR\",\"msg\":\"Unknown cmd\",\"line\":\""));
  Serial.print(line);
  Serial.println(F("\"}"));
}

}  // namespace InaJsonl
