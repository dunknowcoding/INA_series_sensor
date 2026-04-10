/**
 * @file ina232_bridge.ino
 * @brief INA232 — JSONL bridge using library class (see INA_Series_Sensor).
 * ESP32-C3 SuperMini: GPIO8=SDA, GPIO9=SCL, 3V3/GND. USB 115200.
 */
#include <INA_Series_Sensor.h>

static InaBridge226 g_bridge("INA232", 0x40, "TI SLYSF02 (INA232)");

void setup() {
  Serial.begin(115200);
  delay(100);
  g_bridge.beginI2c(8, 9);
}

void loop() {
  g_bridge.tick();
}
