/**
 * UNKNOWN stub (no sensor). Outputs zeroes. Use to verify USB serial only.
 * Send START over Serial to begin streaming; STOP to pause.
 */
#include <INA_Series_Sensor.h>

static InaBridgeUnknown sensor;

void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.begin();
}

void loop() {
  sensor.tick();
}
