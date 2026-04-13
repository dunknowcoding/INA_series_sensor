#include "InaBridge228.h"
#include "InaJsonlProtocol.h"
#include "InaWireCompat.h"
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

InaBridge228::InaBridge228(const char* chipJson, uint8_t i2cAddr, const char* ref)
    : _chip(chipJson), _ref(ref), _addr(i2cAddr) {}

void InaBridge228::beginI2c(int pinSda, int pinScl, uint32_t i2cHz) {
  InaWireBeginMapped(pinSda, pinScl, i2cHz);
  if (!InaWireProbeAddr(_addr)) {
    InaWireReportProbeFailure(_addr);
    return;
  }
  if (!detectChip()) {
    char e[160];
    snprintf(e, sizeof(e),
             "{\"v\":1,\"type\":\"ERR\",\"msg\":\"%s MFG ID mismatch (expected 0x5449 @0x3E)\"}", _chip);
    Serial.println(e);
    return;
  }
  applyResetAndMode();
  printInfo();
  Serial.flush();
}

uint16_t InaBridge228::readU16(uint8_t reg) {
  return InaWireReadU16Reg(_addr, reg);
}

void InaBridge228::writeU16(uint8_t reg, uint16_t val) {
  Wire.beginTransmission(_addr);
  Wire.write(reg);
  Wire.write((uint8_t)((val >> 8) & 0xFF));
  Wire.write((uint8_t)(val & 0xFF));
  Wire.endTransmission(true);
}

uint32_t InaBridge228::readU24(uint8_t reg) {
  return InaWireReadU24RegMsbFirst(_addr, reg);
}

void InaBridge228::applyShuntCalibration() {
  if (_rshuntOhm < 0.0001f) _rshuntOhm = 0.1f;
  if (_imaxA <= 0.0f) _imaxA = 10.0f;
  _currentLsb = _imaxA * 1.9073486328125e-6f;
  float shunt_cal = 13107.2e6f * _currentLsb * _rshuntOhm;
  if (_adcRangeHigh) shunt_cal *= 4.0f;
  if (shunt_cal > 65535.0f) shunt_cal = 65535.0f;
  if (shunt_cal < 1.0f) shunt_cal = 1.0f;
  writeU16(REG_SHUNT_CAL, (uint16_t)shunt_cal);
}

bool InaBridge228::detectChip() {
  return readU16(REG_MFG_ID) == 0x5449;
}

void InaBridge228::applyResetAndMode() {
  uint16_t cfg = readU16(REG_CONFIG);
  writeU16(REG_CONFIG, (uint16_t)(cfg | CFG_RST));
  delay(2);
  uint16_t adc = readU16(REG_ADC_CONFIG);
  adc = (uint16_t)((adc & ~ADC_MODE_MASK) | ((uint16_t)MODE_CONT_TEMP_BUS_SHUNT << 12));
  writeU16(REG_ADC_CONFIG, adc);
  _adcRangeHigh = (readU16(REG_CONFIG) & 0x0010) != 0;
  applyShuntCalibration();
}

void InaBridge228::printInfo() {
  char addrStr[8];
  snprintf(addrStr, sizeof(addrStr), "0x%02X", _addr);
  char buf[280];
  snprintf(buf, sizeof(buf),
           "{\"v\":1,\"type\":\"INFO\",\"msg\":\"%s bridge ready. No JSON samples until START (optional SR <Hz> "
           "first).\",\"author\":\"NiusRobotLab\",\"chip\":\"%s\","
           "\"addr\":\"%s\",\"ref\":\"%s\"}",
           _chip, _chip, addrStr, _ref);
  Serial.println(buf);
}

float InaBridge228::readBusVoltage_V() {
  uint32_t raw = readU24(REG_VBUS);
  int32_t v = (int32_t)(raw >> 4);
  return (float)v * 195.3125e-6f;
}

float InaBridge228::readCurrent_A() {
  uint32_t raw = readU24(REG_CURRENT);
  int32_t v = (int32_t)(raw >> 4);
  if (v & 0x80000) v |= 0xFFF00000;
  return (float)v * _currentLsb;
}

float InaBridge228::readPower_W() {
  uint32_t raw = readU24(REG_POWER);
  return (float)raw * 3.2f * _currentLsb;
}

void InaBridge228::sampleOnce() {
  const float bus_V = readBusVoltage_V();
  const float current_A = readCurrent_A();
  const float power_W = readPower_W();
  const uint32_t t_ms = millis();
  char addrStr[8];
  snprintf(addrStr, sizeof(addrStr), "0x%02X", _addr);
  Serial.print("{\"v\":1,\"chip\":\"");
  Serial.print(_chip);
  Serial.print("\",\"addr\":\"");
  Serial.print(addrStr);
  Serial.print("\",\"seq\":"); Serial.print(_seq++);
  Serial.print(",\"t_ms\":"); Serial.print(t_ms);
  Serial.print(",\"bus_V\":"); Serial.print(bus_V, 6);
  Serial.print(",\"current_A\":"); Serial.print(current_A, 6);
  Serial.print(",\"power_W\":"); Serial.print(power_W, 6);
  Serial.println("}");
}

void InaBridge228::handleCommand(const String& line) {
  String l = line;
  InaJsonl::normalizeCmd(l);
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
    _sampleHz = InaJsonl::clampStreamRateI2c(l.substring(3).toInt());
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

void InaBridge228::tick() {
  if (Serial.available()) {
    handleCommand(Serial.readStringUntil('\n'));
  }
  if (!_streaming) return;
  const uint32_t interval_ms = InaJsonl::sampleIntervalMs(_sampleHz);
  const uint32_t now = millis();
  if ((uint32_t)(now - _lastMs) >= interval_ms) {
    _lastMs = now;
    sampleOnce();
  }
}
