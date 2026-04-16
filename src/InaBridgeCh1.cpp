#include "InaBridgeCh1.h"
#include "InaJsonlProtocol.h"
#include "InaWireCompat.h"
#include <stdio.h>

static const uint8_t REG_CONFIG = 0x00;
static const uint8_t REG_SHUNT1 = 0x01;
static const uint8_t REG_BUS1 = 0x02;
static const uint16_t CONFIG_DEFAULT = 0x7127;

// ── Construction / Initialization ────────────────────────────────

InaBridgeCh1::InaBridgeCh1(const char* chipJson, const char* infoMsg, uint8_t i2cAddr)
    : _chip(chipJson), _infoMsg(infoMsg), _addr(i2cAddr) {}

void InaBridgeCh1::begin(int pinSda, int pinScl, uint32_t i2cHz) {
  InaWireBeginMapped(pinSda, pinScl, i2cHz);
  if (!InaWireProbeAddr(_addr)) {
    InaWireReportProbeFailure(_addr);
  }
  applyDefaultConfig();
  printInfo();
  Serial.flush();
}

// ── Low-level register I/O ───────────────────────────────────────

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

// ── Register → physical conversion ──────────────────────────────

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

// ── Standalone Measurement API ───────────────────────────────────

float InaBridgeCh1::readBusVoltage() {
  return busRegToVolts(readU16(REG_BUS1));
}

float InaBridgeCh1::readShuntVoltage() {
  return shuntRegToVolts(readS16(REG_SHUNT1));
}

float InaBridgeCh1::readCurrent() {
  const float vsh = readShuntVoltage();
  const float r = (_rshuntOhm <= 0.0f) ? 0.1f : _rshuntOhm;
  return vsh / r;
}

float InaBridgeCh1::readPower() {
  const float vb = readBusVoltage();
  const float i  = readCurrent();
  return i * vb;
}

// ── Configuration API ────────────────────────────────────────────

void InaBridgeCh1::startStreaming() { _streaming = true; }
void InaBridgeCh1::stopStreaming()  { _streaming = false; }

void InaBridgeCh1::setSampleRate(int hz) {
  _sampleHz = InaJsonl::clampStreamRateI2c(hz);
}

void InaBridgeCh1::setRshunt(float ohm) {
  if (ohm <= 0.0f) ohm = 0.1f;
  _rshuntOhm = ohm;
}

// ── JSONL streaming (NiusRobotLab_INA_monitor compatible) ────────

void InaBridgeCh1::emitJsonSample() {
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
  Serial.print(F("{\"v\":1,\"chip\":\""));
  Serial.print(_chip);
  Serial.print(F("\",\"addr\":\""));
  Serial.print(addrStr);
  Serial.print(F("\",\"seq\":")); Serial.print(_seq++);
  Serial.print(F(",\"t_ms\":")); Serial.print(t_ms);
  Serial.print(F(",\"bus_V\":")); Serial.print(vb, 6);
  Serial.print(F(",\"current_A\":")); Serial.print(current_A, 6);
  Serial.print(F(",\"power_W\":")); Serial.print(power_W, 6);
  Serial.println(F("}"));
}

// ── Serial command handling ──────────────────────────────────────

void InaBridgeCh1::handleCommand(const String& line) {
  String cmd = line;
  InaJsonl::normalizeCmd(cmd);
  if (cmd.length() == 0) return;
  if (cmd.equalsIgnoreCase("PING")) {
    InaJsonl::pong();
    return;
  }
  if (cmd.equalsIgnoreCase("START")) {
    _streaming = true;
    InaJsonl::ackStart();
    return;
  }
  if (cmd.equalsIgnoreCase("STOP")) {
    _streaming = false;
    InaJsonl::ackStop();
    return;
  }
  if (cmd.startsWith("SR ")) {
    _sampleHz = InaJsonl::clampStreamRateI2c(cmd.substring(3).toInt());
    InaJsonl::ackSr(_sampleHz);
    return;
  }
  if (cmd.startsWith("RSHUNT ")) {
    float rr = cmd.substring(7).toFloat();
    if (rr <= 0.0f) rr = 0.1f;
    _rshuntOhm = rr;
    InaJsonl::ackRshunt(_rshuntOhm);
    return;
  }
  InaJsonl::errUnknownCmd(cmd);
}

void InaBridgeCh1::tick() {
  String line;
  if (_rx.pollLine(line)) {
    handleCommand(line);
  }
  if (!_streaming) return;
  const uint32_t interval_ms = InaJsonl::sampleIntervalMs(_sampleHz);
  const uint32_t now = millis();
  if ((uint32_t)(now - _lastMs) >= interval_ms) {
    _lastMs = now;
    emitJsonSample();
  }
}
