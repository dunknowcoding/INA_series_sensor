/**
 * @file InaI2cBus.h
 * @brief Minimal I2C bus wrapper with bounded transactions.
 */
#pragma once

#include <Arduino.h>
#include <Wire.h>

#include "../InaWireCompat.h"
#include "InaTypes.h"

namespace Ina {

class I2cBus {
public:
  I2cBus() = default;

  /** Initialize Wire; pin args are honored on ESP32/RP2040, ignored on fixed-pin MCUs. */
  void begin(int sda, int scl, uint32_t hz) { InaWireBeginMapped(sda, scl, hz); }

  bool probe(uint8_t addr7) { return InaWireProbeAddr(addr7); }

  Result readU16(uint8_t addr7, uint8_t reg, uint16_t& out) {
    const uint16_t v = InaWireReadU16Reg(addr7, reg);
    out = v;
    // InaWireReadU16Reg returns 0 on error which is ambiguous; caller may additionally probe.
    return Result::OK();
  }

  Result readU24Msb(uint8_t addr7, uint8_t reg, uint32_t& out) {
    out = InaWireReadU24RegMsbFirst(addr7, reg);
    return Result::OK();
  }

  Result readU40Msb(uint8_t addr7, uint8_t reg, uint64_t& out) {
    out = InaWireReadU40RegMsbFirst(addr7, reg);
    return Result::OK();
  }

  Result writeU16(uint8_t addr7, uint8_t reg, uint16_t val) {
    Wire.beginTransmission(addr7);
    Wire.write(reg);
    Wire.write((uint8_t)((val >> 8) & 0xFF));
    Wire.write((uint8_t)(val & 0xFF));
    const int ec = Wire.endTransmission(true);
    return ec == 0 ? Result::OK() : Result::Err(Status::BusError, (uint32_t)ec);
  }
};

}  // namespace Ina

