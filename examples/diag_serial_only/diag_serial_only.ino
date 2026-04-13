/**
 * USB serial smoke test (no I²C). Upload if bridge sketches show nothing @115200.
 * Try each COM port; close INA Monitor; data-capable USB cable; USB CDC On Boot per board.
 * Bridge examples: send START to stream JSON lines, STOP to stop (see *_bridge.ino headers).
 */
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println(F("diag_serial_only OK"));
  Serial.flush();
}

void loop() {
  Serial.print(F("tick "));
  Serial.println(millis());
  delay(500);
}
