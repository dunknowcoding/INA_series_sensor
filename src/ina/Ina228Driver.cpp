/**
 * @file Ina228Driver.cpp
 */
#include "Ina228Driver.h"
#include "InaMathCompat.h"

namespace Ina {

Result Ina228Driver::readDiagAlrt(uint16_t& out) {
  return _bus.readU16(_addr, REG_DIAG_ALRT, out);
}

Result Ina228Driver::writeDiagAlrt(uint16_t v) {
  return _bus.writeU16(_addr, REG_DIAG_ALRT, v);
}

Result Ina228Driver::readBovl(uint16_t& out) { return _bus.readU16(_addr, REG_BOVL, out); }
Result Ina228Driver::writeBovl(uint16_t v) { return _bus.writeU16(_addr, REG_BOVL, v); }
Result Ina228Driver::readBuvl(uint16_t& out) { return _bus.readU16(_addr, REG_BUVL, out); }
Result Ina228Driver::writeBuvl(uint16_t v) { return _bus.writeU16(_addr, REG_BUVL, v); }
Result Ina228Driver::readSovl(uint16_t& out) { return _bus.readU16(_addr, REG_SOVL, out); }
Result Ina228Driver::writeSovl(uint16_t v) { return _bus.writeU16(_addr, REG_SOVL, v); }
Result Ina228Driver::readSuvl(uint16_t& out) { return _bus.readU16(_addr, REG_SUVL, out); }
Result Ina228Driver::writeSuvl(uint16_t v) { return _bus.writeU16(_addr, REG_SUVL, v); }
Result Ina228Driver::readTempLimit(uint16_t& out) { return _bus.readU16(_addr, REG_TEMP_LIMIT, out); }
Result Ina228Driver::writeTempLimit(uint16_t v) { return _bus.writeU16(_addr, REG_TEMP_LIMIT, v); }
Result Ina228Driver::readPowerLimit(uint16_t& out) { return _bus.readU16(_addr, REG_PWR_LIMIT, out); }
Result Ina228Driver::writePowerLimit(uint16_t v) { return _bus.writeU16(_addr, REG_PWR_LIMIT, v); }

Result Ina228Driver::readDieTempRaw(uint16_t& out) { return _bus.readU16(_addr, REG_DIETEMP, out); }

Result Ina228Driver::readEnergyRaw(uint64_t& out) {
  return _bus.readU40Msb(_addr, REG_ENERGY, out);
}

Result Ina228Driver::readChargeRaw(int64_t& out) {
  uint64_t raw = 0;
  Result r = _bus.readU40Msb(_addr, REG_CHARGE, raw);
  if (!r.ok()) return r;
  if (raw & (1ULL << 39)) raw |= ~((1ULL << 40) - 1);
  out = (int64_t)raw;
  return Result::OK();
}

Result Ina228Driver::readDieTemp_C(float& out) {
  uint16_t raw = 0;
  Result r = readDieTempRaw(raw);
  if (!r.ok()) return r;
  out = (float)((int16_t)raw) * 0.0078125f;
  return Result::OK();
}

Result Ina228Driver::readEnergy_J(float& out, float currentLsb) {
  uint64_t raw = 0;
  Result r = readEnergyRaw(raw);
  if (!r.ok()) return r;
  out = (float)raw * 16.0f * 3.2f * currentLsb;
  return Result::OK();
}

Result Ina228Driver::readCharge_C(float& out, float currentLsb) {
  int64_t raw = 0;
  Result r = readChargeRaw(raw);
  if (!r.ok()) return r;
  out = (float)raw * currentLsb;
  return Result::OK();
}

Result Ina228Driver::resetAccumulators() {
  uint16_t cfg = 0;
  Result r = _bus.readU16(_addr, REG_CONFIG, cfg);
  if (!r.ok()) return r;
  return _bus.writeU16(_addr, REG_CONFIG, (uint16_t)(cfg | CFG_RSTACC));
}

uint16_t Ina228Driver::busVoltageToBovlRaw(float limit_V) {
  if (!isfinite(limit_V) || limit_V < 0) limit_V = 0;
  const float mv = limit_V * 1000.0f;
  const uint32_t raw = (uint32_t)(mv / 3.125f + 0.5f);
  // bit 15 reserved 0; clamp to 0x7FFF
  return (uint16_t)(raw > 0x7FFFu ? 0x7FFFu : raw);
}

uint16_t Ina228Driver::shuntVoltageToSovlRaw_uV(float limit_uV, bool adcRangeHigh) {
  if (!isfinite(limit_uV)) limit_uV = 0;
  const float lsb_uV = adcRangeHigh ? 1.25f : 5.0f;
  const float uV = limit_uV;
  int32_t raw = (int32_t)(uV / lsb_uV);
  if (raw > 32767) raw = 32767;
  if (raw < -32768) raw = -32768;
  return (uint16_t)((int16_t)raw);
}

uint16_t Ina228Driver::tempToTempLimitRaw_cC(float temp_C) {
  // TEMP_LIMIT compares against DIETEMP, factor 7.8125 m°C/LSB => 0.0078125 °C/LSB.
  if (!isfinite(temp_C)) temp_C = 0;
  int32_t raw = (int32_t)(temp_C / 0.0078125f);
  if (raw > 32767) raw = 32767;
  if (raw < -32768) raw = -32768;
  return (uint16_t)((int16_t)raw);
}

Result Ina228Driver::setAlertLatch(bool enable) {
  uint16_t d = 0;
  (void)readDiagAlrt(d);
  if (enable) d |= DIAG_ALATCH;
  else d &= (uint16_t)~DIAG_ALATCH;
  return writeDiagAlrt(d);
}

Result Ina228Driver::setAlertPolarityActiveHigh(bool enable) {
  uint16_t d = 0;
  (void)readDiagAlrt(d);
  if (enable) d |= DIAG_APOL;
  else d &= (uint16_t)~DIAG_APOL;
  return writeDiagAlrt(d);
}

Result Ina228Driver::applyAlertConfig(const AlertConfig& cfg, bool enableConversionReady) {
  uint16_t d = 0;
  // Preserve MEMSTAT bit 0 default (1) by reading then rewriting with changes.
  (void)readDiagAlrt(d);

  // Clear output-control bits we own, then re-apply.
  d &= (uint16_t)~(DIAG_ALATCH | DIAG_APOL | DIAG_CNVR);
  if (cfg.latch) d |= DIAG_ALATCH;
  if (cfg.polarityActiveHigh) d |= DIAG_APOL;

  if (cfg.enable && enableConversionReady) d |= DIAG_CNVR;

  return writeDiagAlrt(d);
}

Result Ina228Driver::alertDisable() {
  AlertConfig cfg;
  cfg.enable = false;
  return applyAlertConfig(cfg, false);
}

Result Ina228Driver::alertEnableConversionReady(const AlertConfig& cfg) {
  AlertConfig tmp;
  tmp.enable = true;
  tmp.latch = cfg.latch;
  tmp.polarityActiveHigh = cfg.polarityActiveHigh;
  return applyAlertConfig(tmp, true);
}

Result Ina228Driver::alertEnableBusOverVoltage_V(float limit_V, const AlertConfig& cfg) {
  const uint16_t raw = busVoltageToBovlRaw(limit_V);
  const Result r1 = writeBovl(raw);
  if (!r1.ok()) return r1;
  // ALERT asserts when BUSOL trips against BOVL; DIAG_ALRT controls latch/polarity.
  AlertConfig tmp;
  tmp.enable = true;
  tmp.latch = cfg.latch;
  tmp.polarityActiveHigh = cfg.polarityActiveHigh;
  return applyAlertConfig(tmp, false);
}

Result Ina228Driver::alertEnableBusUnderVoltage_V(float limit_V, const AlertConfig& cfg) {
  const uint16_t raw = busVoltageToBovlRaw(limit_V);
  const Result r1 = writeBuvl(raw);
  if (!r1.ok()) return r1;
  AlertConfig tmp;
  tmp.enable = true;
  tmp.latch = cfg.latch;
  tmp.polarityActiveHigh = cfg.polarityActiveHigh;
  return applyAlertConfig(tmp, false);
}

Result Ina228Driver::alertEnableShuntOverVoltage_uV(float limit_uV, const AlertConfig& cfg) {
  const uint16_t raw = shuntVoltageToSovlRaw_uV(limit_uV, false);
  const Result r1 = writeSovl(raw);
  if (!r1.ok()) return r1;
  AlertConfig tmp;
  tmp.enable = true;
  tmp.latch = cfg.latch;
  tmp.polarityActiveHigh = cfg.polarityActiveHigh;
  return applyAlertConfig(tmp, false);
}

Result Ina228Driver::alertEnableTempOver_C(float limit_C, const AlertConfig& cfg) {
  const uint16_t raw = tempToTempLimitRaw_cC(limit_C);
  const Result r1 = writeTempLimit(raw);
  if (!r1.ok()) return r1;
  AlertConfig tmp;
  tmp.enable = true;
  tmp.latch = cfg.latch;
  tmp.polarityActiveHigh = cfg.polarityActiveHigh;
  return applyAlertConfig(tmp, false);
}

Result Ina228Driver::alertReadStatus(AlertStatus& out) {
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

}  // namespace Ina

