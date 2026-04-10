#pragma once

#include <Arduino.h>
#include <Wire.h>

/** INA228 digital I2C family (INA228/237/238/239/740X…): MFG 0x5449 @0x3E. */
class InaBridge228 {
public:
  InaBridge228(const char* chipJson, uint8_t i2cAddr = 0x40, const char* ref = "TI INA228");
  void beginI2c(int pinSda = 8, int pinScl = 9, uint32_t i2cHz = 400000);
  void tick();

private:
  const char* _chip;
  const char* _ref;
  uint8_t _addr;
  float _rshuntOhm = 0.1f;
  float _imaxA = 10.0f;
  float _currentLsb = 0.0f;
  int _sampleHz = 10;
  bool _streaming = false;
  bool _adcRangeHigh = false;
  uint32_t _seq = 0;
  uint32_t _lastMs = 0;

  uint16_t readU16(uint8_t reg);
  void writeU16(uint8_t reg, uint16_t val);
  uint32_t readU24(uint8_t reg);
  void applyShuntCalibration();
  bool detectChip();
  void applyResetAndMode();
  void printInfo();
  float readBusVoltage_V();
  float readCurrent_A();
  float readPower_W();
  void sampleOnce();
  void handleCommand(const String& line);
};
