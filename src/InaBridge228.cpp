#include "InaBridge228.h"
#include "InaJsonlProtocol.h"
#include "InaWireCompat.h"
#include "ina/Ina228Driver.h"
#include <stdio.h>

// INA228 register addresses
static const uint8_t REG_CONFIG    = 0x00;
static const uint8_t REG_ADC_CFG   = 0x01;
static const uint8_t REG_SHUNT_CAL = 0x02;
static const uint8_t REG_VSHUNT    = 0x04;
static const uint8_t REG_VBUS      = 0x05;
static const uint8_t REG_DIETEMP   = 0x06;
static const uint8_t REG_CURRENT   = 0x07;
static const uint8_t REG_POWER     = 0x08;
static const uint8_t REG_ENERGY    = 0x09;
static const uint8_t REG_CHARGE    = 0x0A;
static const uint8_t REG_DIAG_ALRT = 0x0B;
static const uint8_t REG_SOVL      = 0x0C;
static const uint8_t REG_BOVL      = 0x0E;
static const uint8_t REG_BUVL      = 0x0F;
static const uint8_t REG_PWR_LIMIT = 0x11;
static const uint8_t REG_MFG_ID    = 0x3E;

static const uint16_t CFG_RST      = 0x8000;
static const uint16_t CFG_RSTACC   = 0x4000;
static const uint16_t ADC_MODE_MASK = 0xF000;
static const uint8_t  MODE_CONT_TEMP_BUS_SHUNT = 0x0F;
static const uint16_t DIAG_CNVRF   = 0x0002;

// ── Construction / Initialization ────────────────────────────────

InaBridge228::InaBridge228(const char* chipJson, uint8_t i2cAddr, const char* ref)
    : _chip(chipJson), _ref(ref), _addr(i2cAddr) {}

void InaBridge228::begin(int pinSda, int pinScl, uint32_t i2cHz) {
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

// ── Low-level register I/O ───────────────────────────────────────

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

// ── Calibration ──────────────────────────────────────────────────

void InaBridge228::applyShuntCalibration() {
  if (_rshuntOhm < 0.0001f) _rshuntOhm = 0.1f;
  if (_imaxA <= 0.0f) _imaxA = 10.0f;
  _currentLsb = _imaxA * 1.9073486328125e-6f;    // IMAX / 2^19
  float cal = 13107.2e6f * _currentLsb * _rshuntOhm;
  if (_adcRangeHigh) cal *= 4.0f;
  if (cal > 65535.0f) cal = 65535.0f;
  if (cal < 1.0f)     cal = 1.0f;
  writeU16(REG_SHUNT_CAL, (uint16_t)cal);
}

bool InaBridge228::detectChip() {
  return readU16(REG_MFG_ID) == 0x5449;
}

void InaBridge228::applyResetAndMode() {
  uint16_t cfg = readU16(REG_CONFIG);
  writeU16(REG_CONFIG, (uint16_t)(cfg | CFG_RST));
  delay(2);
  uint16_t adc = readU16(REG_ADC_CFG);
  adc = (uint16_t)((adc & ~ADC_MODE_MASK) | ((uint16_t)MODE_CONT_TEMP_BUS_SHUNT << 12));
  writeU16(REG_ADC_CFG, adc);
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

// ── Standalone Measurement API ───────────────────────────────────

float InaBridge228::readBusVoltage() {
  uint32_t raw = readU24(REG_VBUS);
  int32_t v = (int32_t)(raw >> 4);
  return (float)v * 195.3125e-6f;
}

float InaBridge228::readShuntVoltage() {
  uint32_t raw = readU24(REG_VSHUNT);
  int32_t v = (int32_t)(raw >> 4);
  if (v & 0x80000) v |= (int32_t)0xFFF00000;     // sign-extend 20-bit
  float lsb = _adcRangeHigh ? 78.125e-9f : 312.5e-9f;
  return (float)v * lsb;
}

float InaBridge228::readCurrent() {
  uint32_t raw = readU24(REG_CURRENT);
  int32_t v = (int32_t)(raw >> 4);
  if (v & 0x80000) v |= (int32_t)0xFFF00000;
  return (float)v * _currentLsb;
}

float InaBridge228::readPower() {
  uint32_t raw = readU24(REG_POWER);
  return (float)raw * 3.2f * _currentLsb;
}

float InaBridge228::readDieTemp() {
  uint16_t raw = readU16(REG_DIETEMP);
  return (float)((int16_t)raw) * 0.0078125f;      // 7.8125 m°C / LSB
}

float InaBridge228::readEnergy() {
  uint64_t raw = InaWireReadU40RegMsbFirst(_addr, REG_ENERGY);
  return (float)raw * 16.0f * 3.2f * _currentLsb;
}

float InaBridge228::readCharge() {
  uint64_t raw = InaWireReadU40RegMsbFirst(_addr, REG_CHARGE);
  if (raw & (1ULL << 39)) raw |= ~((1ULL << 40) - 1);  // sign-extend 40-bit
  return (float)((int64_t)raw) * _currentLsb;
}

void InaBridge228::resetAccumulators() {
  uint16_t cfg = readU16(REG_CONFIG);
  writeU16(REG_CONFIG, (uint16_t)(cfg | CFG_RSTACC));
}

bool InaBridge228::dataReady() {
  return (readU16(REG_DIAG_ALRT) & DIAG_CNVRF) != 0;
}

// ── Configuration API ────────────────────────────────────────────

void InaBridge228::startStreaming() { _streaming = true; }
void InaBridge228::stopStreaming()  { _streaming = false; }

void InaBridge228::setSampleRate(int hz) {
  _sampleHz = InaJsonl::clampStreamRateI2c(hz);
}

void InaBridge228::setRshunt(float ohm) {
  if (ohm < 0.0001f) ohm = 0.1f;
  _rshuntOhm = ohm;
  applyShuntCalibration();
}

void InaBridge228::setImax(float ampere) {
  if (ampere <= 0.0f) ampere = 10.0f;
  _imaxA = ampere;
  applyShuntCalibration();
}

// ── JSONL streaming (NiusRobotLab_INA_monitor compatible) ────────

void InaBridge228::emitJsonSample() {
  const float bus_V     = readBusVoltage();
  const float current_A = readCurrent();
  const float power_W   = readPower();
  const uint32_t t_ms   = millis();
  char addrStr[8];
  snprintf(addrStr, sizeof(addrStr), "0x%02X", _addr);
  Serial.print(F("{\"v\":1,\"chip\":\""));
  Serial.print(_chip);
  Serial.print(F("\",\"addr\":\""));
  Serial.print(addrStr);
  Serial.print(F("\",\"seq\":")); Serial.print(_seq++);
  Serial.print(F(",\"t_ms\":")); Serial.print(t_ms);
  Serial.print(F(",\"bus_V\":")); Serial.print(bus_V, 6);
  Serial.print(F(",\"current_A\":")); Serial.print(current_A, 6);
  Serial.print(F(",\"power_W\":")); Serial.print(power_W, 6);
  if (_extraFields) _extraFields();
  Serial.println(F("}"));
}

void InaBridge228::handleCommand(const String& line) {
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
    if (a <= 0.0f) a = 10.0f;
    _imaxA = a;
    applyShuntCalibration();
    InaJsonl::ackImax(_imaxA);
    return;
  }
  if (cmd.startsWith("RSHUNT ")) {
    float r = cmd.substring(7).toFloat();
    if (r < 0.0001f) r = 0.1f;
    _rshuntOhm = r;
    applyShuntCalibration();
    InaJsonl::ackRshunt(_rshuntOhm);
    return;
  }

  if (cmd.equalsIgnoreCase("DIAG")) {
    const uint16_t d    = readU16(REG_DIAG_ALRT);
    const uint16_t bovl = readU16(REG_BOVL);
    const uint16_t buvl = readU16(REG_BUVL);
    const uint16_t sovl = readU16(REG_SOVL);
    const uint16_t pwr  = readU16(REG_PWR_LIMIT);
    char addrStr[8];
    snprintf(addrStr, sizeof(addrStr), "0x%02X", _addr);
    Serial.print(F("{\"v\":1,\"type\":\"DIAG\",\"chip\":\""));
    Serial.print(_chip);
    Serial.print(F("\",\"addr\":\""));
    Serial.print(addrStr);
    Serial.print(F("\",\"diag_alrt\":")); Serial.print(d);
    Serial.print(F(",\"bovl\":"));        Serial.print(bovl);
    Serial.print(F(",\"buvl\":"));        Serial.print(buvl);
    Serial.print(F(",\"sovl\":"));        Serial.print(sovl);
    Serial.print(F(",\"pwr_limit\":"));   Serial.print(pwr);
    Serial.println(F("}"));
    return;
  }

  if (cmd.startsWith("ALERT ")) {
    String rest = cmd.substring(6);
    rest.trim();
    bool latch = false, pol = false;
    int pLatch = rest.indexOf("LATCH");
    if (pLatch >= 0) { latch = rest.substring(pLatch + 5).toInt() != 0; rest = rest.substring(0, pLatch); rest.trim(); }
    int pPol = rest.indexOf("POL");
    if (pPol >= 0) { pol = rest.substring(pPol + 3).toInt() != 0; rest = rest.substring(0, pPol); rest.trim(); }

    Ina::I2cBus bus;
    Ina::Ina228Driver drv(bus, _addr);
    Ina::AlertConfig cfg;
    cfg.enable = true;
    cfg.latch = latch;
    cfg.polarityActiveHigh = pol;

    if (rest.equalsIgnoreCase("OFF"))  { (void)drv.alertDisable(); Serial.println(F("{\"v\":1,\"type\":\"ACK\",\"cmd\":\"ALERT\",\"mode\":\"OFF\"}")); return; }
    if (rest.equalsIgnoreCase("CNVR")) { (void)drv.alertEnableConversionReady(cfg); Serial.println(F("{\"v\":1,\"type\":\"ACK\",\"cmd\":\"ALERT\",\"mode\":\"CNVR\"}")); return; }
    if (rest.startsWith("BOV "))       { (void)drv.alertEnableBusOverVoltage_V(rest.substring(4).toFloat(), cfg); Serial.println(F("{\"v\":1,\"type\":\"ACK\",\"cmd\":\"ALERT\",\"mode\":\"BOV\"}")); return; }
    if (rest.startsWith("BUV "))       { (void)drv.alertEnableBusUnderVoltage_V(rest.substring(4).toFloat(), cfg); Serial.println(F("{\"v\":1,\"type\":\"ACK\",\"cmd\":\"ALERT\",\"mode\":\"BUV\"}")); return; }
    if (rest.startsWith("SOV "))       { (void)drv.alertEnableShuntOverVoltage_uV(rest.substring(4).toFloat(), cfg); Serial.println(F("{\"v\":1,\"type\":\"ACK\",\"cmd\":\"ALERT\",\"mode\":\"SOV\"}")); return; }
    InaJsonl::errUnknownCmd(cmd);
    return;
  }

  InaJsonl::errUnknownCmd(cmd);
}

void InaBridge228::tick() {
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
