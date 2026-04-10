#include "InaBridge219.h"
#include "InaJsonlProtocol.h"
#include "InaWireCompat.h"
#include <stdio.h>

static const uint8_t REG_CONFIG = 0x00;
static const uint8_t REG_SHUNT_V = 0x01;
static const uint8_t REG_BUS_V = 0x02;
static const uint8_t REG_POWER = 0x03;
static const uint8_t REG_CURRENT = 0x04;
static const uint8_t REG_CALIB = 0x05;

InaBridge219::InaBridge219(const char* chipJson, uint8_t i2cAddr) : _chip(chipJson), _addr(i2cAddr) {}

void InaBridge219::beginI2c(int pinSda, int pinScl, uint32_t i2cHz) {
  InaWireBeginMapped(pinSda, pinScl, i2cHz);
  applyDefaultConfig();
  applyCalibration();
  printInfo();
}

uint16_t InaBridge219::readU16(uint8_t reg) {
  Wire.beginTransmission(_addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return 0;
  Wire.requestFrom((int)_addr, 2);
  uint16_t hi = Wire.read();
  uint16_t lo = Wire.read();
  return (hi << 8) | lo;
}

int16_t InaBridge219::readS16(uint8_t reg) {
  return (int16_t)readU16(reg);
}

void InaBridge219::writeU16(uint8_t reg, uint16_t val) {
  Wire.beginTransmission(_addr);
  Wire.write(reg);
  Wire.write((uint8_t)((val >> 8) & 0xFF));
  Wire.write((uint8_t)(val & 0xFF));
  Wire.endTransmission(true);
}

void InaBridge219::applyCalibration() {
  if (_rshuntOhm <= 0.0f) _rshuntOhm = 0.1f;
  if (_imaxA <= 0.0f) _imaxA = 3.2f;
  const float current_LSB = _imaxA / 32768.0f;
  const float cal_f = 0.04096f / (current_LSB * _rshuntOhm);
  uint16_t cal = (uint16_t)(cal_f);
  if (cal == 0) cal = 1;
  writeU16(REG_CALIB, cal);
}

void InaBridge219::applyDefaultConfig() {
  writeU16(REG_CONFIG, 0x399F);
}

void InaBridge219::printInfo() {
  char addrStr[8];
  snprintf(addrStr, sizeof(addrStr), "0x%02X", _addr);
  char buf[220];
  snprintf(buf, sizeof(buf),
                "{\"v\":1,\"type\":\"INFO\",\"msg\":\"%s bridge ready\",\"author\":\"NiusRobotLab\",\"addr\":\"%s\"}",
                _chip, addrStr);
  Serial.println(buf);
}

void InaBridge219::sampleOnce() {
  const int16_t shunt_raw = readS16(REG_SHUNT_V);
  const uint16_t bus_raw = readU16(REG_BUS_V);
  const int16_t current_raw = readS16(REG_CURRENT);
  const uint16_t power_raw = readU16(REG_POWER);
  const float shunt_uV = (float)shunt_raw * 10.0f;
  const uint16_t bus_mv = (bus_raw >> 3) * 4;
  const float bus_V = (float)bus_mv / 1000.0f;
  const float current_LSB = _imaxA / 32768.0f;
  const float current_A = (float)current_raw * current_LSB;
  const float power_LSB = 20.0f * current_LSB;
  const float power_W = (float)power_raw * power_LSB;
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
  Serial.print(",\"shunt_uV\":"); Serial.print(shunt_uV, 1);
  Serial.print(",\"current_A\":"); Serial.print(current_A, 6);
  Serial.print(",\"power_W\":"); Serial.print(power_W, 6);
  Serial.println("}");
}

void InaBridge219::handleCommand(const String& line) {
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
    if (a <= 0.0f) a = 3.2f;
    _imaxA = a;
    applyCalibration();
    InaJsonl::ackImax(_imaxA);
    return;
  }
  if (l.startsWith("RSHUNT ")) {
    float r = l.substring(7).toFloat();
    if (r <= 0.0f) r = 0.1f;
    _rshuntOhm = r;
    applyCalibration();
    InaJsonl::ackRshunt(_rshuntOhm);
    return;
  }
  InaJsonl::errUnknownCmd(l);
}

void InaBridge219::tick() {
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
