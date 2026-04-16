#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "InaSerialLineReader.h"

/**
 * @brief INA228-family sensor driver with optional JSONL serial streaming.
 *
 * Supports INA228, INA228-Q1, INA237, INA237-Q1, INA238, INA238-Q1,
 * INA239, INA239-Q1, INA740X. All share MFG ID 0x5449 at register 0x3E.
 *
 * Two usage modes — can be combined:
 *
 * **Mode A – JSONL streaming (for NiusRobotLab_INA_monitor):**
 *   Call tick() in loop(). The host sends START/STOP/SR commands over Serial
 *   and the bridge emits JSON Lines measurement data at the requested rate.
 *
 * **Mode B – Standalone direct reading (no Serial output):**
 *   Call readBusVoltage(), readCurrent(), readPower(), etc. from your own
 *   code to obtain measurement values. Optionally use dataReady() to poll
 *   for new conversions.
 *
 * @note When both modes are active simultaneously, dataReady() may return
 *       false between streaming samples because the streaming code reads
 *       the same status register that clears the conversion-ready flag.
 */
class InaBridge228 {
public:
  InaBridge228(const char* chipJson, uint8_t i2cAddr = 0x40, const char* ref = "TI INA228");

  /**
   * @brief Initialize I2C bus, detect the chip, reset, configure ADC mode,
   *        apply calibration, and print an INFO line on Serial.
   * @param pinSda  SDA pin (ESP32: GPIO remapping; others: ignored, uses board default).
   * @param pinScl  SCL pin (ESP32: GPIO remapping; others: ignored, uses board default).
   * @param i2cHz   I2C clock frequency in Hz (default 400 kHz).
   */
  void begin(int pinSda = 8, int pinScl = 9, uint32_t i2cHz = 400000);

  /** @brief Legacy alias for begin(). */
  void beginI2c(int pinSda = 8, int pinScl = 9, uint32_t i2cHz = 400000) {
    begin(pinSda, pinScl, i2cHz);
  }

  /**
   * @brief Process serial commands and emit JSONL samples when streaming.
   *        Call once per loop() iteration.
   */
  void tick();

  // ── Standalone Measurement API (no Serial output) ───────────────

  /** @brief Read bus voltage in volts (V). */
  float readBusVoltage();

  /** @brief Read shunt voltage in volts (V). */
  float readShuntVoltage();

  /** @brief Read current in amperes (A). Requires valid calibration. */
  float readCurrent();

  /** @brief Read power in watts (W). */
  float readPower();

  /** @brief Read die temperature in degrees Celsius (°C). */
  float readDieTemp();

  /** @brief Read accumulated energy in joules (J) since last reset. */
  float readEnergy();

  /** @brief Read accumulated charge in coulombs (C) since last reset. */
  float readCharge();

  /** @brief Reset energy and charge accumulators to zero. */
  void resetAccumulators();

  /**
   * @brief Check if new conversion data is available (CNVRF flag).
   * @note Reading the DIAG_ALRT register clears this flag, so calling
   *       dataReady() twice consecutively will return false the second time.
   */
  bool dataReady();

  // ── Configuration ───────────────────────────────────────────────

  /** @brief Start JSONL streaming (equivalent to serial "START" command). */
  void startStreaming();

  /** @brief Stop JSONL streaming (equivalent to serial "STOP" command). */
  void stopStreaming();

  /** @brief Returns true if JSONL streaming is active. */
  bool isStreaming() const { return _streaming; }

  /** @brief Set JSONL streaming sample rate in Hz (clamped to 1–400). */
  void setSampleRate(int hz);

  /**
   * @brief Set shunt resistor value in ohms and recalibrate.
   * @param ohm  Shunt resistance (minimum 0.0001 Ω).
   */
  void setRshunt(float ohm);

  /**
   * @brief Set maximum expected current in amperes and recalibrate.
   * @param ampere  Maximum current (must be > 0).
   */
  void setImax(float ampere);

  float rshunt() const { return _rshuntOhm; }
  float imax() const { return _imaxA; }
  float currentLsb() const { return _currentLsb; }
  uint8_t address() const { return _addr; }

  // ── Extra fields callback (for advanced examples) ───────────────

  typedef void (*ExtraFieldsFn)();
  void setExtraFieldsPrinter(ExtraFieldsFn fn) { _extraFields = fn; }

private:
  const char* _chip;
  const char* _ref;
  uint8_t _addr;
  float _rshuntOhm  = 0.1f;
  float _imaxA       = 10.0f;
  float _currentLsb  = 0.0f;
  int   _sampleHz    = 10;
  bool  _streaming   = false;
  bool  _adcRangeHigh = false;   // true when CONFIG bit4 ADCRANGE = 1 (±40.96 mV)
  uint32_t _seq      = 0;
  uint32_t _lastMs   = 0;
  InaSerialLineReader _rx{96};
  ExtraFieldsFn _extraFields = nullptr;

  uint16_t readU16(uint8_t reg);
  void     writeU16(uint8_t reg, uint16_t val);
  uint32_t readU24(uint8_t reg);
  void     applyShuntCalibration();
  bool     detectChip();
  void     applyResetAndMode();
  void     printInfo();
  void     emitJsonSample();
  void     handleCommand(const String& line);
};
