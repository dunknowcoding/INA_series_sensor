/**
 * @file ina229_q1_bridge.ino
 * @brief INA229-Q1 — SPI JSONL bridge (library InaBridge229Spi).
 * ESP32-C3: GPIO4=SCK, 5=MISO, 6=MOSI, 7=CS. USB 115200.
 */
#include <INA_Series_Sensor.h>

static InaBridge229Spi g_bridge("INA229-Q1", "TI INA229-Q1");

void setup() {
  Serial.begin(115200);
  delay(100);
  g_bridge.beginSpi();
}

void loop() {
  g_bridge.tick();
}
