/**
 * @file Ina219Driver.h
 * @brief INA219/INA220 driver with ALERT support (simple + pro).
 */
#pragma once

#include <Arduino.h>

#include "InaI2cBus.h"
#include "InaTypes.h"

namespace Ina {

class Ina219Driver {
public:
  explicit Ina219Driver(I2cBus& bus, uint8_t addr7 = 0x40) : _bus(bus), _addr(addr7) {}

  uint8_t address() const { return _addr; }
  void setAddress(uint8_t addr7) { _addr = addr7; }

  // -------- Simple API (ALERT) --------
  Result alertDisable();
  Result alertEnableConversionReady(const AlertConfig& cfg = {});
  Result alertEnableBusOverVoltage_V(float limit_V, const AlertConfig& cfg = {});
  Result alertEnableBusUnderVoltage_V(float limit_V, const AlertConfig& cfg = {});
  Result alertEnableShuntOverVoltage_uV(float limit_uV, const AlertConfig& cfg = {});

  Result alertReadStatus(AlertStatus& out);

  // -------- Pro API --------
  Result readMaskEnable(uint16_t& out);
  Result writeMaskEnable(uint16_t v);
  Result readAlertLimit(uint16_t& out);
  Result writeAlertLimit(uint16_t v);
  Result setAlertLatch(bool enable);
  Result setAlertPolarityActiveHigh(bool enable);

  // Conversions (alert limit register uses the same format as the selected alert function; see datasheet)
  static uint16_t busVoltageToAlertLimitRaw(float limit_V);
  static uint16_t shuntVoltageToAlertLimitRaw_uV(float limit_uV);

private:
  I2cBus& _bus;
  uint8_t _addr;

  static constexpr uint8_t REG_MASK_ENABLE = 0x06;
  static constexpr uint8_t REG_ALERT_LIMIT = 0x07;

  // INA219 Mask/Enable bits
  static constexpr uint16_t MASK_SOL = 0x8000;
  static constexpr uint16_t MASK_SUL = 0x4000;
  static constexpr uint16_t MASK_BOL = 0x2000;
  static constexpr uint16_t MASK_BUL = 0x1000;
  static constexpr uint16_t MASK_POL = 0x0800;
  static constexpr uint16_t MASK_CNVR = 0x0400;
  static constexpr uint16_t MASK_AFF = 0x0010;
  static constexpr uint16_t MASK_LEN = 0x0002;
  static constexpr uint16_t MASK_APOL = 0x0001;

  Result applyAlertConfig(const AlertConfig& cfg, uint16_t baseMask);
};

}  // namespace Ina

