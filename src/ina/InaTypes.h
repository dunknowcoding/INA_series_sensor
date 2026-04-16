/**
 * @file InaTypes.h
 * @brief Shared types for INA family drivers (simple + pro APIs).
 *
 * Notes:
 * - This driver layer is independent of the JSONL bridge protocol. Bridge classes may wrap it.
 * - All docs/comments in this folder are English by requirement.
 */
#pragma once

#include <Arduino.h>

namespace Ina {

enum class Status : uint8_t {
  Ok = 0,
  BusError,
  Timeout,
  BadParam,
  Unsupported
};

struct Result {
  Status status = Status::Ok;
  uint32_t detail = 0;  // optional bus error code
  constexpr bool ok() const { return status == Status::Ok; }
  static Result OK() { return {}; }
  static Result Err(Status s, uint32_t d = 0) {
    Result r;
    r.status = s;
    r.detail = d;
    return r;
  }
};

/** Generic ALERT output configuration used by multiple INA families. */
struct AlertConfig {
  bool enable = false;
  bool latch = false;
  bool polarityActiveHigh = false;
};

/** Generic alert status flags (subset; families may extend). */
struct AlertStatus {
  bool asserted = false;
  bool conversionReady = false;
  bool overVoltage = false;
  bool underVoltage = false;
  bool overCurrent = false;
  bool overPower = false;
  bool overTemp = false;            // INA228/229 family: die temperature over-limit
  bool shuntUnderVoltage = false;   // INA228/229 family: shunt under-voltage flag
};

}  // namespace Ina

