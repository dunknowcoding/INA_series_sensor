/**
 * @file InaWireCompat.h
 * @brief Portable I2C (Wire) initialization for ESP32, RP2040 (Pico), and boards with fixed I2C pins.
 */
#pragma once

#include <Arduino.h>
#include <Wire.h>

/**
 * Start I2C at @p i2cHz. On ESP32 and RP2040 (Arduino-Pico), @p pinSda and @p pinScl select the bus pins.
 * On other architectures (e.g. AVR ATmega328 Uno/Nano), pins are ignored and the core uses the
 * board default I2C pins (hardware SDA/SCL).
 */
inline void InaWireBeginMapped(int pinSda, int pinScl, uint32_t i2cHz) {
#if defined(ARDUINO_ARCH_ESP32)
  Wire.begin(pinSda, pinScl);
#elif defined(ARDUINO_ARCH_RP2040)
  Wire.begin(pinSda, pinScl);
#else
  (void)pinSda;
  (void)pinScl;
  Wire.begin();
#endif
  Wire.setClock(i2cHz);
}
