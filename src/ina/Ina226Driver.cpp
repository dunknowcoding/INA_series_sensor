/**
 * @file Ina226Driver.cpp
 */
#include "Ina226Driver.h"
#include <math.h>

namespace Ina {

Result Ina226Driver::readMaskEnable(uint16_t& out) {
  return _bus.readU16(_addr, REG_MASK_ENABLE, out);
}

Result Ina226Driver::writeMaskEnable(uint16_t v) {
  return _bus.writeU16(_addr, REG_MASK_ENABLE, v);
}

Result Ina226Driver::readAlertLimit(uint16_t& out) {
  return _bus.readU16(_addr, REG_ALERT_LIMIT, out);
}

Result Ina226Driver::writeAlertLimit(uint16_t v) {
  return _bus.writeU16(_addr, REG_ALERT_LIMIT, v);
}

Result Ina226Driver::setAlertLatch(bool enable) {
  uint16_t m = 0;
  (void)readMaskEnable(m);
  if (enable) m |= MASK_LEN;
  else m &= (uint16_t)~MASK_LEN;
  return writeMaskEnable(m);
}

Result Ina226Driver::setAlertPolarityActiveHigh(bool enable) {
  uint16_t m = 0;
  (void)readMaskEnable(m);
  if (enable) m |= MASK_APOL;
  else m &= (uint16_t)~MASK_APOL;
  return writeMaskEnable(m);
}

uint16_t Ina226Driver::busVoltageToAlertLimitRaw(float limit_V) {
  // INA226 bus voltage register LSB = 1.25 mV; alert limit uses same format as bus voltage register (per datasheet).
  if (!isfinite(limit_V) || limit_V < 0) limit_V = 0;
  const uint32_t mv = (uint32_t)(limit_V * 1000.0f + 0.5f);
  const uint32_t raw = mv / 1.25f;  // keep in float for correct rounding
  const uint32_t clamped = raw > 0xFFFFu ? 0xFFFFu : raw;
  return (uint16_t)clamped;
}

uint16_t Ina226Driver::shuntVoltageToAlertLimitRaw_uV(float limit_uV) {
  // INA226 shunt voltage register LSB = 2.5 uV.
  if (!isfinite(limit_uV)) limit_uV = 0;
  const float abs_uV = fabsf(limit_uV);
  const uint32_t raw = (uint32_t)(abs_uV / 2.5f + 0.5f);
  return (uint16_t)(raw > 0x7FFFu ? 0x7FFFu : raw);
}

Result Ina226Driver::applyAlertConfig(const AlertConfig& cfg, uint16_t baseMask) {
  uint16_t m = baseMask;
  if (cfg.latch) m |= MASK_LEN;
  if (cfg.polarityActiveHigh) m |= MASK_APOL;
  if (!cfg.enable) m = 0;
  return writeMaskEnable(m);
}

Result Ina226Driver::alertDisable() {
  AlertConfig cfg;
  cfg.enable = false;
  return applyAlertConfig(cfg, 0);
}

Result Ina226Driver::alertEnableConversionReady(const AlertConfig& cfg) {
  AlertConfig tmp;
  tmp.enable = true;
  tmp.latch = cfg.latch;
  tmp.polarityActiveHigh = cfg.polarityActiveHigh;
  return applyAlertConfig(tmp, MASK_CNVR);
}

Result Ina226Driver::alertEnableBusOverVoltage_V(float limit_V, const AlertConfig& cfg) {
  const uint16_t raw = busVoltageToAlertLimitRaw(limit_V);
  const Result r1 = writeAlertLimit(raw);
  if (!r1.ok()) return r1;
  AlertConfig tmp;
  tmp.enable = true;
  tmp.latch = cfg.latch;
  tmp.polarityActiveHigh = cfg.polarityActiveHigh;
  return applyAlertConfig(tmp, MASK_BOL);
}

Result Ina226Driver::alertEnableBusUnderVoltage_V(float limit_V, const AlertConfig& cfg) {
  const uint16_t raw = busVoltageToAlertLimitRaw(limit_V);
  const Result r1 = writeAlertLimit(raw);
  if (!r1.ok()) return r1;
  AlertConfig tmp;
  tmp.enable = true;
  tmp.latch = cfg.latch;
  tmp.polarityActiveHigh = cfg.polarityActiveHigh;
  return applyAlertConfig(tmp, MASK_BUL);
}

Result Ina226Driver::alertEnableShuntOverVoltage_uV(float limit_uV, const AlertConfig& cfg) {
  const uint16_t raw = shuntVoltageToAlertLimitRaw_uV(limit_uV);
  const Result r1 = writeAlertLimit(raw);
  if (!r1.ok()) return r1;
  AlertConfig tmp;
  tmp.enable = true;
  tmp.latch = cfg.latch;
  tmp.polarityActiveHigh = cfg.polarityActiveHigh;
  return applyAlertConfig(tmp, MASK_SOL);
}

Result Ina226Driver::alertEnablePowerOver_W(float limit_W, const AlertConfig& cfg) {
  // Pro-grade power limit requires correct calibration; we accept a raw conversion-free path here:
  // caller should compute and use writeAlertLimit() directly if they need exact control.
  // For a "simple" helper we treat limit_W as unsupported unless extended with calibration context.
  (void)limit_W;
  return Result::Err(Status::Unsupported);
}

Result Ina226Driver::alertReadStatus(AlertStatus& out) {
  uint16_t m = 0;
  const Result r = readMaskEnable(m);
  if (!r.ok()) return r;
  out.asserted = (m & MASK_AFF) != 0;
  out.conversionReady = (m & MASK_CNVR) != 0;
  out.overVoltage = (m & MASK_BOL) != 0;
  out.underVoltage = (m & MASK_BUL) != 0;
  out.overCurrent = (m & MASK_SOL) != 0;  // proxy
  out.overPower = (m & MASK_POL) != 0;
  return Result::OK();
}

}  // namespace Ina

