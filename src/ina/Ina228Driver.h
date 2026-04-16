/**
 * @file Ina228Driver.h
 * @brief INA228 digital family driver (INA228/INA237/INA238/INA239/INA740x).
 *
 * Special pin: ALERT (open-drain) controlled by DIAG_ALRT + threshold registers.
 */
#pragma once

#include <Arduino.h>

#include "InaI2cBus.h"
#include "InaTypes.h"

namespace Ina {

class Ina228Driver {
public:
  explicit Ina228Driver(I2cBus& bus, uint8_t addr7 = 0x40) : _bus(bus), _addr(addr7) {}

  uint8_t address() const { return _addr; }
  void setAddress(uint8_t addr7) { _addr = addr7; }

  // -------- Simple API (measurement) --------
  Result readDieTemp_C(float& out);
  Result readEnergy_J(float& out, float currentLsb);
  Result readCharge_C(float& out, float currentLsb);
  Result resetAccumulators();

  // -------- Simple API (ALERT) --------
  Result alertDisable();
  Result alertEnableConversionReady(const AlertConfig& cfg = {});
  Result alertEnableBusOverVoltage_V(float limit_V, const AlertConfig& cfg = {});
  Result alertEnableBusUnderVoltage_V(float limit_V, const AlertConfig& cfg = {});
  Result alertEnableShuntOverVoltage_uV(float limit_uV, const AlertConfig& cfg = {});
  Result alertEnableTempOver_C(float limit_C, const AlertConfig& cfg = {});

  Result alertReadStatus(AlertStatus& out);

  // -------- Pro API (register-level) --------
  Result readDiagAlrt(uint16_t& out);
  Result writeDiagAlrt(uint16_t v);

  Result readBovl(uint16_t& out);
  Result writeBovl(uint16_t v);
  Result readBuvl(uint16_t& out);
  Result writeBuvl(uint16_t v);
  Result readSovl(uint16_t& out);
  Result writeSovl(uint16_t v);
  Result readSuvl(uint16_t& out);
  Result writeSuvl(uint16_t v);
  Result readTempLimit(uint16_t& out);
  Result writeTempLimit(uint16_t v);
  Result readPowerLimit(uint16_t& out);
  Result writePowerLimit(uint16_t v);

  Result readDieTempRaw(uint16_t& out);
  Result readEnergyRaw(uint64_t& out);
  Result readChargeRaw(int64_t& out);

  // Helpers: convert physical units to raw threshold encodings.
  // Datasheet: BOVL/BUVL = 3.125 mV/LSB (signed? table shows reserved bit15=0, so use unsigned magnitude).
  static uint16_t busVoltageToBovlRaw(float limit_V);
  // Datasheet: SOVL = 5 uV/LSB when ADCRANGE=0, 1.25 uV/LSB when ADCRANGE=1 (two's complement).
  static uint16_t shuntVoltageToSovlRaw_uV(float limit_uV, bool adcRangeHigh);
  static uint16_t tempToTempLimitRaw_cC(float temp_C);

  // Pro: set ALATCH/APOL bits inside DIAG_ALRT.
  Result setAlertLatch(bool enable);
  Result setAlertPolarityActiveHigh(bool enable);

private:
  I2cBus& _bus;
  uint8_t _addr;

  static constexpr uint8_t REG_CONFIG = 0x00;
  static constexpr uint8_t REG_DIETEMP = 0x06;
  static constexpr uint8_t REG_ENERGY = 0x09;
  static constexpr uint8_t REG_CHARGE = 0x0A;
  static constexpr uint8_t REG_DIAG_ALRT = 0x0B;
  static constexpr uint8_t REG_SOVL = 0x0C;
  static constexpr uint8_t REG_SUVL = 0x0D;
  static constexpr uint8_t REG_BOVL = 0x0E;
  static constexpr uint8_t REG_BUVL = 0x0F;
  static constexpr uint8_t REG_TEMP_LIMIT = 0x10;
  static constexpr uint8_t REG_PWR_LIMIT = 0x11;

  static constexpr uint16_t CFG_RSTACC = 0x4000;  // CONFIG bit 14: reset accumulators

  // DIAG_ALRT bits (INA228 Table 7-16)
  static constexpr uint16_t DIAG_ALATCH = 0x8000;
  static constexpr uint16_t DIAG_CNVR = 0x4000;
  static constexpr uint16_t DIAG_SLOWALERT = 0x2000;
  static constexpr uint16_t DIAG_APOL = 0x1000;
  static constexpr uint16_t DIAG_ENERGYOF = 0x0800;
  static constexpr uint16_t DIAG_CHARGEOF = 0x0400;
  static constexpr uint16_t DIAG_MATHOF = 0x0200;
  static constexpr uint16_t DIAG_TMPOL = 0x0080;
  static constexpr uint16_t DIAG_SHNTOL = 0x0040;
  static constexpr uint16_t DIAG_SHNTUL = 0x0020;
  static constexpr uint16_t DIAG_BUSOL = 0x0010;
  static constexpr uint16_t DIAG_BUSUL = 0x0008;
  static constexpr uint16_t DIAG_POL = 0x0004;
  static constexpr uint16_t DIAG_CNVRF = 0x0002;
  static constexpr uint16_t DIAG_MEMSTAT = 0x0001;

  Result applyAlertConfig(const AlertConfig& cfg, bool enableConversionReady);
};

}  // namespace Ina

