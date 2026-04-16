/**
 * INA233 basic example — JSONL bridge (I²C).
 * Wiring (ESP32-C3): SDA=GPIO8, SCL=GPIO9, 115200 baud.
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

static InaBridge226 sensor("INA233", 0x40, "TI SLYSF02 (INA233)");

void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.begin(8, 9);
}

void loop() {
  sensor.tick();
}
