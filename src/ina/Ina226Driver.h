/**
 * @file Ina226Driver.h
 * @brief INA226-class driver (INA226 / INA230–234 / INA236).
 *
 * Focus: special-pin support for ALERT (a.k.a. ALE on many modules) + simple/pro APIs.
 */
#pragma once

#include <Arduino.h>

#include "InaI2cBus.h"
#include "InaTypes.h"

namespace Ina {

class Ina226Driver {
public:
  explicit Ina226Driver(I2cBus& bus, uint8_t addr7 = 0x40) : _bus(bus), _addr(addr7) {}

  uint8_t address() const { return _addr; }
  void setAddress(uint8_t addr7) { _addr = addr7; }

  // -------- Simple API (ALERT) --------

  /** Disable ALERT output (mask/enable cleared, latch off, active-low). */
  Result alertDisable();

  /** Configure ALERT as "conversion ready" output. */
  Result alertEnableConversionReady(const AlertConfig& cfg = {});

  /** Configure ALERT for bus over-voltage (V) threshold. */
  Result alertEnableBusOverVoltage_V(float limit_V, const AlertConfig& cfg = {});

  /** Configure ALERT for bus under-voltage (V) threshold. */
  Result alertEnableBusUnderVoltage_V(float limit_V, const AlertConfig& cfg = {});

  /** Configure ALERT for shunt over-voltage (uV) threshold (proxy for over-current). */
  Result alertEnableShuntOverVoltage_uV(float limit_uV, const AlertConfig& cfg = {});

  /** Configure ALERT for power over-limit (W) threshold. Requires correct calibration. */
  Result alertEnablePowerOver_W(float limit_W, const AlertConfig& cfg = {});

  /** Read alert flags from Mask/Enable register (pro-friendly summary). */
  Result alertReadStatus(AlertStatus& out);

  // -------- Pro API (register-level) --------
  Result readMaskEnable(uint16_t& out);
  Result writeMaskEnable(uint16_t v);
  Result readAlertLimit(uint16_t& out);
  Result writeAlertLimit(uint16_t v);

  Result setAlertLatch(bool enable);
  Result setAlertPolarityActiveHigh(bool enable);

  // -------- Pro API (raw conversions helpers) --------
  static uint16_t busVoltageToAlertLimitRaw(float limit_V);
  static uint16_t shuntVoltageToAlertLimitRaw_uV(float limit_uV);

private:
  I2cBus& _bus;
  uint8_t _addr;

  // Registers (INA226 datasheet)
  static constexpr uint8_t REG_MASK_ENABLE = 0x06;
  static constexpr uint8_t REG_ALERT_LIMIT = 0x07;

  // Mask/Enable bits (common to INA226 family)
  static constexpr uint16_t MASK_SOL = 0x8000;   // Shunt over-voltage
  static constexpr uint16_t MASK_SUL = 0x4000;   // Shunt under-voltage
  static constexpr uint16_t MASK_BOL = 0x2000;   // Bus over-voltage
  static constexpr uint16_t MASK_BUL = 0x1000;   // Bus under-voltage
  static constexpr uint16_t MASK_POL = 0x0800;   // Power over-limit
  static constexpr uint16_t MASK_CNVR = 0x0400;  // Conversion ready
  static constexpr uint16_t MASK_AFF = 0x0010;   // Alert function flag (read-only)

  static constexpr uint16_t MASK_LEN = 0x0002;   // Latch enable
  static constexpr uint16_t MASK_APOL = 0x0001;  // Alert polarity (1=active high)

  Result applyAlertConfig(const AlertConfig& cfg, uint16_t baseMask);
};

}  // namespace Ina

