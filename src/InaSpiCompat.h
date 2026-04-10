/**
 * @file InaSpiCompat.h
 * @brief Portable SPI initialization: ESP32 / RP2040 use explicit pins; other cores use default SPI pins.
 */
#pragma once

#include <Arduino.h>
#include <SPI.h>

/** CS must be an output before transfer; this sets CS high then starts SPI. */
inline void InaSpiBeginMapped(int cs, int sck, int miso, int mosi) {
  pinMode(cs, OUTPUT);
  digitalWrite(cs, HIGH);
#if defined(ARDUINO_ARCH_ESP32)
  SPI.begin(sck, miso, mosi, cs);
#elif defined(ARDUINO_ARCH_RP2040)
  SPI.setRX(miso);
  SPI.setTX(mosi);
  SPI.setSCK(sck);
  SPI.begin(false);
#else
  (void)sck;
  (void)miso;
  (void)mosi;
  SPI.begin();
#endif
}
