/**
 * INA219 – ALERT interrupt demo (bus over-voltage).
 *
 * =========================================================================
 * IMPORTANT – Module compatibility
 * =========================================================================
 * Most common INA219 breakout boards (Adafruit INA219 #904, GY-219, generic
 * "CJMCU-219") do NOT expose the ALERT pin on headers.  ALERT is IC pin 3
 * (SOT-23-8 / MSOP-8 package) and directly connected to the chip only.
 *
 * To use this example you must either:
 *   (a) solder a fly-wire from IC pin 3 to a free GPIO, or
 *   (b) use a module / custom PCB that breaks out the ALERT trace.
 *
 * If your module does not expose ALERT, you can still configure alert
 * thresholds via serial commands (ALERT BOV / BUV / SOV) — the bridge
 * writes the chip registers, and you can poll the Alert Function Flag (AFF)
 * by sending the DIAG command.
 *
 * =========================================================================
 * Wiring (ESP32-C3 defaults, when ALERT is accessible)
 * =========================================================================
 *   Module VCC  ->  3.3 V or 5 V
 *   Module GND  ->  GND
 *   Module SDA  ->  GPIO 8  (I2C SDA)
 *   Module SCL  ->  GPIO 9  (I2C SCL)
 *   Module/IC ALERT (pin 3, open-drain, active-low) -> GPIO 2 + 10 k pull-up
 *   Module IN+  ->  load high-side (power supply +)
 *   Module IN-  ->  load positive terminal
 *
 * =========================================================================
 * What this example does
 * =========================================================================
 * 1. Starts the JSONL bridge (NiusRobotLab_INA_monitor compatible).
 * 2. Configures a bus-over-voltage threshold (DEFAULT_BOV_V) on the chip.
 * 3. Attaches an MCU interrupt to ALERT_PIN (falling edge).
 * 4. When the measured bus voltage exceeds the threshold the chip pulls
 *    ALERT low -> ISR sets a flag -> loop() reads the alert status register
 *    and prints a JSONL ALERT_EVENT line to Serial.
 *
 * =========================================================================
 * How to change parameters at runtime (serial commands)
 * =========================================================================
 *   ALERT BOV <V>    — set bus over-voltage threshold  (e.g. ALERT BOV 5.5)
 *   ALERT BUV <V>    — set bus under-voltage threshold (e.g. ALERT BUV 3.0)
 *   ALERT SOV <uV>   — set shunt over-voltage (overcurrent proxy, in uV)
 *   ALERT OFF         — disable alert
 *   DIAG              — read Mask/Enable + Alert Limit registers
 *   START / STOP      — start / stop JSONL measurement stream
 *   SR <Hz>           — set sample rate (1–400)
 *   RSHUNT <ohm>      — set shunt resistance (e.g. RSHUNT 0.1)
 *   IMAX <A>          — set max expected current  (e.g. IMAX 3.2)
 *
 * Available alert modes (only one active at a time):
 *   BOV  = bus over-voltage    BUV = bus under-voltage
 *   SOV  = shunt over-voltage  CNVR = conversion ready
 *
 * NiusRobotLab_INA_monitor compatibility:
 *   Measurement lines use the standard JSONL format (bus_V, shunt_uV,
 *   current_A, power_W).  ALERT_CFG / ALERT_EVENT lines are placeholders
 *   that INA_monitor silently ignores (no measurement fields).
 */
#include <INA_Series_Sensor.h>

// ---- User configuration (edit these) ------------------------------------
static const int   ALERT_PIN      = 2;      // GPIO wired to IC ALERT output
static const float DEFAULT_BOV_V  = 5.5f;   // bus over-voltage threshold (V)
// -------------------------------------------------------------------------

#if defined(ESP32) || defined(ESP8266)
  #define INA_ISR_ATTR IRAM_ATTR
#else
  #define INA_ISR_ATTR
#endif

static InaBridge219 sensor("INA219", 0x40);
static Ina::I2cBus     g_i2c;
static Ina::Ina219Driver g_drv(g_i2c, 0x40);
static volatile bool   alertTriggered = false;

static void INA_ISR_ATTR onAlert() { alertTriggered = true; }

void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.begin(8, 9);

  pinMode(ALERT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ALERT_PIN), onAlert, FALLING);

  Ina::AlertConfig cfg;
  cfg.latch = true;
  g_drv.alertEnableBusOverVoltage_V(DEFAULT_BOV_V, cfg);

  // Placeholder — INA_monitor has no alert-config UI; this line is silently dropped.
  Serial.print(F("{\"v\":1,\"type\":\"ALERT_CFG\",\"chip\":\"INA219\",\"addr\":\"0x40\""));
  Serial.print(F(",\"mode\":\"BOV\",\"threshold_V\":"));
  Serial.print(DEFAULT_BOV_V, 3);
  Serial.print(F(",\"pin\":"));
  Serial.print(ALERT_PIN);
  Serial.println(F(",\"_note\":\"INA_monitor does not display this line\"}"));
}

void loop() {
  sensor.tick();

  if (alertTriggered) {
    alertTriggered = false;
    Ina::AlertStatus st;
    g_drv.alertReadStatus(st);

    // Placeholder — INA_monitor ignores lines without bus_V/current_A/power_W.
    Serial.print(F("{\"v\":1,\"type\":\"ALERT_EVENT\",\"chip\":\"INA219\",\"addr\":\"0x40\""));
    Serial.print(F(",\"asserted\":"));    Serial.print(st.asserted ? 1 : 0);
    Serial.print(F(",\"overvoltage\":")); Serial.print(st.overVoltage ? 1 : 0);
    Serial.print(F(",\"undervoltage\":"));Serial.print(st.underVoltage ? 1 : 0);
    Serial.print(F(",\"overcurrent\":"));  Serial.print(st.overCurrent ? 1 : 0);
    Serial.print(F(",\"t_ms\":"));         Serial.print(millis());
    Serial.println(F(",\"_note\":\"INA_monitor does not display this line\"}"));
  }
}
