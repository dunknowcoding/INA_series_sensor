/**
 * @file Ina219Driver.cpp
 */
#include "Ina219Driver.h"
#include <math.h>

namespace Ina {

Result Ina219Driver::readMaskEnable(uint16_t& out) {
  return _bus.readU16(_addr, REG_MASK_ENABLE, out);
}

Result Ina219Driver::writeMaskEnable(uint16_t v) {
  return _bus.writeU16(_addr, REG_MASK_ENABLE, v);
}

Result Ina219Driver::readAlertLimit(uint16_t& out) {
  return _bus.readU16(_addr, REG_ALERT_LIMIT, out);
}

Result Ina219Driver::writeAlertLimit(uint16_t v) {
  return _bus.writeU16(_addr, REG_ALERT_LIMIT, v);
}

Result Ina219Driver::setAlertLatch(bool enable) {
  uint16_t m = 0;
  (void)readMaskEnable(m);
  if (enable) m |= MASK_LEN;
  else m &= (uint16_t)~MASK_LEN;
  return writeMaskEnable(m);
}

Result Ina219Driver::setAlertPolarityActiveHigh(bool enable) {
  uint16_t m = 0;
  (void)readMaskEnable(m);
  if (enable) m |= MASK_APOL;
  else m &= (uint16_t)~MASK_APOL;
  return writeMaskEnable(m);
}

uint16_t Ina219Driver::busVoltageToAlertLimitRaw(float limit_V) {
  // INA219 Bus Voltage Register LSB = 4 mV, stored in bits [15:3] (same format used for bus alert).
  if (!isfinite(limit_V) || limit_V < 0) limit_V = 0;
  const uint32_t mv = (uint32_t)(limit_V * 1000.0f + 0.5f);
  const uint32_t steps = mv / 4u;
  const uint32_t reg = (steps & 0x1FFFu) << 3;
  return (uint16_t)(reg > 0xFFFFu ? 0xFFFFu : reg);
}

uint16_t Ina219Driver::shuntVoltageToAlertLimitRaw_uV(float limit_uV) {
  // INA219 Shunt Voltage Register LSB = 10 uV (signed). Alert limit uses same LSB.
  if (!isfinite(limit_uV)) limit_uV = 0;
  const float abs_uV = fabsf(limit_uV);
  const uint32_t raw = (uint32_t)(abs_uV / 10.0f + 0.5f);
  return (uint16_t)(raw > 0x7FFFu ? 0x7FFFu : raw);
}

Result Ina219Driver::applyAlertConfig(const AlertConfig& cfg, uint16_t baseMask) {
  uint16_t m = baseMask;
  if (cfg.latch) m |= MASK_LEN;
  if (cfg.polarityActiveHigh) m |= MASK_APOL;
  if (!cfg.enable) m = 0;
  return writeMaskEnable(m);
}

Result Ina219Driver::alertDisable() {
  AlertConfig cfg;
  cfg.enable = false;
  return applyAlertConfig(cfg, 0);
}

Result Ina219Driver::alertEnableConversionReady(const AlertConfig& cfg) {
  AlertConfig tmp;
  tmp.enable = true;
  tmp.latch = cfg.latch;
  tmp.polarityActiveHigh = cfg.polarityActiveHigh;
  return applyAlertConfig(tmp, MASK_CNVR);
}

Result Ina219Driver::alertEnableBusOverVoltage_V(float limit_V, const AlertConfig& cfg) {
  const uint16_t raw = busVoltageToAlertLimitRaw(limit_V);
  const Result r1 = writeAlertLimit(raw);
  if (!r1.ok()) return r1;
  AlertConfig tmp;
  tmp.enable = true;
  tmp.latch = cfg.latch;
  tmp.polarityActiveHigh = cfg.polarityActiveHigh;
  return applyAlertConfig(tmp, MASK_BOL);
}

Result Ina219Driver::alertEnableBusUnderVoltage_V(float limit_V, const AlertConfig& cfg) {
  const uint16_t raw = busVoltageToAlertLimitRaw(limit_V);
  const Result r1 = writeAlertLimit(raw);
  if (!r1.ok()) return r1;
  AlertConfig tmp;
  tmp.enable = true;
  tmp.latch = cfg.latch;
  tmp.polarityActiveHigh = cfg.polarityActiveHigh;
  return applyAlertConfig(tmp, MASK_BUL);
}

Result Ina219Driver::alertEnableShuntOverVoltage_uV(float limit_uV, const AlertConfig& cfg) {
  const uint16_t raw = shuntVoltageToAlertLimitRaw_uV(limit_uV);
  const Result r1 = writeAlertLimit(raw);
  if (!r1.ok()) return r1;
  AlertConfig tmp;
  tmp.enable = true;
  tmp.latch = cfg.latch;
  tmp.polarityActiveHigh = cfg.polarityActiveHigh;
  return applyAlertConfig(tmp, MASK_SOL);
}

Result Ina219Driver::alertReadStatus(AlertStatus& out) {
  uint16_t m = 0;
  const Result r = readMaskEnable(m);
  if (!r.ok()) return r;
  out.asserted = (m & MASK_AFF) != 0;
  out.conversionReady = (m & MASK_CNVR) != 0;
  out.overVoltage = (m & MASK_BOL) != 0;
  out.underVoltage = (m & MASK_BUL) != 0;
  out.overCurrent = (m & MASK_SOL) != 0;
  out.overPower = (m & MASK_POL) != 0;
  return Result::OK();
}

}  // namespace Ina

