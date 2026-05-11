#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "InaSerialLineReader.h"

/**
 * @brief CH1-only sensor driver for INA2227 / INA4230 / INA4235 with
 *        optional JSONL serial streaming.
 *
 * Simple two-register path: shunt voltage at 0x01 (signed field in bits
 * [15:3], LSB = 40 µV) and bus voltage at 0x02 ((raw >> 3) & 0x1FFF,
 * LSB = 8 mV).
 * Current is computed in software: shuntV / Rshunt.
 *
 * Two usage modes — can be combined:
 *
 * **Mode A – JSONL streaming (for NiusRobotLab_INA_monitor):**
 *   Call tick() in loop(). The host sends START/STOP/SR commands over Serial
 *   and the bridge emits JSON Lines measurement data at the requested rate.
 *
 * **Mode B – Standalone direct reading (no Serial output):**
 *   Call readBusVoltage(), readCurrent(), readPower(), etc. from your own
 *   code to obtain measurement values.
 *
 * @note These chips have no calibration register and no documented
 *       conversion-ready flag, so dataReady() is not provided.
 */
class InaBridgeCh1 {
public:
  InaBridgeCh1(const char* chipJson, const char* infoMsg, uint8_t i2cAddr = 0x40);

  /**
   * @brief Initialize I2C bus, probe the chip, apply default config,
   *        and print an INFO line on Serial.
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

  /** @brief Read current in amperes (A). Uses software division: shuntV / Rshunt. */
  float readCurrent();

  /** @brief Read power in watts (W). Computed as current × busV. */
  float readPower();

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
   * @brief Set shunt resistor value in ohms.
   * @param ohm  Shunt resistance (minimum 0.0001 Ω; values below are clamped to 0.1 Ω).
   */
  void setRshunt(float ohm);

  /** @brief Get current shunt resistance in ohms. */
  float rshunt() const { return _rshuntOhm; }

  /** @brief Get I2C address. */
  uint8_t address() const { return _addr; }

private:
  const char* _chip;
  const char* _infoMsg;
  uint8_t _addr;
  float _rshuntOhm = 0.1f;
  int _sampleHz = 10;
  bool _streaming = false;
  uint32_t _seq = 0;
  uint32_t _lastMs = 0;
  InaSerialLineReader _rx{96};

  uint16_t readU16(uint8_t reg);
  int16_t readS16(uint8_t reg);
  void writeU16(uint8_t reg, uint16_t val);
  static float shuntRegToVolts(int16_t raw);
  static float busRegToVolts(uint16_t raw);
  void applyDefaultConfig();
  void printInfo();
  void emitJsonSample();
  void handleCommand(const String& line);
};
