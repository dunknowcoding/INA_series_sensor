#include "InaBridgeCh1.h"
#include "InaJsonlProtocol.h"
#include "InaWireCompat.h"
#include <stdio.h>

static const uint8_t REG_CONFIG = 0x00;
static const uint8_t REG_SHUNT1 = 0x01;
static const uint8_t REG_BUS1 = 0x02;
static const uint16_t CONFIG_DEFAULT = 0x7127;

InaBridgeCh1::InaBridgeCh1(const char* chipJson, const char* infoMsg, uint8_t i2cAddr)
    : _chip(chipJson), _infoMsg(infoMsg), _addr(i2cAddr) {}

void InaBridgeCh1::beginI2c(int pinSda, int pinScl, uint32_t i2cHz) {
  InaWireBeginMapped(pinSda, pinScl, i2cHz);
  if (!InaWireProbeAddr(_addr)) {
    InaWireReportProbeFailure(_addr);
  }
  applyDefaultConfig();
  printInfo();
  Serial.flush();
}

uint16_t InaBridgeCh1::readU16(uint8_t reg) {
  return InaWireReadU16Reg(_addr, reg);
}

int16_t InaBridgeCh1::readS16(uint8_t reg) {
  return (int16_t)readU16(reg);
}

void InaBridgeCh1::writeU16(uint8_t reg, uint16_t val) {
  Wire.beginTransmission(_addr);
  Wire.write(reg);
  Wire.write((uint8_t)((val >> 8) & 0xFF));
  Wire.write((uint8_t)(val & 0xFF));
  Wire.endTransmission(true);
}

float InaBridgeCh1::shuntRegToVolts(int16_t raw) {
  return (float)raw * 40.0e-6f;
}

float InaBridgeCh1::busRegToVolts(uint16_t raw) {
  uint16_t shifted = (uint16_t)(raw >> 3);
  uint16_t masked = shifted & 0x1FFF;
  return (float)masked * 8.0e-3f;
}

void InaBridgeCh1::applyDefaultConfig() {
  writeU16(REG_CONFIG, CONFIG_DEFAULT);
  delay(10);
}

void InaBridgeCh1::printInfo() {
  char addrStr[8];
  snprintf(addrStr, sizeof(addrStr), "0x%02X", _addr);
  char buf[300];
  snprintf(buf, sizeof(buf),
           "{\"v\":1,\"type\":\"INFO\",\"msg\":\"%s No JSON samples until START (optional SR <Hz> first).\","
           "\"author\":\"NiusRobotLab\",\"chip\":\"%s\",\"addr\":\"%s\"}",
           _infoMsg, _chip, addrStr);
  Serial.println(buf);
}

void InaBridgeCh1::sampleOnce() {
  const int16_t sh = readS16(REG_SHUNT1);
  const uint16_t bu = readU16(REG_BUS1);
  const float vsh = shuntRegToVolts(sh);
  const float vb = busRegToVolts(bu);
  const float r = (_rshuntOhm <= 0.0f) ? 0.1f : _rshuntOhm;
  const float current_A = vsh / r;
  const float power_W = current_A * vb;
  const uint32_t t_ms = millis();
  char addrStr[8];
  snprintf(addrStr, sizeof(addrStr), "0x%02X", _addr);
  Serial.print("{\"v\":1,\"chip\":\"");
  Serial.print(_chip);
  Serial.print("\",\"addr\":\"");
  Serial.print(addrStr);
  Serial.print("\",\"seq\":"); Serial.print(_seq++);
  Serial.print(",\"t_ms\":"); Serial.print(t_ms);
  Serial.print(",\"bus_V\":"); Serial.print(vb, 6);
  Serial.print(",\"current_A\":"); Serial.print(current_A, 6);
  Serial.print(",\"power_W\":"); Serial.print(power_W, 6);
  Serial.println("}");
}

void InaBridgeCh1::handleCommand(const String& line) {
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

void InaBridgeCh1::tick() {
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
