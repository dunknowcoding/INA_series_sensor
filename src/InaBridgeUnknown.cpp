#include "InaBridgeUnknown.h"
#include "InaJsonlProtocol.h"

void InaBridgeUnknown::begin() {
  printInfo();
}

void InaBridgeUnknown::printInfo() {
  Serial.println(
      "{\"v\":1,\"type\":\"INFO\",\"msg\":\"UNKNOWN stub bridge\",\"author\":\"NiusRobotLab\",\"chip\":\"UNKNOWN\"}");
}

void InaBridgeUnknown::sampleOnce() {
  const uint32_t t_ms = millis();
  Serial.print("{\"v\":1,\"chip\":\"UNKNOWN\",\"seq\":");
  Serial.print(_seq++);
  Serial.print(",\"t_ms\":"); Serial.print(t_ms);
  Serial.print(",\"bus_V\":"); Serial.print(0.0f, 6);
  Serial.print(",\"current_A\":"); Serial.print(0.0f, 6);
  Serial.print(",\"power_W\":"); Serial.print(0.0f, 6);
  Serial.println("}");
}

void InaBridgeUnknown::handleCommand(const String& line) {
  String l = line;
  l.trim();
  if (l.length() == 0) return;
  if (l.equalsIgnoreCase("PING")) {
    InaJsonl::pong();
    return;
  }
  if (l.equalsIgnoreCase("START")) {
    _streaming = true;
    InaJsonl::ackStart();
    return;
  }
  if (l.equalsIgnoreCase("STOP")) {
    _streaming = false;
    InaJsonl::ackStop();
    return;
  }
  if (l.startsWith("SR ")) {
    int hz = l.substring(3).toInt();
    if (hz < 1) hz = 1;
    if (hz > 200) hz = 200;
    _sampleHz = hz;
    InaJsonl::ackSr(_sampleHz);
    return;
  }
  InaJsonl::errUnknownCmd(l);
}

void InaBridgeUnknown::tick() {
  if (Serial.available()) {
    handleCommand(Serial.readStringUntil('\n'));
  }
  const uint32_t now = millis();
  const uint32_t interval_ms = InaJsonl::sampleIntervalMs(_sampleHz);
  if (_streaming && (now - _lastMs >= interval_ms)) {
    _lastMs = now;
    sampleOnce();
  }
}
