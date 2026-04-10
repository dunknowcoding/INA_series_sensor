/**
 * @file ina4230_bridge.ino
 * @brief INA4230 — JSONL bridge using library class (see INA_Series_Sensor).
 * ESP32-C3 SuperMini: GPIO8=SDA, GPIO9=SCL, 3V3/GND. USB 115200.
 */
#include <INA_Series_Sensor.h>

static InaBridgeCh1 g_bridge("INA4230", "INA4230 bridge ready (CH1)", 0x40);

void setup() {
  Serial.begin(115200);
  delay(100);
  g_bridge.beginI2c(8, 9);
}

void loop() {
  g_bridge.tick();
}
