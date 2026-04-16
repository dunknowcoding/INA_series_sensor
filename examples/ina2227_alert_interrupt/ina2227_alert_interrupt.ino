/**
 * INA2227 – ALERT interrupt placeholder.
 *
 * =========================================================================
 * Module compatibility
 * =========================================================================
 * The INA2227 is a 2-channel power monitor (1.7–5.5 V supply, up to 48 V
 * common-mode).  As of 2025, no widely-available breakout boards exist;
 * custom PCBs are the typical platform.  The chip has:
 *   ALERT pin  — configurable for overcurrent, undercurrent, overvoltage,
 *                undervoltage conditions (per datasheet).
 *   EN pin     — hardware enable; pull low to enter <50 nA shutdown.
 *
 * =========================================================================
 * Wiring (ESP32-C3 defaults)
 * =========================================================================
 *   INA2227 VCC     ->  1.7–5.5 V
 *   INA2227 GND     ->  GND
 *   INA2227 SDA     ->  GPIO 8  (I2C SDA)
 *   INA2227 SCL     ->  GPIO 9  (I2C SCL)
 *   INA2227 ALERT   ->  GPIO 2  (open-drain, add 10 k pull-up to VCC)
 *   INA2227 EN      ->  VCC  (or GPIO for on/off control)
 *   INA2227 CBYPASS ->  0.1 uF cap to GND
 *   INA2227 INP1/INM1 .. INP2/INM2 -> per-channel shunt inputs
 *
 * =========================================================================
 * What this example does
 * =========================================================================
 * 1. Starts the JSONL bridge (NiusRobotLab_INA_monitor compatible, CH1 only).
 * 2. Attaches an MCU interrupt to ALERT_PIN.
 * 3. When the ALERT pin fires, loop() outputs a generic ALERT_EVENT.
 *
 * NOTE: The InaCh1Driver does not yet provide high-level alert methods for
 * INA2227.  To actually configure an alert threshold, you must write the
 * chip-specific registers using raw I2C:
 *
 *   Ina::I2cBus bus;
 *   Ina::InaCh1Driver drv(bus, 0x40);
 *   // Example: write an overcurrent threshold to a register (see datasheet)
 *   // drv.writeU16(REG_OC_LIMIT_CH1, rawThreshold);
 *   // drv.writeU16(REG_ALERT_CONFIG, enableBits);
 *
 * =========================================================================
 * Runtime serial commands
 * =========================================================================
 *   START / STOP / SR <Hz> / RSHUNT <ohm> / PING
 *   (No ALERT / DIAG commands — not yet implemented in bridge for CH1 family.)
 *
 * NiusRobotLab_INA_monitor compatibility:
 *   Measurement lines: standard JSONL (bus_V, current_A, power_W).
 *   ALERT_CFG / ALERT_EVENT are placeholders — INA_monitor ignores them.
 */
#include <INA_Series_Sensor.h>

// ---- User configuration (edit these) ------------------------------------
static const int ALERT_PIN = 2;   // GPIO wired to chip ALERT output
// -------------------------------------------------------------------------

#if defined(ESP32) || defined(ESP8266)
  #define INA_ISR_ATTR IRAM_ATTR
#else
  #define INA_ISR_ATTR
#endif

static InaBridgeCh1 sensor("INA2227", "INA2227 bridge ready (CH1)", 0x40);
static volatile bool  alertTriggered = false;

static void INA_ISR_ATTR onAlert() { alertTriggered = true; }

void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.begin(8, 9);

  pinMode(ALERT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ALERT_PIN), onAlert, FALLING);

  Serial.print(F("{\"v\":1,\"type\":\"ALERT_CFG\",\"chip\":\"INA2227\",\"addr\":\"0x40\""));
  Serial.print(F(",\"mode\":\"NONE\",\"pin\":"));
  Serial.print(ALERT_PIN);
  Serial.println(F(",\"_note\":\"Alert API not yet implemented for INA2227; configure via raw register writes per datasheet\"}"));
}

void loop() {
  sensor.tick();

  if (alertTriggered) {
    alertTriggered = false;

    Serial.print(F("{\"v\":1,\"type\":\"ALERT_EVENT\",\"chip\":\"INA2227\",\"addr\":\"0x40\""));
    Serial.print(F(",\"t_ms\":"));  Serial.print(millis());
    Serial.println(F(",\"_note\":\"Raw alert detected; decode per INA2227 datasheet\"}"));
  }
}
