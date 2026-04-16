/**
 * INA229 basic example — JSONL bridge (SPI).
 * Wiring (ESP32-C3): SCK=GPIO4, MISO=GPIO5, MOSI=GPIO6, CS=GPIO7, 115200 baud.
 *
 * Usage with NiusRobotLab_INA_monitor:
 *   Send START over Serial to begin streaming; STOP to pause.
 *   Optional: SR <Hz> to set sample rate before START.
 *
 * Standalone (no INA_monitor):
 *   You can also call sensor.readBusVoltage(), .readCurrent(), etc.
 *   directly in loop() to obtain measurements without JSONL output.
 */
#include <INA_Series_Sensor.h>

static InaBridge229Spi sensor("INA229", "TI INA229");

void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.beginSpi();
}

void loop() {
  sensor.tick();
}
