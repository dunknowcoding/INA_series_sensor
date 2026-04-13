/**
 * @file InaWireCompat.h
 * @brief Portable I2C (Wire) initialization for ESP32, RP2040 (Pico), and boards with fixed I2C pins.
 */
#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <stdio.h>
#include <stddef.h>

/**
 * Start I2C at @p i2cHz. On ESP32 and RP2040 (Arduino-Pico), @p pinSda and @p pinScl select the bus pins.
 * On other architectures (e.g. AVR ATmega328 Uno/Nano), pins are ignored and the core uses the
 * board default I2C pins (hardware SDA/SCL).
 */
inline void InaWireBeginMapped(int pinSda, int pinScl, uint32_t i2cHz) {
#if defined(ARDUINO_ARCH_ESP32)
  Wire.begin(pinSda, pinScl);
  /* Stuck SDA/SCL (short, missing pull-ups, bad module) otherwise can block forever. */
  Wire.setTimeOut(100);
#elif defined(ARDUINO_ARCH_RP2040)
  Wire.begin(pinSda, pinScl);
#else
  (void)pinSda;
  (void)pinScl;
  Wire.begin();
#endif
  Wire.setClock(i2cHz);
}

/**
 * Read @p n bytes from register @p reg: write pointer with STOP, then read in a new transaction.
 * TI INA parts allow this; it avoids ESP32 Arduino Wire deadlocks seen with
 * endTransmission(false) + requestFrom (repeated START) on some boards/modules.
 */
inline size_t InaWireReadRegister(uint8_t addr7, uint8_t reg, uint8_t* buf, size_t n) {
  Wire.beginTransmission(addr7);
  Wire.write(reg);
  if (Wire.endTransmission(true) != 0) return 0;
  const size_t got = (size_t)Wire.requestFrom((int)addr7, (int)n);
  if (got != n) return 0;
  for (size_t i = 0; i < n; i++) {
    const int v = Wire.read();
    if (v < 0) return 0;
    buf[i] = (uint8_t)v;
  }
  return n;
}

inline uint16_t InaWireReadU16Reg(uint8_t addr7, uint8_t reg) {
  uint8_t b[2];
  if (InaWireReadRegister(addr7, reg, b, 2) != 2) return 0;
  return (uint16_t)(((uint16_t)b[0] << 8) | (uint16_t)b[1]);
}

inline uint32_t InaWireReadU24RegMsbFirst(uint8_t addr7, uint8_t reg) {
  uint8_t b[3];
  if (InaWireReadRegister(addr7, reg, b, 3) != 3) return 0;
  return ((uint32_t)b[0] << 16) | ((uint32_t)b[1] << 8) | (uint32_t)b[2];
}

/** Quick I2C presence check (7-bit @p addr). Call after InaWireBeginMapped. */
inline bool InaWireProbeAddr(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

/** JSON ERR line if probe fails (visible when USB Serial / CDC works). */
inline void InaWireReportProbeFailure(uint8_t addr) {
  char addrStr[8];
  snprintf(addrStr, sizeof(addrStr), "0x%02X", addr);
  char err[200];
  snprintf(err, sizeof(err),
           "{\"v\":1,\"type\":\"ERR\",\"msg\":\"I2C no ACK at %s check SDA SCL pullups ADDR 3V3\","
           "\"addr\":\"%s\"}",
           addrStr, addrStr);
  Serial.println(err);
}
