/**
 * UNKNOWN stub (no bus). 115200 — use to verify USB serial only.
 * Serial @115200: send START (newline) to stream JSON measurement lines; send STOP to stop.
 * Optional before START: SR <Hz> for nominal rate (INA Monitor).
 */
#include <INA_Series_Sensor.h>

static InaBridgeUnknown g_bridge;

void setup() {
  Serial.begin(115200);
  delay(500);
  g_bridge.begin();
}

void loop() {
  g_bridge.tick();
}
