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

/** Interval between samples when streaming at the given nominal rate (Hz). */
inline uint32_t sampleIntervalMs(int sampleHz) {
  return static_cast<uint32_t>(1000 / (sampleHz <= 0 ? 1 : sampleHz));
}

inline void pong() { Serial.println(F("{\"v\":1,\"type\":\"PONG\"}")); }

inline void ackStart() { Serial.println(F("{\"v\":1,\"type\":\"ACK\",\"cmd\":\"START\"}")); }

inline void ackStop() { Serial.println(F("{\"v\":1,\"type\":\"ACK\",\"cmd\":\"STOP\"}")); }

inline void ackSr(int hz) {
  Serial.print(F("{\"v\":1,\"type\":\"ACK\",\"cmd\":\"SR\",\"hz\":"));
  Serial.print(hz);
  Serial.println('}');
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
