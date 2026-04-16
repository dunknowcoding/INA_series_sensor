#pragma once

#include <Arduino.h>
#include <SPI.h>
#include "InaSerialLineReader.h"

/**
 * @brief INA229-family SPI sensor driver with optional JSONL serial streaming.
 *
 * Same measurement math as INA228 (I2C), accessed over SPI.
 * Supports INA229 / INA229-Q1.
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
 * @note Uses micros() for timing to support sample rates up to 2000 Hz.
 * @note When both modes are active simultaneously, dataReady() may return
 *       false between streaming samples because the streaming code reads
 *       the same status register that clears the conversion-ready flag.
 */
class InaBridge229Spi {
public:
  InaBridge229Spi(const char* chipJson, const char* ref, int pinCs = 7, int pinSck = 4, int pinMiso = 5,
                  int pinMosi = 6);

  /**
   * @brief Initialize SPI bus, detect the chip, reset, configure ADC mode,
   *        apply calibration, and print an INFO line on Serial.
   * @param spiHz  SPI clock frequency in Hz (default 10 MHz).
   */
  void beginSpi(uint32_t spiHz = 10000000);

  /** @brief Alias for beginSpi(). */
  void begin(uint32_t spiHz = 10000000) { beginSpi(spiHz); }

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

  /** @brief Set JSONL streaming sample rate in Hz (clamped to 1–2000). */
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

  // ── Extra fields callback (for advanced examples) ───────────────

  typedef void (*ExtraFieldsFn)();
  void setExtraFieldsPrinter(ExtraFieldsFn fn) { _extraFields = fn; }

private:
  const char* _chip;
  const char* _ref;
  int _cs, _sck, _miso, _mosi;
  float _rshuntOhm     = 0.1f;
  float _imaxA          = 10.0f;
  float _currentLsb     = 0.0f;
  int   _sampleHz       = 10;
  bool  _streaming      = false;
  bool  _adcRangeHigh   = false;
  uint32_t _seq         = 0;
  uint32_t _lastSampleUs = 0;
  SPISettings _spiSettings;
  InaSerialLineReader _rx{96};
  ExtraFieldsFn _extraFields = nullptr;

  uint32_t spiReadRegister(uint8_t reg, uint8_t nbytes);
  void     spiWriteU16(uint8_t reg, uint16_t val);
  uint16_t readU16(uint8_t reg);
  uint32_t readU24(uint8_t reg);
  uint64_t readU40(uint8_t reg);
  void     applyShuntCalibration();
  bool     detectChip();
  void     applyResetAndMode();
  void     printInfo();
  void     emitJsonSample();
  void     handleCommand(const String& line);
};
