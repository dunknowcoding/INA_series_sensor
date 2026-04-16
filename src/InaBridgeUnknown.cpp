#include "InaBridgeUnknown.h"
#include "InaJsonlProtocol.h"

// ── Initialization ───────────────────────────────────────────────

void InaBridgeUnknown::begin() {
  printInfo();
}

void InaBridgeUnknown::printInfo() {
  Serial.println(
      F("{\"v\":1,\"type\":\"INFO\",\"msg\":\"UNKNOWN stub bridge. No JSON samples until START (optional SR <Hz> "
        "first).\",\"author\":\"NiusRobotLab\",\"chip\":\"UNKNOWN\"}"));
}

// ── Configuration API ────────────────────────────────────────────

void InaBridgeUnknown::startStreaming() { _streaming = true; }
void InaBridgeUnknown::stopStreaming()  { _streaming = false; }

void InaBridgeUnknown::setSampleRate(int hz) {
  _sampleHz = InaJsonl::clampStreamRateI2c(hz);
}

// ── JSONL streaming ──────────────────────────────────────────────

void InaBridgeUnknown::emitJsonSample() {
  const uint32_t t_ms = millis();
  Serial.print(F("{\"v\":1,\"chip\":\"UNKNOWN\",\"seq\":"));
  Serial.print(_seq++);
  Serial.print(F(",\"t_ms\":")); Serial.print(t_ms);
  Serial.print(F(",\"bus_V\":")); Serial.print(0.0f, 6);
  Serial.print(F(",\"current_A\":")); Serial.print(0.0f, 6);
  Serial.print(F(",\"power_W\":")); Serial.print(0.0f, 6);
  Serial.println(F("}"));
}

// ── Serial command handling ──────────────────────────────────────

void InaBridgeUnknown::handleCommand(const String& line) {
  String cmd = line;
  InaJsonl::normalizeCmd(cmd);
  if (cmd.length() == 0) return;
  if (cmd.equalsIgnoreCase("PING")) {
    InaJsonl::pong();
    return;
  }
  if (cmd.equalsIgnoreCase("START")) {
    _streaming = true;
    InaJsonl::ackStart();
    return;
  }
  if (cmd.equalsIgnoreCase("STOP")) {
    _streaming = false;
    InaJsonl::ackStop();
    return;
  }
  if (cmd.startsWith("SR ")) {
    _sampleHz = InaJsonl::clampStreamRateI2c(cmd.substring(3).toInt());
    InaJsonl::ackSr(_sampleHz);
    return;
  }
  InaJsonl::errUnknownCmd(cmd);
}

void InaBridgeUnknown::tick() {
  String line;
  if (_rx.pollLine(line)) {
    handleCommand(line);
  }
  if (!_streaming) return;
  const uint32_t interval_ms = InaJsonl::sampleIntervalMs(_sampleHz);
  const uint32_t now = millis();
  if ((uint32_t)(now - _lastMs) >= interval_ms) {
    _lastMs = now;
    emitJsonSample();
  }
}
