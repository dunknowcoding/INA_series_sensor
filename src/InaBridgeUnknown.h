#pragma once

#include <Arduino.h>
#include "InaSerialLineReader.h"

/**
 * @brief Placeholder JSONL bridge for when no sensor is detected.
 *
 * Always outputs zeros for bus voltage, current, and power.
 * Accepts START/STOP/SR serial commands like other bridges so that
 * the host application does not need special-case handling.
 */
class InaBridgeUnknown {
public:
  /**
   * @brief Print an INFO line on Serial.
   */
  void begin();

  /**
   * @brief Process serial commands and emit JSONL samples when streaming.
   *        Call once per loop() iteration.
   */
  void tick();

  // ── Configuration ───────────────────────────────────────────────

  /** @brief Start JSONL streaming (equivalent to serial "START" command). */
  void startStreaming();

  /** @brief Stop JSONL streaming (equivalent to serial "STOP" command). */
  void stopStreaming();

  /** @brief Returns true if JSONL streaming is active. */
  bool isStreaming() const { return _streaming; }

  /** @brief Set JSONL streaming sample rate in Hz (clamped to 1–400). */
  void setSampleRate(int hz);

private:
  int _sampleHz = 10;
  bool _streaming = false;
  uint32_t _seq = 0;
  uint32_t _lastMs = 0;
  InaSerialLineReader _rx{64};

  void printInfo();
  void emitJsonSample();
  void handleCommand(const String& line);
};
