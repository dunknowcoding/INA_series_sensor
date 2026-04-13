#include "InaBridge3221.h"
#include "InaJsonlProtocol.h"
#include "InaWireCompat.h"
#include <stdio.h>

static const uint8_t REG_CONFIG = 0x00;
static const uint8_t REG_SHUNT1 = 0x01;
static const uint8_t REG_BUS1 = 0x02;
static const uint8_t REG_SHUNT2 = 0x03;
static const uint8_t REG_BUS2 = 0x04;
static const uint8_t REG_SHUNT3 = 0x05;
static const uint8_t REG_BUS3 = 0x06;
static const uint16_t CONFIG_DEFAULT = 0x7127;

InaBridge3221::InaBridge3221(const char* chipJson, uint8_t i2cAddr) : _chip(chipJson), _addr(i2cAddr) {}

void InaBridge3221::beginI2c(int pinSda, int pinScl, uint32_t i2cHz) {
  InaWireBeginMapped(pinSda, pinScl, i2cHz);
  if (!InaWireProbeAddr(_addr)) {
    InaWireReportProbeFailure(_addr);
  }
  applyDefaultConfig();
  printInfo();
  Serial.flush();
}

uint16_t InaBridge3221::readU16(uint8_t reg) {
  return InaWireReadU16Reg(_addr, reg);
}

int16_t InaBridge3221::readS16(uint8_t reg) {
  return (int16_t)readU16(reg);
}

void InaBridge3221::writeU16(uint8_t reg, uint16_t val) {
  Wire.beginTransmission(_addr);
  Wire.write(reg);
  Wire.write((uint8_t)((val >> 8) & 0xFF));
  Wire.write((uint8_t)(val & 0xFF));
  Wire.endTransmission(true);
}

float InaBridge3221::shuntRegToVolts(int16_t raw) {
  return (float)raw * 40.0e-6f;
}

float InaBridge3221::busRegToVolts(uint16_t raw) {
  uint16_t shifted = (uint16_t)(raw >> 3);
  uint16_t masked = shifted & 0x1FFF;
  return (float)masked * 8.0e-3f;
}

void InaBridge3221::applyDefaultConfig() {
  writeU16(REG_CONFIG, CONFIG_DEFAULT);
  delay(10);
}

void InaBridge3221::printInfo() {
  char addrStr[8];
  snprintf(addrStr, sizeof(addrStr), "0x%02X", _addr);
  char buf[280];
  snprintf(buf, sizeof(buf),
           "{\"v\":1,\"type\":\"INFO\",\"msg\":\"%s bridge ready (3ch). No JSON samples until START (optional SR "
           "<Hz> first).\",\"author\":\"NiusRobotLab\",\"chip\":\"%s\","
           "\"addr\":\"%s\"}",
           _chip, _chip, addrStr);
  Serial.println(buf);
}

void InaBridge3221::sampleOnce() {
  const int16_t sh1 = readS16(REG_SHUNT1);
  const int16_t sh2 = readS16(REG_SHUNT2);
  const int16_t sh3 = readS16(REG_SHUNT3);
  const uint16_t bu1 = readU16(REG_BUS1);
  const uint16_t bu2 = readU16(REG_BUS2);
  const uint16_t bu3 = readU16(REG_BUS3);
  const float vsh1 = shuntRegToVolts(sh1);
  const float vsh2 = shuntRegToVolts(sh2);
  const float vsh3 = shuntRegToVolts(sh3);
  const float vb1 = busRegToVolts(bu1);
  const float vb2 = busRegToVolts(bu2);
  const float vb3 = busRegToVolts(bu3);
  const float r = (_rshuntOhm <= 0.0f) ? 0.1f : _rshuntOhm;
  const float i1 = vsh1 / r;
  const float i2 = vsh2 / r;
  const float i3 = vsh3 / r;
  const float p1 = i1 * vb1;
  const float p2 = i2 * vb2;
  const float p3 = i3 * vb3;
  const float bus_avg = (vb1 + vb2 + vb3) / 3.0f;
  const uint32_t t_ms = millis();
  char addrStr[8];
  snprintf(addrStr, sizeof(addrStr), "0x%02X", _addr);
  Serial.print("{\"v\":1,\"chip\":\"");
  Serial.print(_chip);
  Serial.print("\",\"addr\":\"");
  Serial.print(addrStr);
  Serial.print("\",\"seq\":"); Serial.print(_seq++);
  Serial.print(",\"t_ms\":"); Serial.print(t_ms);
  Serial.print(",\"bus_V\":"); Serial.print(bus_avg, 6);
  Serial.print(",\"channels\":[");
  Serial.print("{\"bus_V\":"); Serial.print(vb1, 6);
  Serial.print(",\"current_A\":"); Serial.print(i1, 6);
  Serial.print(",\"power_W\":"); Serial.print(p1, 6);
  Serial.print("},{\"bus_V\":"); Serial.print(vb2, 6);
  Serial.print(",\"current_A\":"); Serial.print(i2, 6);
  Serial.print(",\"power_W\":"); Serial.print(p2, 6);
  Serial.print("},{\"bus_V\":"); Serial.print(vb3, 6);
  Serial.print(",\"current_A\":"); Serial.print(i3, 6);
  Serial.print(",\"power_W\":"); Serial.print(p3, 6);
  Serial.println("}]}");
}

void InaBridge3221::handleCommand(const String& line) {
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
  if (l.startsWith("RSHUNT ")) {
    float rr = l.substring(7).toFloat();
    if (rr <= 0.0f) rr = 0.1f;
    _rshuntOhm = rr;
    InaJsonl::ackRshunt(_rshuntOhm);
    return;
  }
  InaJsonl::errUnknownCmd(l);
}

void InaBridge3221::tick() {
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
