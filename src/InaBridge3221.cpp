#include "InaBridge3221.h"
#include "InaJsonlProtocol.h"
#include "InaWireCompat.h"
#include "ina/Ina3221Driver.h"
#include <stdio.h>

// INA3221 register addresses
static const uint8_t  REG_CONFIG      = 0x00;
static const uint8_t  REG_SHUNT1      = 0x01;
static const uint8_t  REG_BUS1        = 0x02;
static const uint8_t  REG_SHUNT2      = 0x03;
static const uint8_t  REG_BUS2        = 0x04;
static const uint8_t  REG_SHUNT3      = 0x05;
static const uint8_t  REG_BUS3        = 0x06;
static const uint8_t  REG_CRIT_CH1    = 0x07;
static const uint8_t  REG_WARN_CH1    = 0x08;
static const uint8_t  REG_CRIT_CH2    = 0x09;
static const uint8_t  REG_WARN_CH2    = 0x0A;
static const uint8_t  REG_CRIT_CH3    = 0x0B;
static const uint8_t  REG_WARN_CH3    = 0x0C;
static const uint8_t  REG_MASK_ENABLE = 0x0F;
static const uint8_t  REG_PV_UPPER    = 0x10;
static const uint8_t  REG_PV_LOWER    = 0x11;
static const uint16_t CONFIG_DEFAULT  = 0x7127;

static const uint8_t SHUNT_REGS[] = { REG_SHUNT1, REG_SHUNT2, REG_SHUNT3 };
static const uint8_t BUS_REGS[]   = { REG_BUS1,   REG_BUS2,   REG_BUS3   };

// ── Construction / Initialization ────────────────────────────────

InaBridge3221::InaBridge3221(const char* chipJson, uint8_t i2cAddr) : _chip(chipJson), _addr(i2cAddr) {}

void InaBridge3221::begin(int pinSda, int pinScl, uint32_t i2cHz) {
  InaWireBeginMapped(pinSda, pinScl, i2cHz);
  if (!InaWireProbeAddr(_addr)) {
    InaWireReportProbeFailure(_addr);
  }
  applyDefaultConfig();
  printInfo();
  Serial.flush();
}

// ── Low-level register I/O ───────────────────────────────────────

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
  uint16_t reg = (uint16_t)raw;
  int16_t steps = (int16_t)(reg >> 3);
  if ((steps & 0x1000) != 0) steps |= (int16_t)0xE000;
  return (float)steps * 40.0e-6f;                  // signed bits [15:3], 40 µV / LSB
}

float InaBridge3221::busRegToVolts(uint16_t raw) {
  return (float)((raw >> 3) & 0x1FFF) * 8.0e-3f;   // 8 mV / LSB
}

float InaBridge3221::rshuntSafe(uint8_t ch) const {
  if (ch < 1 || ch > 3) return 0.1f;
  float r = _rshuntOhm[ch - 1];
  return (r <= 0.0f) ? 0.1f : r;
}

// ── Configuration internals ──────────────────────────────────────

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

// ── Standalone Measurement API ───────────────────────────────────

float InaBridge3221::readBusVoltage(uint8_t ch) {
  if (ch < 1 || ch > 3) return 0.0f;
  return busRegToVolts(readU16(BUS_REGS[ch - 1]));
}

float InaBridge3221::readShuntVoltage(uint8_t ch) {
  if (ch < 1 || ch > 3) return 0.0f;
  return shuntRegToVolts(readS16(SHUNT_REGS[ch - 1]));
}

float InaBridge3221::readCurrent(uint8_t ch) {
  return readShuntVoltage(ch) / rshuntSafe(ch);
}

float InaBridge3221::readPower(uint8_t ch) {
  if (ch < 1 || ch > 3) return 0.0f;
  const float vbus = busRegToVolts(readU16(BUS_REGS[ch - 1]));
  const float vsh  = shuntRegToVolts(readS16(SHUNT_REGS[ch - 1]));
  return (vsh / rshuntSafe(ch)) * vbus;
}

bool InaBridge3221::dataReady() {
  return (readU16(REG_MASK_ENABLE) & 0x0001) != 0;
}

// ── Channel Control ──────────────────────────────────────────────

void InaBridge3221::enableChannel(uint8_t ch, bool enable) {
  if (ch < 1 || ch > 3) return;
  // CONFIG bits 14/13/12 correspond to CH1/CH2/CH3 enable
  const uint16_t bit = (uint16_t)(1u << (15 - ch));   // CH1→14, CH2→13, CH3→12
  uint16_t cfg = readU16(REG_CONFIG);
  if (enable) cfg |= bit;
  else        cfg &= ~bit;
  writeU16(REG_CONFIG, cfg);
}

bool InaBridge3221::isChannelEnabled(uint8_t ch) {
  if (ch < 1 || ch > 3) return false;
  const uint16_t bit = (uint16_t)(1u << (15 - ch));
  return (readU16(REG_CONFIG) & bit) != 0;
}

// ── Configuration API ────────────────────────────────────────────

void InaBridge3221::startStreaming()  { _streaming = true; }
void InaBridge3221::stopStreaming()   { _streaming = false; }

void InaBridge3221::setSampleRate(int hz) {
  _sampleHz = InaJsonl::clampStreamRateI2c(hz);
}

void InaBridge3221::setRshunt(uint8_t ch, float ohm) {
  if (ch < 1 || ch > 3) return;
  if (ohm <= 0.0f) ohm = 0.1f;
  _rshuntOhm[ch - 1] = ohm;
}

void InaBridge3221::setRshunt(float ohm) {
  if (ohm <= 0.0f) ohm = 0.1f;
  _rshuntOhm[0] = _rshuntOhm[1] = _rshuntOhm[2] = ohm;
}

float InaBridge3221::rshunt(uint8_t ch) const {
  if (ch < 1 || ch > 3) return 0.1f;
  return _rshuntOhm[ch - 1];
}

// ── JSONL streaming (NiusRobotLab_INA_monitor compatible) ────────

void InaBridge3221::emitJsonSample() {
  float vsh[3], vb[3], cur[3], pwr[3];
  for (uint8_t i = 0; i < 3; i++) {
    vsh[i] = shuntRegToVolts(readS16(SHUNT_REGS[i]));
    vb[i]  = busRegToVolts(readU16(BUS_REGS[i]));
    float r = rshuntSafe(i + 1);
    cur[i] = vsh[i] / r;
    pwr[i] = cur[i] * vb[i];
  }
  const float bus_avg = (vb[0] + vb[1] + vb[2]) / 3.0f;
  const uint32_t t_ms = millis();
  char addrStr[8];
  snprintf(addrStr, sizeof(addrStr), "0x%02X", _addr);

  Serial.print(F("{\"v\":1,\"chip\":\""));
  Serial.print(_chip);
  Serial.print(F("\",\"addr\":\""));
  Serial.print(addrStr);
  Serial.print(F("\",\"seq\":")); Serial.print(_seq++);
  Serial.print(F(",\"t_ms\":")); Serial.print(t_ms);
  Serial.print(F(",\"bus_V\":")); Serial.print(bus_avg, 6);
  Serial.print(F(",\"channels\":["));
  for (uint8_t i = 0; i < 3; i++) {
    if (i > 0) Serial.print(',');
    Serial.print(F("{\"bus_V\":")); Serial.print(vb[i], 6);
    Serial.print(F(",\"current_A\":")); Serial.print(cur[i], 6);
    Serial.print(F(",\"power_W\":")); Serial.print(pwr[i], 6);
    Serial.print('}');
  }
  Serial.print(']');
  if (_extraFields) _extraFields();
  Serial.println(F("}"));
}

void InaBridge3221::handleCommand(const String& line) {
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
  if (cmd.startsWith("RSHUNT ")) {
    float rr = cmd.substring(7).toFloat();
    if (rr <= 0.0f) rr = 0.1f;
    setRshunt(rr);
    InaJsonl::ackRshunt(rr);
    return;
  }

  if (cmd.equalsIgnoreCase("DIAG")) {
    const uint16_t m   = InaWireReadU16Reg(_addr, REG_MASK_ENABLE);
    const uint16_t pvU = InaWireReadU16Reg(_addr, REG_PV_UPPER);
    const uint16_t pvL = InaWireReadU16Reg(_addr, REG_PV_LOWER);
    char addrStr[8];
    snprintf(addrStr, sizeof(addrStr), "0x%02X", _addr);
    Serial.print(F("{\"v\":1,\"type\":\"DIAG\",\"chip\":\""));
    Serial.print(_chip);
    Serial.print(F("\",\"addr\":\""));
    Serial.print(addrStr);
    Serial.print(F("\",\"mask_enable\":")); Serial.print(m);
    Serial.print(F(",\"pv_upper_raw\":")); Serial.print(pvU);
    Serial.print(F(",\"pv_lower_raw\":")); Serial.print(pvL);
    Serial.println(F("}"));
    return;
  }

  if (cmd.startsWith("PV ")) {
    const int sp = cmd.indexOf(' ', 3);
    if (sp < 0) { InaJsonl::errUnknownCmd(cmd); return; }
    const float vmin = cmd.substring(3, sp).toFloat();
    const float vmax = cmd.substring(sp + 1).toFloat();
    writeU16(REG_PV_LOWER, Ina::Ina3221Driver::busVoltageToPvLimitRaw(vmin));
    writeU16(REG_PV_UPPER, Ina::Ina3221Driver::busVoltageToPvLimitRaw(vmax));
    Serial.print(F("{\"v\":1,\"type\":\"ACK\",\"cmd\":\"PV\",\"vmin_V\":"));
    Serial.print(vmin, 6);
    Serial.print(F(",\"vmax_V\":"));
    Serial.print(vmax, 6);
    Serial.println(F("}"));
    return;
  }

  if (cmd.startsWith("CRIT ")) {
    const int sp = cmd.indexOf(' ', 5);
    if (sp < 0) { InaJsonl::errUnknownCmd(cmd); return; }
    const int ch    = cmd.substring(5, sp).toInt();
    const float a   = cmd.substring(sp + 1).toFloat();
    const float uV  = (a * rshuntSafe(ch)) * 1.0e6f;
    const uint16_t raw = Ina::Ina3221Driver::shuntVoltageToLimitRaw_uV(uV);
    const uint8_t reg = (ch == 1) ? REG_CRIT_CH1 : (ch == 2) ? REG_CRIT_CH2 : REG_CRIT_CH3;
    writeU16(reg, raw);
    uint16_t me = readU16(REG_MASK_ENABLE);
    me |= (uint16_t)(1u << 10);
    writeU16(REG_MASK_ENABLE, me);
    Serial.print(F("{\"v\":1,\"type\":\"ACK\",\"cmd\":\"CRIT\",\"ch\":"));
    Serial.print(ch);
    Serial.print(F(",\"A\":")); Serial.print(a, 6);
    Serial.println(F("}"));
    return;
  }

  if (cmd.startsWith("WARN ")) {
    const int sp = cmd.indexOf(' ', 5);
    if (sp < 0) { InaJsonl::errUnknownCmd(cmd); return; }
    const int ch    = cmd.substring(5, sp).toInt();
    const float a   = cmd.substring(sp + 1).toFloat();
    const float uV  = (a * rshuntSafe(ch)) * 1.0e6f;
    const uint16_t raw = Ina::Ina3221Driver::shuntVoltageToLimitRaw_uV(uV);
    const uint8_t reg = (ch == 1) ? REG_WARN_CH1 : (ch == 2) ? REG_WARN_CH2 : REG_WARN_CH3;
    writeU16(reg, raw);
    uint16_t me = readU16(REG_MASK_ENABLE);
    me |= (uint16_t)(1u << 11);
    writeU16(REG_MASK_ENABLE, me);
    Serial.print(F("{\"v\":1,\"type\":\"ACK\",\"cmd\":\"WARN\",\"ch\":"));
    Serial.print(ch);
    Serial.print(F(",\"A\":")); Serial.print(a, 6);
    Serial.println(F("}"));
    return;
  }

  if (cmd.equalsIgnoreCase("TC")) {
    const uint16_t me = readU16(REG_MASK_ENABLE);
    Serial.print(F("{\"v\":1,\"type\":\"DIAG\",\"cmd\":\"TC\",\"tcf\":"));
    Serial.print((me & (uint16_t)(1u << 1)) ? 1 : 0);
    Serial.println(F("}"));
    return;
  }

  InaJsonl::errUnknownCmd(cmd);
}

void InaBridge3221::tick() {
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
