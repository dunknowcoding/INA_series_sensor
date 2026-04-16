/**
 * @file InaCh1Driver.h
 * @brief CH1-only driver (INA2227 / INA4230 / INA4235 style register map).
 *
 * These parts may have dedicated alert pins, but this library currently only exposes CH1 sampling
 * plus the standard I2C register access hooks. Special-pin support can be added per datasheet.
 */
#pragma once

#include <Arduino.h>

#include "InaI2cBus.h"
#include "InaTypes.h"

namespace Ina {

class InaCh1Driver {
public:
  explicit InaCh1Driver(I2cBus& bus, uint8_t addr7 = 0x40) : _bus(bus), _addr(addr7) {}

  uint8_t address() const { return _addr; }
  void setAddress(uint8_t addr7) { _addr = addr7; }

  // Pro API: basic register read/write for extensions.
  Result readU16(uint8_t reg, uint16_t& out) { return _bus.readU16(_addr, reg, out); }
  Result writeU16(uint8_t reg, uint16_t v) { return _bus.writeU16(_addr, reg, v); }

private:
  I2cBus& _bus;
  uint8_t _addr;
};

}  // namespace Ina

