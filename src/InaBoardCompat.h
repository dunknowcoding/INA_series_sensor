/**
 * @file InaBoardCompat.h
 * @brief Nordic nRF52 / nRF53 and related board-package detection for portable I2C/SPI setup.
 */
#pragma once

#include <Arduino.h>

/**
 * Cores where Wire.begin(sda, scl) pin remapping is supported and used by InaWireBeginMapped().
 *
 * - ESP32 Arduino
 * - ArduinoNRF (ARDUINO_ARCH_NRF52)
 * - Adafruit nRF52 BSP (same macro + ARDUINO_NRF52_ADAFRUIT_FEATHER)
 * - Arduino Mbed Nano 33 BLE (ARDUINO_NANO33BLE / ARDUINO_ARCH_NRF52840)
 * - nRF5340 application targets (ARDUINO_ARCH_NRF53)
 */
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_NRF52) || defined(ARDUINO_ARCH_NRF52840) \
    || defined(ARDUINO_ARCH_NRF53) || defined(ARDUINO_NRF52_ADAFRUIT_FEATHER) \
    || defined(ARDUINO_NANO33BLE)
#define INA_WIRE_SUPPORTS_PIN_REMAP 1
#else
#define INA_WIRE_SUPPORTS_PIN_REMAP 0
#endif

#if defined(ARDUINO_ARCH_NRF53) || defined(NRF5340_XXAA) || defined(NRF5340_XXAA_APPLICATION) \
    || defined(ARDUINO_NRF5340_XXAA)
#define INA_BOARD_IS_NRF53 1
#else
#define INA_BOARD_IS_NRF53 0
#endif

#if defined(ARDUINO_ARCH_NRF52) || defined(ARDUINO_ARCH_NRF52840) \
    || defined(ARDUINO_NRF52_ADAFRUIT_FEATHER) || defined(ARDUINO_NANO33BLE) \
    || defined(NRF52840_XXAA)
#define INA_BOARD_IS_NRF52 1
#else
#define INA_BOARD_IS_NRF52 0
#endif

inline bool InaBoardIsNrf() {
#if INA_BOARD_IS_NRF52 || INA_BOARD_IS_NRF53
  return true;
#else
  return false;
#endif
}
