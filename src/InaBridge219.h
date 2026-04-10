#pragma once

#include <Arduino.h>
#include <Wire.h>

/** INA219 / INA220 family: cal 0.04096, shunt_uV in JSON. */
class InaBridge219 {
public:
  InaBridge219(const char* chipJson, uint8_t i2cAddr = 0x40);

  /** Wire + config + calibration + INFO line. Serial must already be started if you use custom baud. */
  void beginI2c(int pinSda = 8, int pinScl = 9, uint32_t i2cHz = 400000);

  /** Call from loop: serial commands + streaming samples. */
  void tick();

private:
  const char* _chip;
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
