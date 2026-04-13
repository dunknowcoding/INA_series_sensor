#pragma once

#include <Arduino.h>
#include <SPI.h>

/** INA229 / INA229-Q1 — SPI only (same math as INA228 I2C). */
class InaBridge229Spi {
public:
  InaBridge229Spi(const char* chipJson, const char* ref, int pinCs = 7, int pinSck = 4, int pinMiso = 5,
                  int pinMosi = 6);
  void beginSpi(uint32_t spiHz = 10000000);
  void tick();

private:
  const char* _chip;
  const char* _ref;
  int _cs, _sck, _miso, _mosi;
  float _rshuntOhm = 0.1f;
  float _imaxA = 10.0f;
  float _currentLsb = 0.0f;
  int _sampleHz = 10;
  bool _streaming = false;
  bool _adcRangeHigh = false;
  uint32_t _seq = 0;
  uint32_t _lastSampleUs = 0;
  SPISettings _spiSettings;

  uint32_t spiReadRegister(uint8_t reg, uint8_t nbytes);
  void spiWriteU16(uint8_t reg, uint16_t val);
  uint16_t readU16(uint8_t reg);
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
