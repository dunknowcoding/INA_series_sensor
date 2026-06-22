#include "InaBridge226.h"
#include "InaJsonlProtocol.h"
#include "InaWireCompat.h"
#include "ina/Ina226Driver.h"
#include <stdio.h>
#include "InaMathCompat.h"

// INA226 register addresses
static const uint8_t REG_CONFIG      = 0x00;
static const uint8_t REG_SHUNT_V     = 0x01;
static const uint8_t REG_BUS_V       = 0x02;
static const uint8_t REG_POWER       = 0x03;
static const uint8_t REG_CURRENT     = 0x04;
static const uint8_t REG_CALIB       = 0x05;
static const uint8_t REG_MASK_ENABLE = 0x06;
static const uint8_t REG_ALERT_LIMIT = 0x07;

// ── Construction / Initialization ────────────────────────────────

InaBridge226::InaBridge226(const char* chipJson, uint8_t i2cAddr, const char* ref)
    : _chip(chipJson), _ref(ref), _addr(i2cAddr) {}

void InaBridge226::begin(int pinSda, int pinScl, uint32_t i2cHz) {
  InaWireBeginMapped(pinSda, pinScl, i2cHz);
  if (!InaWireProbeAddr(_addr)) {
    InaWireReportProbeFailure(_addr);
  }
  applyDefaultConfig();
  applyCalibration();
  printInfo();
  Serial.flush();
}

// ── Low-level register I/O ───────────────────────────────────────

uint16_t InaBridge226::readU16(uint8_t reg) {
  return InaWireReadU16Reg(_addr, reg);
}

int16_t InaBridge226::readS16(uint8_t reg) {
  return (int16_t)readU16(reg);
}

void InaBridge226::writeU16(uint8_t reg, uint16_t val) {
  Wire.beginTransmission(_addr);
  Wire.write(reg);
  Wire.write((uint8_t)((val >> 8) & 0xFF));
  Wire.write((uint8_t)(val & 0xFF));
  Wire.endTransmission(true);
}

// ── Calibration ──────────────────────────────────────────────────

void InaBridge226::applyCalibration() {
  if (_rshuntOhm <= 0.0f) _rshuntOhm = 0.1f;
  if (_imaxA <= 0.0f) _imaxA = 3.2f;
  const float current_LSB = _imaxA / 32768.0f;
  const float cal_f = 0.00512f / (current_LSB * _rshuntOhm);
  uint16_t cal = (uint16_t)(cal_f);
  if (cal == 0) cal = 1;
  writeU16(REG_CALIB, cal);
}

void InaBridge226::applyDefaultConfig() {
  writeU16(REG_CONFIG, 0x4127);
}

void InaBridge226::printInfo() {
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

// ── Standalone Measurement API ───────────────────────────────────

float InaBridge226::readBusVoltage() {
  const uint16_t raw = readU16(REG_BUS_V);
  return (float)raw * 0.00125f;
}

float InaBridge226::readShuntVoltage() {
  const int16_t raw = readS16(REG_SHUNT_V);
  return (float)raw * 2.5e-6f;
}

float InaBridge226::readCurrent() {
  const int16_t raw = readS16(REG_CURRENT);
  return (float)raw * (_imaxA / 32768.0f);
}

float InaBridge226::readPower() {
  const uint16_t raw = readU16(REG_POWER);
  return (float)raw * 25.0f * (_imaxA / 32768.0f);
}

bool InaBridge226::dataReady() {
  return (readU16(REG_MASK_ENABLE) & 0x0008) != 0;
}

// ── Configuration API ────────────────────────────────────────────

void InaBridge226::startStreaming() { _streaming = true; }
void InaBridge226::stopStreaming()  { _streaming = false; }

void InaBridge226::setSampleRate(int hz) {
  _sampleHz = InaJsonl::clampStreamRateI2c(hz);
}

void InaBridge226::setRshunt(float ohm) {
  if (ohm <= 0.0f) ohm = 0.1f;
  _rshuntOhm = ohm;
  applyCalibration();
}

void InaBridge226::setImax(float ampere) {
  if (ampere <= 0.0f) ampere = 3.2f;
  _imaxA = ampere;
  applyCalibration();
}

// ── JSONL streaming (NiusRobotLab_INA_monitor compatible) ────────

void InaBridge226::emitJsonSample() {
  const int16_t shunt_raw = readS16(REG_SHUNT_V);
  const uint16_t bus_raw = readU16(REG_BUS_V);
  const int16_t current_raw = readS16(REG_CURRENT);
  const uint16_t power_raw = readU16(REG_POWER);
  const float shunt_uV = (float)shunt_raw * 2.5f;
  const float bus_V = (float)bus_raw * 0.00125f;
  const float current_LSB = _imaxA / 32768.0f;
  const float current_A = (float)current_raw * current_LSB;
  const float power_LSB = 25.0f * current_LSB;
  const float power_W = (float)power_raw * power_LSB;
  const uint32_t t_ms = millis();
  char addrStr[8];
  snprintf(addrStr, sizeof(addrStr), "0x%02X", _addr);
  Serial.print(F("{\"v\":1,\"chip\":\""));
  Serial.print(_chip);
  Serial.print(F("\",\"addr\":\""));
  Serial.print(addrStr);
  Serial.print(F("\",\"seq\":")); Serial.print(_seq++);
  Serial.print(F(",\"t_ms\":")); Serial.print(t_ms);
  Serial.print(F(",\"bus_V\":")); Serial.print(bus_V, 6);
  Serial.print(F(",\"shunt_uV\":")); Serial.print(shunt_uV, 1);
  Serial.print(F(",\"current_A\":")); Serial.print(current_A, 6);
  Serial.print(F(",\"power_W\":")); Serial.print(power_W, 6);
  Serial.println(F("}"));
}

void InaBridge226::handleCommand(const String& line) {
  String cmd = line;
  InaJsonl::normalizeCmd(cmd);
  if (cmd.length() == 0) return;

  if (cmd.equalsIgnoreCase("PING"))  { InaJsonl::pong(); return; }
  if (cmd.equalsIgnoreCase("START")) { _streaming = true;  InaJsonl::ackStart(); return; }
  if (cmd.equalsIgnoreCase("STOP"))  { _streaming = false; InaJsonl::ackStop();  return; }

  if (cmd.startsWith("SR ")) {
    _sampleHz = InaJsonl::clampStreamRateI2c(cmd.substring(3).toInt());
    InaJsonl::ackSr(_sampleHz);
    return;
  }
  if (cmd.startsWith("IMAX ")) {
    float a = cmd.substring(5).toFloat();
    if (a <= 0.0f) a = 3.2f;
    _imaxA = a;
    applyCalibration();
    InaJsonl::ackImax(_imaxA);
    return;
  }
  if (cmd.startsWith("RSHUNT ")) {
    float r = cmd.substring(7).toFloat();
    if (r <= 0.0f) r = 0.1f;
    _rshuntOhm = r;
    applyCalibration();
    InaJsonl::ackRshunt(_rshuntOhm);
    return;
  }

  if (cmd.equalsIgnoreCase("DIAG")) {
    const uint16_t me = readU16(REG_MASK_ENABLE);
    const uint16_t lim = readU16(REG_ALERT_LIMIT);
    char addrStr[8];
    snprintf(addrStr, sizeof(addrStr), "0x%02X", _addr);
    Serial.print(F("{\"v\":1,\"type\":\"DIAG\",\"chip\":\""));
    Serial.print(_chip);
    Serial.print(F("\",\"addr\":\""));
    Serial.print(addrStr);
    Serial.print(F("\",\"mask_enable\":"));
    Serial.print(me);
    Serial.print(F(",\"alert_limit\":"));
    Serial.print(lim);
    Serial.println(F("}"));
    return;
  }

  if (cmd.startsWith("ALERT ")) {
    String rest = cmd.substring(6);
    rest.trim();
    bool latch = false;
    bool pol = false;
    int pLatch = rest.indexOf("LATCH");
    if (pLatch >= 0) {
      latch = rest.substring(pLatch + 5).toInt() != 0;
      rest = rest.substring(0, pLatch);
      rest.trim();
    }
    int pPol = rest.indexOf("POL");
    if (pPol >= 0) {
      pol = rest.substring(pPol + 3).toInt() != 0;
      rest = rest.substring(0, pPol);
      rest.trim();
    }
    if (rest.equalsIgnoreCase("OFF")) {
      writeU16(REG_MASK_ENABLE, 0);
      Serial.println(F("{\"v\":1,\"type\":\"ACK\",\"cmd\":\"ALERT\",\"mode\":\"OFF\"}"));
      return;
    }
    uint16_t me = 0;
    if (rest.equalsIgnoreCase("CNVR")) {
      me = 0x0400;
    } else if (rest.startsWith("BOV ")) {
      const float v = rest.substring(4).toFloat();
      writeU16(REG_ALERT_LIMIT, (uint16_t)(v * 800.0f + 0.5f));
      me = 0x2000;
    } else if (rest.startsWith("BUV ")) {
      const float v = rest.substring(4).toFloat();
      writeU16(REG_ALERT_LIMIT, (uint16_t)(v * 800.0f + 0.5f));
      me = 0x1000;
    } else if (rest.startsWith("SOV ")) {
      const float uV = rest.substring(4).toFloat();
      writeU16(REG_ALERT_LIMIT, (uint16_t)(fabsf(uV) / 2.5f + 0.5f));
      me = 0x8000;
    } else {
      InaJsonl::errUnknownCmd(cmd);
      return;
    }
    if (latch) me |= 0x0002;
    if (pol) me |= 0x0001;
    writeU16(REG_MASK_ENABLE, me);
    Serial.println(F("{\"v\":1,\"type\":\"ACK\",\"cmd\":\"ALERT\"}"));
    return;
  }

  InaJsonl::errUnknownCmd(cmd);
}

void InaBridge226::tick() {
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
