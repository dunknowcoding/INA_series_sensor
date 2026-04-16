/**
 * @file Ina3221Driver.h
 * @brief INA3221 driver with special-pin support (CRI/WAR/TC/PV) via Mask/Enable + limit registers.
 *
 * Module pins often labeled:
 * - CRI: Critical alert output (active-low, open-drain)
 * - WAR: Warning alert output (active-low, open-drain)
 * - TC:  Timing control alert output (active-low, open-drain)
 * - PV:  Power valid output (active-low/open-drain depends on board); uses VPU as pull-up rail on some modules
 *
 * This driver exposes both simple (common) and pro (full register) APIs.
 */
#pragma once

#include <Arduino.h>

#include "InaI2cBus.h"
#include "InaTypes.h"

namespace Ina {

struct Ina3221MaskEnable {
  uint16_t raw = 0;
  bool convReadyFlag = false;   // CVRF (bit 0)
  bool timeControlFlag = false; // TCF (bit 1)
  bool powerValidFlag = false;  // PVF (bit 2)
  bool warnFlag[3] = {false, false, false}; // WF1-3 (bits 5-3)
  bool summationFlag = false;   // SF (bit 6)
  bool critFlag[3] = {false, false, false}; // CF1-3 (bits 9-7)
  bool criticalLatchEnable = false; // CEN (bit 10)
  bool warningLatchEnable = false;  // WEN (bit 11)
  bool sumChannelEnable[3] = {false, false, false}; // SCC1-3 (bits 14-12)
};

class Ina3221Driver {
public:
  explicit Ina3221Driver(I2cBus& bus, uint8_t addr7 = 0x40) : _bus(bus), _addr(addr7) {}

  uint8_t address() const { return _addr; }
  void setAddress(uint8_t addr7) { _addr = addr7; }

  // -------- Simple API (alerts) --------

  /** Disable all alert outputs (Mask/Enable cleared). */
  Result alertsDisableAll();

  /** Enable critical alert for one channel with limit in Amps (uses shunt resistance). */
  Result enableCriticalOverCurrent_A(uint8_t ch1to3, float limit_A, float rshunt_Ohm);

  /** Enable warning alert for one channel with limit in Amps (uses shunt resistance). */
  Result enableWarningOverCurrent_A(uint8_t ch1to3, float limit_A, float rshunt_Ohm);

  /** Enable power-valid comparator (PV pin) with bus-voltage window. */
  Result enablePowerValidWindow_V(float vmin_V, float vmax_V);

  /** Enable summation alert: CRI fires when sum of selected channels exceeds limit. */
  Result enableSummationAlert_uV(float limit_uV, bool ch1 = true, bool ch2 = true, bool ch3 = true);

  /** Read shunt voltage sum of enabled channels (µV). */
  Result readShuntVoltageSum_uV(float& out);

  /** Read Mask/Enable register and decode into a struct. */
  Result readMaskEnable(Ina3221MaskEnable& out);

  // -------- Pro API (register-level) --------
  Result readMaskEnableRaw(uint16_t& out);
  Result writeMaskEnableRaw(uint16_t v);

  Result writeCriticalLimitRaw(uint8_t ch1to3, uint16_t regVal);
  Result writeWarningLimitRaw(uint8_t ch1to3, uint16_t regVal);

  Result writePowerValidUpperLimitRaw(uint16_t regVal);
  Result writePowerValidLowerLimitRaw(uint16_t regVal);

  Result readShuntVoltageSumRaw(int16_t& out);
  Result writeShuntVoltageSumLimitRaw(uint16_t v);

  // -------- Helpers: conversions --------
  // INA3221 shunt voltage LSB = 40 uV; limit regs use same format as shunt regs (signed).
  static uint16_t shuntVoltageToLimitRaw_uV(float limit_uV);
  // INA3221 bus voltage LSB = 8 mV; PV limit regs use bus format.
  static uint16_t busVoltageToPvLimitRaw(float limit_V);

private:
  I2cBus& _bus;
  uint8_t _addr;

  static constexpr uint8_t REG_SHUNT_SUM = 0x0D;
  static constexpr uint8_t REG_SHUNT_SUM_LIMIT = 0x0E;
  static constexpr uint8_t REG_MASK_ENABLE = 0x0F;
  static constexpr uint8_t REG_PV_UPPER = 0x10;
  static constexpr uint8_t REG_PV_LOWER = 0x11;

  // Critical/Warning limit registers (per TI docs / common implementations)
  static constexpr uint8_t REG_CRIT_CH1 = 0x07;
  static constexpr uint8_t REG_WARN_CH1 = 0x08;
  static constexpr uint8_t REG_CRIT_CH2 = 0x09;
  static constexpr uint8_t REG_WARN_CH2 = 0x0A;
  static constexpr uint8_t REG_CRIT_CH3 = 0x0B;
  static constexpr uint8_t REG_WARN_CH3 = 0x0C;

  // Mask/Enable bits (INA3221 datasheet Table 7-35 / 7-36)
  static constexpr uint16_t ME_CVRF = 1u << 0;
  static constexpr uint16_t ME_TCF = 1u << 1;
  static constexpr uint16_t ME_PVF = 1u << 2;
  static constexpr uint16_t ME_WF3 = 1u << 3;
  static constexpr uint16_t ME_WF2 = 1u << 4;
  static constexpr uint16_t ME_WF1 = 1u << 5;
  static constexpr uint16_t ME_SF = 1u << 6;
  static constexpr uint16_t ME_CF3 = 1u << 7;
  static constexpr uint16_t ME_CF2 = 1u << 8;
  static constexpr uint16_t ME_CF1 = 1u << 9;
  static constexpr uint16_t ME_CEN = 1u << 10;
  static constexpr uint16_t ME_WEN = 1u << 11;
  static constexpr uint16_t ME_SCC3 = 1u << 12;
  static constexpr uint16_t ME_SCC2 = 1u << 13;
  static constexpr uint16_t ME_SCC1 = 1u << 14;

  static uint8_t critRegForCh(uint8_t ch1to3);
  static uint8_t warnRegForCh(uint8_t ch1to3);
};

}  // namespace Ina

