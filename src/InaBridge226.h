#pragma once

#include <Arduino.h>
#include <Wire.h>

/** INA226 family (SLYSF02): cal 0.00512, shunt_uV in JSON. */
class InaBridge226 {
public:
  InaBridge226(const char* chipJson, uint8_t i2cAddr = 0x40, const char* ref = "TI SLYSF02");
  void beginI2c(int pinSda = 8, int pinScl = 9, uint32_t i2cHz = 400000);
  void tick();

private:
  const char* _chip;
  const char* _ref;
  uint8_t _addr;
  float _rshuntOhm = 0.1f;
  float _imaxA = 3.2f;
  int _sampleHz = 10;
  bool _streaming = false;
  uint32_t _seq = 0;
  uint32_t _lastMs = 0;

  uint16_t readU16(uint8_t reg);
  int16_t readS16(uint8_t reg);
  void writeU16(uint8_t reg, uint16_t val);
  void applyCalibration();
  void applyDefaultConfig();
  void printInfo();
  void sampleOnce();
  void handleCommand(const String& line);
};
