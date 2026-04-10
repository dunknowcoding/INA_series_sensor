/**
 * @file unknown_bridge.ino
 * @brief UNKNOWN chip placeholder (InaBridgeUnknown).
 */
#include <INA_Series_Sensor.h>

static InaBridgeUnknown g_bridge;

void setup() {
  Serial.begin(115200);
  delay(100);
  g_bridge.begin();
}

void loop() {
  g_bridge.tick();
}
