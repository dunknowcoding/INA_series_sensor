/**
 * @file Ina3221Driver.cpp
 */
#include "Ina3221Driver.h"
#include "InaMathCompat.h"

namespace Ina {

uint8_t Ina3221Driver::critRegForCh(uint8_t ch1to3) {
  switch (ch1to3) {
    case 1: return REG_CRIT_CH1;
    case 2: return REG_CRIT_CH2;
    default: return REG_CRIT_CH3;
  }
}

uint8_t Ina3221Driver::warnRegForCh(uint8_t ch1to3) {
  switch (ch1to3) {
    case 1: return REG_WARN_CH1;
    case 2: return REG_WARN_CH2;
    default: return REG_WARN_CH3;
  }
}

Result Ina3221Driver::readMaskEnableRaw(uint16_t& out) {
  return _bus.readU16(_addr, REG_MASK_ENABLE, out);
}

Result Ina3221Driver::writeMaskEnableRaw(uint16_t v) {
  return _bus.writeU16(_addr, REG_MASK_ENABLE, v);
}

Result Ina3221Driver::readMaskEnable(Ina3221MaskEnable& out) {
  uint16_t m = 0;
  const Result r = readMaskEnableRaw(m);
  if (!r.ok()) return r;
  out.raw = m;
  out.convReadyFlag = (m & ME_CVRF) != 0;
  out.timeControlFlag = (m & ME_TCF) != 0;
  out.powerValidFlag = (m & ME_PVF) != 0;
  out.warnFlag[0] = (m & ME_WF1) != 0;
  out.warnFlag[1] = (m & ME_WF2) != 0;
  out.warnFlag[2] = (m & ME_WF3) != 0;
  out.summationFlag = (m & ME_SF) != 0;
  out.critFlag[0] = (m & ME_CF1) != 0;
  out.critFlag[1] = (m & ME_CF2) != 0;
  out.critFlag[2] = (m & ME_CF3) != 0;
  out.criticalLatchEnable = (m & ME_CEN) != 0;
  out.warningLatchEnable = (m & ME_WEN) != 0;
  out.sumChannelEnable[0] = (m & ME_SCC1) != 0;
  out.sumChannelEnable[1] = (m & ME_SCC2) != 0;
  out.sumChannelEnable[2] = (m & ME_SCC3) != 0;
  return Result::OK();
}

Result Ina3221Driver::alertsDisableAll() {
  return writeMaskEnableRaw(0);
}

uint16_t Ina3221Driver::shuntVoltageToLimitRaw_uV(float limit_uV) {
  // LSB = 40 uV per datasheet; signed register.
  if (!isfinite(limit_uV)) limit_uV = 0;
  int32_t raw = (int32_t)(limit_uV / 40.0f);
  if (raw > 32767) raw = 32767;
  if (raw < -32768) raw = -32768;
  return (uint16_t)((int16_t)raw);
}

uint16_t Ina3221Driver::busVoltageToPvLimitRaw(float limit_V) {
  // LSB = 8 mV, stored in bits [15:3] like bus voltage register.
  if (!isfinite(limit_V) || limit_V < 0) limit_V = 0;
  const uint32_t mv = (uint32_t)(limit_V * 1000.0f + 0.5f);
  const uint32_t steps = mv / 8u;
  const uint32_t reg = (steps & 0x1FFFu) << 3;
  return (uint16_t)(reg > 0xFFFFu ? 0xFFFFu : reg);
}

Result Ina3221Driver::writeCriticalLimitRaw(uint8_t ch1to3, uint16_t regVal) {
  if (ch1to3 < 1 || ch1to3 > 3) return Result::Err(Status::BadParam);
  return _bus.writeU16(_addr, critRegForCh(ch1to3), regVal);
}

Result Ina3221Driver::writeWarningLimitRaw(uint8_t ch1to3, uint16_t regVal) {
  if (ch1to3 < 1 || ch1to3 > 3) return Result::Err(Status::BadParam);
  return _bus.writeU16(_addr, warnRegForCh(ch1to3), regVal);
}

Result Ina3221Driver::writePowerValidUpperLimitRaw(uint16_t regVal) {
  return _bus.writeU16(_addr, REG_PV_UPPER, regVal);
}

Result Ina3221Driver::writePowerValidLowerLimitRaw(uint16_t regVal) {
  return _bus.writeU16(_addr, REG_PV_LOWER, regVal);
}

Result Ina3221Driver::enableCriticalOverCurrent_A(uint8_t ch1to3, float limit_A, float rshunt_Ohm) {
  if (ch1to3 < 1 || ch1to3 > 3) return Result::Err(Status::BadParam);
  if (!isfinite(limit_A) || limit_A < 0) return Result::Err(Status::BadParam);
  if (!isfinite(rshunt_Ohm) || rshunt_Ohm <= 0) return Result::Err(Status::BadParam);
  const float limit_uV = (limit_A * rshunt_Ohm) * 1.0e6f;
  const uint16_t raw = shuntVoltageToLimitRaw_uV(limit_uV);
  const Result r1 = writeCriticalLimitRaw(ch1to3, raw);
  if (!r1.ok()) return r1;
  uint16_t m = 0;
  (void)readMaskEnableRaw(m);  // read does clear CF/SF/WF/CVRF flags per datasheet
  m |= ME_CEN;                 // enable critical alert pin function
  return writeMaskEnableRaw(m);
}

Result Ina3221Driver::enableWarningOverCurrent_A(uint8_t ch1to3, float limit_A, float rshunt_Ohm) {
  if (ch1to3 < 1 || ch1to3 > 3) return Result::Err(Status::BadParam);
  if (!isfinite(limit_A) || limit_A < 0) return Result::Err(Status::BadParam);
  if (!isfinite(rshunt_Ohm) || rshunt_Ohm <= 0) return Result::Err(Status::BadParam);
  const float limit_uV = (limit_A * rshunt_Ohm) * 1.0e6f;
  const uint16_t raw = shuntVoltageToLimitRaw_uV(limit_uV);
  const Result r1 = writeWarningLimitRaw(ch1to3, raw);
  if (!r1.ok()) return r1;
  uint16_t m = 0;
  (void)readMaskEnableRaw(m);
  m |= ME_WEN;  // enable warning alert pin function
  return writeMaskEnableRaw(m);
}

Result Ina3221Driver::enablePowerValidWindow_V(float vmin_V, float vmax_V) {
  if (!isfinite(vmin_V) || !isfinite(vmax_V) || vmin_V < 0 || vmax_V < 0 || vmax_V < vmin_V) {
    return Result::Err(Status::BadParam);
  }
  const Result r1 = writePowerValidLowerLimitRaw(busVoltageToPvLimitRaw(vmin_V));
  if (!r1.ok()) return r1;
  const Result r2 = writePowerValidUpperLimitRaw(busVoltageToPvLimitRaw(vmax_V));
  if (!r2.ok()) return r2;
  // PV output asserts based on bus voltages and PV upper/lower limit registers.
  // PVF is a status flag (not an enable). No Mask/Enable bits are required for PV output.
  return Result::OK();
}

Result Ina3221Driver::readShuntVoltageSumRaw(int16_t& out) {
  uint16_t raw = 0;
  Result r = _bus.readU16(_addr, REG_SHUNT_SUM, raw);
  if (!r.ok()) return r;
  out = (int16_t)raw;
  return Result::OK();
}

Result Ina3221Driver::readShuntVoltageSum_uV(float& out) {
  int16_t raw = 0;
  Result r = readShuntVoltageSumRaw(raw);
  if (!r.ok()) return r;
  out = (float)raw * 40.0f;
  return Result::OK();
}

Result Ina3221Driver::writeShuntVoltageSumLimitRaw(uint16_t v) {
  return _bus.writeU16(_addr, REG_SHUNT_SUM_LIMIT, v);
}

Result Ina3221Driver::enableSummationAlert_uV(float limit_uV, bool ch1, bool ch2, bool ch3) {
  const uint16_t raw = shuntVoltageToLimitRaw_uV(limit_uV);
  Result r = writeShuntVoltageSumLimitRaw(raw);
  if (!r.ok()) return r;
  uint16_t m = 0;
  (void)readMaskEnableRaw(m);
  m &= (uint16_t)~(ME_SCC1 | ME_SCC2 | ME_SCC3);
  if (ch1) m |= ME_SCC1;
  if (ch2) m |= ME_SCC2;
  if (ch3) m |= ME_SCC3;
  return writeMaskEnableRaw(m);
}

}  // namespace Ina

