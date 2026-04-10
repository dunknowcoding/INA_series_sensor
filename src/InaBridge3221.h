#pragma once

#include <Arduino.h>
#include <Wire.h>

/** INA3221 triple-channel (SLYSF03). */
class InaBridge3221 {
public:
  InaBridge3221(const char* chipJson, uint8_t i2cAddr = 0x40);
  void beginI2c(int pinSda = 8, int pinScl = 9, uint32_t i2cHz = 400000);
  void tick();

private:
  const char* _chip;
  uint8_t _addr;
  float _rshuntOhm = 0.1f;
  int _sampleHz = 10;
  bool _streaming = false;
  uint32_t _seq = 0;
  uint32_t _lastMs = 0;

  uint16_t readU16(uint8_t reg);
  int16_t readS16(uint8_t reg);
  void writeU16(uint8_t reg, uint16_t val);
  static float shuntRegToVolts(int16_t raw);
  static float busRegToVolts(uint16_t raw);
  void applyDefaultConfig();
  void printInfo();
  void sampleOnce();
  void handleCommand(const String& line);
};
