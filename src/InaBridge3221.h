#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "InaSerialLineReader.h"

/**
 * @brief INA3221 triple-channel sensor driver with optional JSONL serial streaming.
 *
 * Three independent measurement channels, each with bus voltage and shunt
 * voltage readings. Current is computed as shuntV / rshunt. Each channel
 * can have its own shunt resistor value and can be individually enabled
 * or disabled.
 *
 * Two usage modes — can be combined:
 *
 * **Mode A – JSONL streaming (for NiusRobotLab_INA_monitor):**
 *   Call tick() in loop(). The host sends START/STOP/SR commands over Serial
 *   and the bridge emits JSON Lines measurement data at the requested rate.
 *
 * **Mode B – Standalone direct reading (no Serial output):**
 * @code
 *   InaBridge3221 sensor("INA3221", 0x40);
 *   sensor.begin(8, 9);
 *   sensor.setRshunt(1, 0.1);   // CH1: 100 mΩ shunt
 *   sensor.setRshunt(2, 0.05);  // CH2:  50 mΩ shunt
 *   sensor.setRshunt(3, 0.01);  // CH3:  10 mΩ shunt
 *
 *   // In loop():
 *   if (sensor.dataReady()) {
 *     float v1 = sensor.readBusVoltage(1);
 *     float i1 = sensor.readCurrent(1);    // uses CH1's rshunt
 *     float i2 = sensor.readCurrent(2);    // uses CH2's rshunt
 *   }
 * @endcode
 */
class InaBridge3221 {
public:
  InaBridge3221(const char* chipJson, uint8_t i2cAddr = 0x40);

  /**
   * @brief Initialize I2C bus, probe the device, apply default configuration,
   *        and print an INFO line on Serial.
   */
  void begin(int pinSda = 8, int pinScl = 9, uint32_t i2cHz = 400000);

  /** @brief Legacy alias for begin(). */
  void beginI2c(int pinSda = 8, int pinScl = 9, uint32_t i2cHz = 400000) {
    begin(pinSda, pinScl, i2cHz);
  }

  /** @brief Process serial commands and emit JSONL samples. Call in loop(). */
  void tick();

  // ── Per-Channel Measurement (no Serial output) ──────────────────

  /**
   * @brief Read bus voltage for channel @p ch in volts (V).
   * @param ch  Channel number: 1, 2, or 3. Returns 0 if out of range.
   */
  float readBusVoltage(uint8_t ch);

  /**
   * @brief Read shunt voltage for channel @p ch in volts (V).
   * @param ch  Channel number: 1, 2, or 3.
   */
  float readShuntVoltage(uint8_t ch);

  /**
   * @brief Read current for channel @p ch in amperes (A).
   * @param ch  Channel number: 1, 2, or 3.
   * @note  Computed as shuntVoltage / rshunt(ch).
   */
  float readCurrent(uint8_t ch);

  /**
   * @brief Read power for channel @p ch in watts (W).
   * @param ch  Channel number: 1, 2, or 3.
   * @note  Computed as current(ch) × busVoltage(ch).
   */
  float readPower(uint8_t ch);

  /**
   * @brief Check if new conversion data is available (CVRF flag).
   * @note Reading the Mask/Enable register clears this flag.
   */
  bool dataReady();

  // ── Channel Control ─────────────────────────────────────────────

  /**
   * @brief Enable or disable a specific measurement channel.
   * @param ch      Channel number: 1, 2, or 3.
   * @param enable  true to enable (default), false to disable.
   *
   * Disabled channels are skipped during conversion, reducing total
   * conversion time and allowing faster sample rates.
   */
  void enableChannel(uint8_t ch, bool enable = true);

  /**
   * @brief Check if a channel is enabled.
   * @param ch  Channel number: 1, 2, or 3.
   */
  bool isChannelEnabled(uint8_t ch);

  // ── Configuration ───────────────────────────────────────────────

  void startStreaming();
  void stopStreaming();
  bool isStreaming() const { return _streaming; }
  void setSampleRate(int hz);

  /**
   * @brief Set shunt resistor value for a specific channel.
   * @param ch   Channel number: 1, 2, or 3.
   * @param ohm  Shunt resistance in ohms (must be > 0).
   */
  void setRshunt(uint8_t ch, float ohm);

  /**
   * @brief Set the same shunt resistor value for all three channels.
   * @param ohm  Shunt resistance in ohms (must be > 0).
   */
  void setRshunt(float ohm);

  /**
   * @brief Get the shunt resistor value for a specific channel.
   * @param ch  Channel number: 1, 2, or 3 (default 1).
   */
  float rshunt(uint8_t ch = 1) const;

  uint8_t address() const { return _addr; }

  // ── Extra fields callback (for advanced examples) ───────────────

  typedef void (*ExtraFieldsFn)();
  void setExtraFieldsPrinter(ExtraFieldsFn fn) { _extraFields = fn; }

private:
  const char* _chip;
  uint8_t _addr;
  float _rshuntOhm[3] = {0.1f, 0.1f, 0.1f};
  int   _sampleHz     = 10;
  bool  _streaming    = false;
  uint32_t _seq       = 0;
  uint32_t _lastMs    = 0;
  InaSerialLineReader _rx{96};
  ExtraFieldsFn _extraFields = nullptr;

  uint16_t readU16(uint8_t reg);
  int16_t  readS16(uint8_t reg);
  void     writeU16(uint8_t reg, uint16_t val);
  static float shuntRegToVolts(int16_t raw);
  static float busRegToVolts(uint16_t raw);
  float    rshuntSafe(uint8_t ch) const;
  void     applyDefaultConfig();
  void     printInfo();
  void     emitJsonSample();
  void     handleCommand(const String& line);
};
