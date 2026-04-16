#include "InaBridge229Spi.h"
#include "InaJsonlProtocol.h"
#include "InaSpiCompat.h"
#include "ina/InaSpiBus.h"
#include "ina/Ina229Driver.h"
#include <stdio.h>

// INA229 register addresses (same as INA228)
static const uint8_t REG_CONFIG    = 0x00;
static const uint8_t REG_ADC_CONFIG = 0x01;
static const uint8_t REG_SHUNT_CAL = 0x02;
static const uint8_t REG_VSHUNT    = 0x04;
static const uint8_t REG_VBUS      = 0x05;
static const uint8_t REG_DIETEMP   = 0x06;
static const uint8_t REG_CURRENT   = 0x07;
static const uint8_t REG_POWER     = 0x08;
static const uint8_t REG_ENERGY    = 0x09;
static const uint8_t REG_CHARGE    = 0x0A;
static const uint8_t REG_DIAG_ALRT = 0x0B;
static const uint8_t REG_MFG_ID    = 0x3E;

static const uint16_t CFG_RST      = 0x8000;
static const uint16_t CFG_RSTACC   = 0x4000;
static const uint16_t ADC_MODE_MASK = 0xF000;
static const uint8_t  MODE_CONT_TEMP_BUS_SHUNT = 0x0F;
static const uint16_t DIAG_CNVRF   = 0x0002;

// ── Construction / Initialization ────────────────────────────────

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
  Serial.flush();
}

// ── Low-level SPI register I/O ───────────────────────────────────

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

uint64_t InaBridge229Spi::readU40(uint8_t reg) {
  uint8_t addr = (uint8_t)((reg << 2) | 1u);
  digitalWrite(_cs, LOW);
  SPI.beginTransaction(_spiSettings);
  (void)SPI.transfer(addr);
  uint64_t value = 0;
  for (uint8_t i = 0; i < 5; i++) {
    value = (value << 8) | SPI.transfer(0);
  }
  SPI.endTransaction();
  digitalWrite(_cs, HIGH);
  return value;
}

// ── Calibration ──────────────────────────────────────────────────

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
           "{\"v\":1,\"type\":\"INFO\",\"msg\":\"%s bridge ready. No JSON samples until START (optional SR <Hz> "
           "first).\",\"author\":\"NiusRobotLab\",\"chip\":\"%s\","
           "\"addr\":\"SPI\",\"ref\":\"%s\"}",
           _chip, _chip, _ref);
  Serial.println(buf);
}

// ── Standalone Measurement API ───────────────────────────────────

float InaBridge229Spi::readBusVoltage() {
  uint32_t raw = readU24(REG_VBUS);
  int32_t v = (int32_t)(raw >> 4);
  return (float)v * 195.3125e-6f;
}

float InaBridge229Spi::readShuntVoltage() {
  uint32_t raw = readU24(REG_VSHUNT);
  int32_t v = (int32_t)(raw >> 4);
  if (v & 0x80000) v |= (int32_t)0xFFF00000;     // sign-extend 20-bit
  float lsb = _adcRangeHigh ? 78.125e-9f : 312.5e-9f;
  return (float)v * lsb;
}

float InaBridge229Spi::readCurrent() {
  uint32_t raw = readU24(REG_CURRENT);
  int32_t v = (int32_t)(raw >> 4);
  if (v & 0x80000) v |= 0xFFF00000;
  return (float)v * _currentLsb;
}

float InaBridge229Spi::readPower() {
  uint32_t raw = readU24(REG_POWER);
  return (float)raw * 3.2f * _currentLsb;
}

float InaBridge229Spi::readDieTemp() {
  uint16_t raw = readU16(REG_DIETEMP);
  return (float)((int16_t)raw) * 0.0078125f;      // 7.8125 m°C / LSB
}

float InaBridge229Spi::readEnergy() {
  uint64_t raw = readU40(REG_ENERGY);
  return (float)raw * 16.0f * 3.2f * _currentLsb;
}

float InaBridge229Spi::readCharge() {
  uint64_t raw = readU40(REG_CHARGE);
  if (raw & (1ULL << 39)) raw |= ~((1ULL << 40) - 1);  // sign-extend 40-bit
  return (float)((int64_t)raw) * _currentLsb;
}

void InaBridge229Spi::resetAccumulators() {
  uint16_t cfg = readU16(REG_CONFIG);
  spiWriteU16(REG_CONFIG, (uint16_t)(cfg | CFG_RSTACC));
}

bool InaBridge229Spi::dataReady() {
  return (readU16(REG_DIAG_ALRT) & DIAG_CNVRF) != 0;
}

// ── Configuration API ────────────────────────────────────────────

void InaBridge229Spi::startStreaming()  { _streaming = true; }
void InaBridge229Spi::stopStreaming()   { _streaming = false; }

void InaBridge229Spi::setSampleRate(int hz) {
  _sampleHz = InaJsonl::clampStreamRateSpi(hz);
}

void InaBridge229Spi::setRshunt(float ohm) {
  if (ohm < 0.0001f) ohm = 0.1f;
  _rshuntOhm = ohm;
  applyShuntCalibration();
}

void InaBridge229Spi::setImax(float ampere) {
  if (ampere <= 0.0f) ampere = 10.0f;
  _imaxA = ampere;
  applyShuntCalibration();
}

// ── JSONL streaming (NiusRobotLab_INA_monitor compatible) ────────

void InaBridge229Spi::emitJsonSample() {
  const float bus_V     = readBusVoltage();
  const float current_A = readCurrent();
  const float power_W   = readPower();
  const uint32_t t_ms   = millis();
  Serial.print(F("{\"v\":1,\"chip\":\""));
  Serial.print(_chip);
  Serial.print(F("\",\"addr\":\"SPI\",\"seq\":")); Serial.print(_seq++);
  Serial.print(F(",\"t_ms\":")); Serial.print(t_ms);
  Serial.print(F(",\"bus_V\":")); Serial.print(bus_V, 6);
  Serial.print(F(",\"current_A\":")); Serial.print(current_A, 6);
  Serial.print(F(",\"power_W\":")); Serial.print(power_W, 6);
  if (_extraFields) _extraFields();
  Serial.println(F("}"));
}

void InaBridge229Spi::handleCommand(const String& line) {
  String cmd = line;
  InaJsonl::normalizeCmd(cmd);
  if (cmd.length() == 0) return;

  if (cmd.equalsIgnoreCase("PING"))  { InaJsonl::pong(); return; }
  if (cmd.equalsIgnoreCase("START")) { _streaming = true;  InaJsonl::ackStart(); return; }
  if (cmd.equalsIgnoreCase("STOP"))  { _streaming = false; InaJsonl::ackStop();  return; }

  if (cmd.startsWith("SR ")) {
    _sampleHz = InaJsonl::clampStreamRateSpi(cmd.substring(3).toInt());
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
    const uint16_t d = readU16(REG_DIAG_ALRT);
    Serial.print(F("{\"v\":1,\"type\":\"DIAG\",\"chip\":\""));
    Serial.print(_chip);
    Serial.print(F("\",\"addr\":\"SPI\",\"diag_alrt\":"));
    Serial.print(d);
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
    Ina::SpiBus bus(_cs, _sck, _miso, _mosi, _spiSettings);
    bus.begin();
    Ina::Ina229Driver drv(bus);
    Ina::AlertConfig cfg;
    cfg.enable = true;
    cfg.latch = latch;
    cfg.polarityActiveHigh = pol;
    if (rest.equalsIgnoreCase("OFF")) {
      (void)drv.alertDisable();
      Serial.println(F("{\"v\":1,\"type\":\"ACK\",\"cmd\":\"ALERT\",\"mode\":\"OFF\"}"));
      return;
    }
    if (rest.equalsIgnoreCase("CNVR")) {
      (void)drv.alertEnableConversionReady(cfg);
      Serial.println(F("{\"v\":1,\"type\":\"ACK\",\"cmd\":\"ALERT\",\"mode\":\"CNVR\"}"));
      return;
    }
    InaJsonl::errUnknownCmd(cmd);
    return;
  }

  InaJsonl::errUnknownCmd(cmd);
}

void InaBridge229Spi::tick() {
  String line;
  if (_rx.pollLine(line)) {
    handleCommand(line);
  }
  if (!_streaming) return;
  const uint32_t interval_us = InaJsonl::sampleIntervalMicros(_sampleHz);
  const uint32_t now = micros();
  if ((uint32_t)(now - _lastSampleUs) >= interval_us) {
    _lastSampleUs = now;
    emitJsonSample();
  }
}
