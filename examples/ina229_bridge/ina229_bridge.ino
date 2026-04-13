/**
 * INA229 JSONL bridge (SPI). ESP32-C3: SCK=4 MISO=5 MOSI=6 CS=7, 115200.
 * Serial @115200: send START (newline) to stream JSON measurement lines; send STOP to stop.
 * Optional before START: SR <Hz> for nominal rate (INA Monitor).
 */
#include <INA_Series_Sensor.h>

static InaBridge229Spi g_bridge("INA229", "TI INA229");

void setup() {
  Serial.begin(115200);
  delay(500);
  g_bridge.beginSpi();
}

void loop() {
  g_bridge.tick();
}
