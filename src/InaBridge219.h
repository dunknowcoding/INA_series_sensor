#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "InaSerialLineReader.h"

/**
 * @brief INA219/INA220 sensor driver with optional JSONL serial streaming.
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
 *       registers that clear the conversion-ready flag.
 */
class InaBridge219 {
public:
  InaBridge219(const char* chipJson, uint8_t i2cAddr = 0x40);

  /**
   * @brief Initialize I2C bus, configure ADC, apply calibration, and print
   *        an INFO line on Serial.
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

  /**
   * @brief Check if new conversion data is available (CNVR flag).
   * @note CNVR is bit 1 of the Bus Voltage register (0x02). Reading the
   *       Power register clears this flag, so calling dataReady() twice
   *       consecutively will return false the second time.
   */
  bool dataReady();

  // ── Configuration ───────────────────────────────────────────────

  /** @brief Start JSONL streaming (equivalent to serial "START" command). */
  void startStreaming();

  /** @brief Stop JSONL streaming (equivalent to serial "STOP" command). */
  void stopStreaming();

  /** @brief Returns true if JSONL streaming is active. */
  bool isStreaming() const { return _streaming; }

  /** @brief Set JSONL streaming sample rate in Hz (clamped to valid range). */
  void setSampleRate(int hz);

  /**
   * @brief Set shunt resistor value in ohms and recalibrate.
   * @param ohm  Shunt resistance (must be > 0).
   */
  void setRshunt(float ohm);

  /**
   * @brief Set maximum expected current in amperes and recalibrate.
   * @param ampere  Maximum current (must be > 0).
   */
  void setImax(float ampere);

  float rshunt() const { return _rshuntOhm; }
  float imax() const { return _imaxA; }
  uint8_t address() const { return _addr; }

private:
  const char* _chip;
  uint8_t _addr;
  float _rshuntOhm  = 0.1f;
  float _imaxA       = 3.2f;
  int   _sampleHz    = 10;
  bool  _streaming   = false;
  uint32_t _seq      = 0;
  uint32_t _lastMs   = 0;
  InaSerialLineReader _rx{96};

  uint16_t readU16(uint8_t reg);
  int16_t  readS16(uint8_t reg);
  void     writeU16(uint8_t reg, uint16_t val);
  void     applyCalibration();
  void     applyDefaultConfig();
  void     printInfo();
  void     emitJsonSample();
  void     handleCommand(const String& line);
};
