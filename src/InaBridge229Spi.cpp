#include "InaBridge229Spi.h"
#include "InaJsonlProtocol.h"
#include "InaSpiCompat.h"
#include <stdio.h>

static const uint8_t REG_CONFIG = 0x00;
static const uint8_t REG_ADC_CONFIG = 0x01;
static const uint8_t REG_SHUNT_CAL = 0x02;
static const uint8_t REG_VBUS = 0x05;
static const uint8_t REG_CURRENT = 0x07;
static const uint8_t REG_POWER = 0x08;
static const uint8_t REG_MFG_ID = 0x3E;
static const uint16_t CFG_RST = 0x8000;
static const uint16_t ADC_MODE_MASK = 0xF000;
static const uint8_t MODE_CONT_TEMP_BUS_SHUNT = 0x0F;

InaBridge229Spi::InaBridge229Spi(const char* chipJson, const char* ref, int pinCs, int pinSck, int pinMiso,
                                 int pinMosi)
    : _chip(chipJson), _ref(ref), _cs(pinCs), _sck(pinSck), _miso(pinMiso), _mosi(pinMosi),
      _spiSettings(10000000, MSBFIRST, SPI_MODE1) {}

void InaBridge229Spi::beginSpi(uint32_t spiHz) {
  _spiSettings = SPISettings(spiHz, MSBFIRST, SPI_MODE1);
  InaSpiBeginMapped(_cs, _sck, _miso, _mosi);
  if (!detectChip()) {
    char e[180];
    snprintf(e, sizeof(e),
             "{\"v\":1,\"type\":\"ERR\",\"msg\":\"%s MFG ID mismatch (expected 0x5449 @0x3E)\"}", _chip);
    Serial.println(e);
    return;
  }
  applyResetAndMode();
  printInfo();
}

uint32_t InaBridge229Spi::spiReadRegister(uint8_t reg, uint8_t nbytes) {
  uint8_t addr = (uint8_t)((reg << 2) | 1);
  digitalWrite(_cs, LOW);
  SPI.beginTransaction(_spiSettings);
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

void InaBridge229Spi::spiWriteU16(uint8_t reg, uint16_t val) {
  uint8_t cmd = (uint8_t)(reg << 2);
  digitalWrite(_cs, LOW);
  SPI.beginTransaction(_spiSettings);
  SPI.transfer(cmd);
  SPI.transfer((uint8_t)(val >> 8));
  SPI.transfer((uint8_t)(val & 0xFF));
  SPI.endTransaction();
  digitalWrite(_cs, HIGH);
}

uint16_t InaBridge229Spi::readU16(uint8_t reg) {
  return (uint16_t)(spiReadRegister(reg, 2) & 0xFFFFu);
}

uint32_t InaBridge229Spi::readU24(uint8_t reg) {
  return spiReadRegister(reg, 3) & 0xFFFFFFu;
}

void InaBridge229Spi::applyShuntCalibration() {
  if (_rshuntOhm < 0.0001f) _rshuntOhm = 0.1f;
  if (_imaxA <= 0.0f) _imaxA = 10.0f;
  _currentLsb = _imaxA * 1.9073486328125e-6f;
  float shunt_cal = 13107.2e6f * _currentLsb * _rshuntOhm;
  if (_adcRangeHigh) shunt_cal *= 4.0f;
  if (shunt_cal > 65535.0f) shunt_cal = 65535.0f;
  if (shunt_cal < 1.0f) shunt_cal = 1.0f;
  spiWriteU16(REG_SHUNT_CAL, (uint16_t)shunt_cal);
}

bool InaBridge229Spi::detectChip() {
  return readU16(REG_MFG_ID) == 0x5449;
}

void InaBridge229Spi::applyResetAndMode() {
  uint16_t cfg = readU16(REG_CONFIG);
  spiWriteU16(REG_CONFIG, (uint16_t)(cfg | CFG_RST));
  delay(2);
  uint16_t adc = readU16(REG_ADC_CONFIG);
  adc = (uint16_t)((adc & ~ADC_MODE_MASK) | ((uint16_t)MODE_CONT_TEMP_BUS_SHUNT << 12));
  spiWriteU16(REG_ADC_CONFIG, adc);
  _adcRangeHigh = (readU16(REG_CONFIG) & 0x0010) != 0;
  applyShuntCalibration();
}

void InaBridge229Spi::printInfo() {
  char buf[300];
  snprintf(buf, sizeof(buf),
           "{\"v\":1,\"type\":\"INFO\",\"msg\":\"%s bridge ready\",\"author\":\"NiusRobotLab\",\"chip\":\"%s\","
           "\"addr\":\"SPI\",\"ref\":\"%s\"}",
           _chip, _chip, _ref);
  Serial.println(buf);
}

float InaBridge229Spi::readBusVoltage_V() {
  uint32_t raw = readU24(REG_VBUS);
  int32_t v = (int32_t)(raw >> 4);
  return (float)v * 195.3125e-6f;
}

float InaBridge229Spi::readCurrent_A() {
  uint32_t raw = readU24(REG_CURRENT);
  int32_t v = (int32_t)(raw >> 4);
  if (v & 0x80000) v |= 0xFFF00000;
  return (float)v * _currentLsb;
}

float InaBridge229Spi::readPower_W() {
  uint32_t raw = readU24(REG_POWER);
  return (float)raw * 3.2f * _currentLsb;
}

void InaBridge229Spi::sampleOnce() {
  const float bus_V = readBusVoltage_V();
  const float current_A = readCurrent_A();
  const float power_W = readPower_W();
  const uint32_t t_ms = millis();
  Serial.print("{\"v\":1,\"chip\":\"");
  Serial.print(_chip);
  Serial.print("\",\"addr\":\"SPI\",\"seq\":"); Serial.print(_seq++);
  Serial.print(",\"t_ms\":"); Serial.print(t_ms);
  Serial.print(",\"bus_V\":"); Serial.print(bus_V, 6);
  Serial.print(",\"current_A\":"); Serial.print(current_A, 6);
  Serial.print(",\"power_W\":"); Serial.print(power_W, 6);
  Serial.println("}");
}

void InaBridge229Spi::handleCommand(const String& line) {
  String l = line;
  l.trim();
  if (l.length() == 0) return;
  if (l.equalsIgnoreCase("PING")) {
    InaJsonl::pong();
    return;
  }
  if (l.equalsIgnoreCase("START")) {
    _streaming = true;
    InaJsonl::ackStart();
    return;
  }
  if (l.equalsIgnoreCase("STOP")) {
    _streaming = false;
    InaJsonl::ackStop();
    return;
  }
  if (l.startsWith("SR ")) {
    int hz = l.substring(3).toInt();
    if (hz < 1) hz = 1;
    if (hz > 200) hz = 200;
    _sampleHz = hz;
    InaJsonl::ackSr(_sampleHz);
    return;
  }
  if (l.startsWith("IMAX ")) {
    float a = l.substring(5).toFloat();
    if (a <= 0.0f) a = 10.0f;
    _imaxA = a;
    applyShuntCalibration();
    InaJsonl::ackImax(_imaxA);
    return;
  }
  if (l.startsWith("RSHUNT ")) {
    float r = l.substring(7).toFloat();
    if (r < 0.0001f) r = 0.1f;
    _rshuntOhm = r;
    applyShuntCalibration();
    InaJsonl::ackRshunt(_rshuntOhm);
    return;
  }
  InaJsonl::errUnknownCmd(l);
}

void InaBridge229Spi::tick() {
  if (Serial.available()) {
    handleCommand(Serial.readStringUntil('\n'));
  }
  const uint32_t now = millis();
  const uint32_t interval_ms = InaJsonl::sampleIntervalMs(_sampleHz);
  if (_streaming && (now - _lastMs >= interval_ms)) {
    _lastMs = now;
    sampleOnce();
  }
}
