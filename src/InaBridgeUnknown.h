#pragma once

#include <Arduino.h>

/** Placeholder JSONL for chip id UNKNOWN (no sensor). */
class InaBridgeUnknown {
public:
  void begin();
  void tick();

private:
  int _sampleHz = 10;
  bool _streaming = false;
  uint32_t _seq = 0;
  uint32_t _lastMs = 0;
  void printInfo();
  void sampleOnce();
  void handleCommand(const String& line);
};
