/**
 * @file InaSerialLineReader.h
 * @brief Non-blocking Serial line reader for bridge command parsing.
 *
 * Reads bytes from Serial one at a time and assembles complete lines delimited
 * by '\n'. Lines that exceed the buffer capacity are silently discarded (the
 * entire line is dropped, not just the overflowing portion).
 */
#pragma once

#include <Arduino.h>

class InaSerialLineReader {
public:
  explicit InaSerialLineReader(size_t cap = 96) : _cap(cap) {
    if (_cap < 24) _cap = 24;
    _buf = (char*)malloc(_cap + 1);
    if (_buf) {
      _buf[0] = '\0';
    } else {
      _cap = 0;
    }
  }

  ~InaSerialLineReader() {
    if (_buf) free(_buf);
    _buf = nullptr;
    _cap = 0;
    _len = 0;
  }

  InaSerialLineReader(const InaSerialLineReader&) = delete;
  InaSerialLineReader& operator=(const InaSerialLineReader&) = delete;

  /**
   * @brief Read available bytes and return true when a full line is captured.
   * @param out  Receives the line content (excluding trailing newline).
   * @return true if a complete line was captured, false otherwise.
   *
   * Carriage returns ('\r') are silently stripped.
   * If a line exceeds the buffer, it is discarded entirely.
   */
  bool pollLine(String& out) {
    if (!_buf || _cap == 0) return false;
    while (Serial.available() > 0) {
      const int v = Serial.read();
      if (v < 0) break;
      const char c = (char)v;
      if (c == '\r') continue;
      if (c == '\n') {
        if (_overflow) {
          _overflow = false;
          _len = 0;
          _buf[0] = '\0';
          continue;       // discard the overflowed line, wait for next
        }
        _buf[_len] = '\0';
        out = String(_buf);
        _len = 0;
        _buf[0] = '\0';
        return true;
      }
      if (_len + 1 >= _cap) {
        _overflow = true;
        continue;         // keep reading until '\n' to discard the whole line
      }
      if (!_overflow) {
        _buf[_len++] = c;
        _buf[_len] = '\0';
      }
    }
    return false;
  }

private:
  char* _buf = nullptr;
  size_t _cap = 0;
  size_t _len = 0;
  bool _overflow = false;
};
