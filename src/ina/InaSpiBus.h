/**
 * @file InaSpiBus.h
 * @brief Minimal SPI bus wrapper for INA229-class devices (bounded transactions).
 */
#pragma once

#include <Arduino.h>
#include <SPI.h>

#include "../InaSpiCompat.h"
#include "InaTypes.h"

namespace Ina {

class SpiBus {
public:
  SpiBus(int pinCs, int pinSck, int pinMiso, int pinMosi, SPISettings settings)
      : _cs(pinCs), _sck(pinSck), _miso(pinMiso), _mosi(pinMosi), _settings(settings) {}

  void begin() { InaSpiBeginMapped(_cs, _sck, _miso, _mosi); }

  void setSettings(SPISettings s) { _settings = s; }

  Result readU16(uint8_t reg, uint16_t& out) {
    out = (uint16_t)(readRegister(reg, 2) & 0xFFFFu);
    return Result::OK();
  }

  Result readU24(uint8_t reg, uint32_t& out) {
    out = readRegister(reg, 3) & 0xFFFFFFu;
    return Result::OK();
  }

  Result readU40(uint8_t reg, uint64_t& out) {
    out = readRegister64(reg, 5);
    return Result::OK();
  }

  Result writeU16(uint8_t reg, uint16_t val) {
    const uint8_t cmd = (uint8_t)(reg << 2);
    digitalWrite(_cs, LOW);
    SPI.beginTransaction(_settings);
    SPI.transfer(cmd);
    SPI.transfer((uint8_t)(val >> 8));
    SPI.transfer((uint8_t)(val & 0xFF));
    SPI.endTransaction();
    digitalWrite(_cs, HIGH);
    return Result::OK();
  }

private:
  int _cs, _sck, _miso, _mosi;
  SPISettings _settings;

  uint32_t readRegister(uint8_t reg, uint8_t nbytes) {
    const uint8_t addr = (uint8_t)((reg << 2) | 1u);
    digitalWrite(_cs, LOW);
    SPI.beginTransaction(_settings);
    uint32_t value = SPI.transfer(addr);
    uint8_t count = nbytes;
    while (count--) {
      value <<= 8;
      value |= SPI.transfer(0);
    }
    SPI.endTransaction();
    digitalWrite(_cs, HIGH);
    return value;
  }

  uint64_t readRegister64(uint8_t reg, uint8_t nbytes) {
    const uint8_t addr = (uint8_t)((reg << 2) | 1u);
    digitalWrite(_cs, LOW);
    SPI.beginTransaction(_settings);
    (void)SPI.transfer(addr);
    uint64_t value = 0;
    for (uint8_t i = 0; i < nbytes; i++) {
      value = (value << 8) | SPI.transfer(0);
    }
    SPI.endTransaction();
    digitalWrite(_cs, HIGH);
    return value;
  }
};

}  // namespace Ina

