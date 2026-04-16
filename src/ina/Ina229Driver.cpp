/**
 * @file Ina229Driver.cpp
 */
#include "Ina229Driver.h"
#include <math.h>

namespace Ina {

Result Ina229Driver::readDiagAlrt(uint16_t& out) {
  return _bus.readU16(REG_DIAG_ALRT, out);
}

Result Ina229Driver::writeDiagAlrt(uint16_t v) {
  return _bus.writeU16(REG_DIAG_ALRT, v);
}

Result Ina229Driver::setAlertLatch(bool enable) {
  uint16_t d = 0;
  (void)readDiagAlrt(d);
  if (enable) d |= DIAG_ALATCH;
  else d &= (uint16_t)~DIAG_ALATCH;
  return writeDiagAlrt(d);
}

Result Ina229Driver::setAlertPolarityActiveHigh(bool enable) {
  uint16_t d = 0;
  (void)readDiagAlrt(d);
  if (enable) d |= DIAG_APOL;
  else d &= (uint16_t)~DIAG_APOL;
  return writeDiagAlrt(d);
}

Result Ina229Driver::alertDisable() {
  uint16_t d = 0;
  (void)readDiagAlrt(d);
  d &= (uint16_t)~(DIAG_CNVR | DIAG_ALATCH | DIAG_APOL);
  return writeDiagAlrt(d);
}

Result Ina229Driver::alertEnableConversionReady(const AlertConfig& cfg) {
  uint16_t d = 0;
  (void)readDiagAlrt(d);
  d &= (uint16_t)~(DIAG_ALATCH | DIAG_APOL);
  if (cfg.latch) d |= DIAG_ALATCH;
  if (cfg.polarityActiveHigh) d |= DIAG_APOL;
  d |= DIAG_CNVR;
  return writeDiagAlrt(d);
}

Result Ina229Driver::alertEnableTempOver_C(float limit_C, const AlertConfig& cfg) {
  const uint16_t raw = tempToTempLimitRaw(limit_C);
  Result r = writeTempLimit(raw);
  if (!r.ok()) return r;
  uint16_t d = 0;
  (void)readDiagAlrt(d);
  d &= (uint16_t)~(DIAG_ALATCH | DIAG_APOL);
  if (cfg.latch) d |= DIAG_ALATCH;
  if (cfg.polarityActiveHigh) d |= DIAG_APOL;
  return writeDiagAlrt(d);
}

Result Ina229Driver::alertReadStatus(AlertStatus& out) {
  uint16_t d = 0;
  const Result r = readDiagAlrt(d);
  if (!r.ok()) return r;
  out.conversionReady = (d & DIAG_CNVRF) != 0;
  out.overVoltage = (d & DIAG_BUSOL) != 0;
  out.underVoltage = (d & DIAG_BUSUL) != 0;
  out.overCurrent = (d & DIAG_SHNTOL) != 0;
  out.overPower = (d & DIAG_POL) != 0;
  out.overTemp = (d & DIAG_TMPOL) != 0;
  out.shuntUnderVoltage = (d & DIAG_SHNTUL) != 0;
  out.asserted = out.conversionReady || out.overVoltage || out.underVoltage ||
                 out.overCurrent || out.overPower || out.overTemp || out.shuntUnderVoltage;
  return Result::OK();
}

Result Ina229Driver::readDieTempRaw(uint16_t& out) { return _bus.readU16(REG_DIETEMP, out); }

Result Ina229Driver::readDieTemp_C(float& out) {
  uint16_t raw = 0;
  Result r = readDieTempRaw(raw);
  if (!r.ok()) return r;
  out = (float)((int16_t)raw) * 0.0078125f;
  return Result::OK();
}

Result Ina229Driver::readEnergyRaw(uint64_t& out) { return _bus.readU40(REG_ENERGY, out); }

Result Ina229Driver::readChargeRaw(int64_t& out) {
  uint64_t raw = 0;
  Result r = _bus.readU40(REG_CHARGE, raw);
  if (!r.ok()) return r;
  if (raw & (1ULL << 39)) raw |= ~((1ULL << 40) - 1);
  out = (int64_t)raw;
  return Result::OK();
}

Result Ina229Driver::readEnergy_J(float& out, float currentLsb) {
  uint64_t raw = 0;
  Result r = readEnergyRaw(raw);
  if (!r.ok()) return r;
  out = (float)raw * 16.0f * 3.2f * currentLsb;
  return Result::OK();
}

Result Ina229Driver::readCharge_C(float& out, float currentLsb) {
  int64_t raw = 0;
  Result r = readChargeRaw(raw);
  if (!r.ok()) return r;
  out = (float)raw * currentLsb;
  return Result::OK();
}

Result Ina229Driver::resetAccumulators() {
  uint16_t cfg = 0;
  Result r = _bus.readU16(REG_CONFIG, cfg);
  if (!r.ok()) return r;
  return _bus.writeU16(REG_CONFIG, (uint16_t)(cfg | CFG_RSTACC));
}

Result Ina229Driver::readTempLimit(uint16_t& out) { return _bus.readU16(REG_TEMP_LIMIT, out); }
Result Ina229Driver::writeTempLimit(uint16_t v) { return _bus.writeU16(REG_TEMP_LIMIT, v); }

uint16_t Ina229Driver::tempToTempLimitRaw(float temp_C) {
  if (!isfinite(temp_C)) temp_C = 0;
  int32_t raw = (int32_t)(temp_C / 0.0078125f);
  if (raw > 32767) raw = 32767;
  if (raw < -32768) raw = -32768;
  return (uint16_t)((int16_t)raw);
}

}  // namespace Ina

