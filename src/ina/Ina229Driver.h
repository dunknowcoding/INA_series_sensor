/**
 * @file Ina229Driver.h
 * @brief INA229/INA229-Q1 driver (SPI) with DIAG_ALRT special-pin support.
 *
 * INA229 shares the same conceptual DIAG/ALERT behavior as INA228 family, but uses SPI transport.
 */
#pragma once

#include <Arduino.h>

#include "InaSpiBus.h"
#include "InaTypes.h"

namespace Ina {

class Ina229Driver {
public:
  explicit Ina229Driver(SpiBus& bus) : _bus(bus) {}

  // -------- Simple API (measurement) --------
  Result readDieTemp_C(float& out);
  Result readEnergy_J(float& out, float currentLsb);
  Result readCharge_C(float& out, float currentLsb);
  Result resetAccumulators();

  // -------- Simple API (ALERT) --------
  Result alertDisable();
  Result alertEnableConversionReady(const AlertConfig& cfg = {});
  Result alertEnableTempOver_C(float limit_C, const AlertConfig& cfg = {});
  Result alertReadStatus(AlertStatus& out);

  // -------- Pro API --------
  Result readDiagAlrt(uint16_t& out);
  Result writeDiagAlrt(uint16_t v);

  Result readDieTempRaw(uint16_t& out);
  Result readEnergyRaw(uint64_t& out);
  Result readChargeRaw(int64_t& out);

  Result readTempLimit(uint16_t& out);
  Result writeTempLimit(uint16_t v);

  Result setAlertLatch(bool enable);
  Result setAlertPolarityActiveHigh(bool enable);

  static uint16_t tempToTempLimitRaw(float temp_C);

private:
  SpiBus& _bus;

  static constexpr uint8_t REG_CONFIG = 0x00;
  static constexpr uint8_t REG_DIETEMP = 0x06;
  static constexpr uint8_t REG_ENERGY = 0x09;
  static constexpr uint8_t REG_CHARGE = 0x0A;
  static constexpr uint8_t REG_DIAG_ALRT = 0x0B;
  static constexpr uint8_t REG_TEMP_LIMIT = 0x10;

  static constexpr uint16_t CFG_RSTACC = 0x4000;

  static constexpr uint16_t DIAG_ALATCH = 0x8000;
  static constexpr uint16_t DIAG_CNVR = 0x4000;
  static constexpr uint16_t DIAG_APOL = 0x1000;
  static constexpr uint16_t DIAG_TMPOL = 0x0080;
  static constexpr uint16_t DIAG_SHNTOL = 0x0040;
  static constexpr uint16_t DIAG_SHNTUL = 0x0020;
  static constexpr uint16_t DIAG_BUSOL = 0x0010;
  static constexpr uint16_t DIAG_BUSUL = 0x0008;
  static constexpr uint16_t DIAG_POL = 0x0004;
  static constexpr uint16_t DIAG_CNVRF = 0x0002;
};

}  // namespace Ina

