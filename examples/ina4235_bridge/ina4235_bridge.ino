/**
 * INA4235 JSONL bridge (I²C). ESP32-C3: SDA=8 SCL=9, 115200. See README if Serial is empty.
 * Serial @115200: send START (newline) to stream JSON measurement lines; send STOP to stop.
 * Optional before START: SR <Hz> for nominal rate (INA Monitor).
 */
#include <INA_Series_Sensor.h>

static InaBridgeCh1 g_bridge("INA4235", "INA4235 bridge ready (CH1)", 0x40);

void setup() {
  Serial.begin(115200);
  delay(500);
  g_bridge.beginI2c(8, 9);
}

void loop() {
  g_bridge.tick();
}
