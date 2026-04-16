/**
 * INA234 – ALERT interrupt demo (bus over-voltage).
 *
 * =========================================================================
 * Module compatibility
 * =========================================================================
 * The CJMCU-226 module exposes the ALE (ALERT) pin on its 8-pin header:
 *   IN+ | IN- | VBS | ALE | SDA | SCL | GND | VCC
 * Other INA226 modules may also break out ALERT; check your board's
 * silkscreen or schematic.
 *
 * =========================================================================
 * Wiring (ESP32-C3 defaults)
 * =========================================================================
 *   Module VCC  ->  3.3 V or 5 V
 *   Module GND  ->  GND
 *   Module SDA  ->  GPIO 8  (I2C SDA)
 *   Module SCL  ->  GPIO 9  (I2C SCL)
 *   Module ALE  ->  GPIO 2  (open-drain, add 10 k pull-up to VCC)
 *   Module IN+  ->  load high-side (power supply +)
 *   Module IN-  ->  load positive terminal
 *
 * =========================================================================
 * What this example does
 * =========================================================================
 * 1. Starts the JSONL bridge (NiusRobotLab_INA_monitor compatible).
 * 2. Configures a bus-over-voltage threshold (DEFAULT_BOV_V).
 * 3. Attaches an MCU interrupt to ALERT_PIN (falling edge).
 * 4. When bus voltage > threshold, the chip asserts ALERT -> ISR flag ->
 *    loop() reads status and prints a JSONL ALERT_EVENT placeholder.
 *
 * =========================================================================
 * How to change parameters at runtime (serial commands)
 * =========================================================================
 *   ALERT BOV <V>    — bus over-voltage   (e.g. ALERT BOV 5.5)
 *   ALERT BUV <V>    — bus under-voltage  (e.g. ALERT BUV 3.0)
 *   ALERT SOV <uV>   — shunt over-voltage / overcurrent proxy
 *   ALERT OFF         — disable alert
 *   DIAG              — read Mask/Enable + Alert Limit registers
 *   START / STOP / SR <Hz> / RSHUNT <ohm> / IMAX <A> / PING
 *
 * Available alert modes (only one active at a time):
 *   BOV = bus over-voltage     BUV = bus under-voltage
 *   SOV = shunt over-voltage   CNVR = conversion ready
 *   (POL = power over-limit — declared in datasheet but returns
 *    Unsupported in this driver; use the Pro API if needed.)
 *
 * NiusRobotLab_INA_monitor compatibility:
 *   Measurement lines: standard JSONL (bus_V, shunt_uV, current_A, power_W).
 *   ALERT_CFG / ALERT_EVENT lines are placeholders — INA_monitor ignores them.
 */
#include <INA_Series_Sensor.h>

// ---- User configuration (edit these) ------------------------------------
static const int   ALERT_PIN      = 2;      // GPIO wired to ALE / ALERT
static const float DEFAULT_BOV_V  = 5.5f;   // bus over-voltage threshold (V)
// -------------------------------------------------------------------------

#if defined(ESP32) || defined(ESP8266)
  #define INA_ISR_ATTR IRAM_ATTR
#else
  #define INA_ISR_ATTR
#endif

static InaBridge226 sensor("INA234", 0x40, "TI SLYSF02 (INA234)");
static Ina::I2cBus     g_i2c;
static Ina::Ina226Driver g_drv(g_i2c, 0x40);
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

  Serial.print(F("{\"v\":1,\"type\":\"ALERT_CFG\",\"chip\":\"INA234\",\"addr\":\"0x40\""));
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

    Serial.print(F("{\"v\":1,\"type\":\"ALERT_EVENT\",\"chip\":\"INA234\",\"addr\":\"0x40\""));
    Serial.print(F(",\"asserted\":"));    Serial.print(st.asserted ? 1 : 0);
    Serial.print(F(",\"overvoltage\":")); Serial.print(st.overVoltage ? 1 : 0);
    Serial.print(F(",\"undervoltage\":"));Serial.print(st.underVoltage ? 1 : 0);
    Serial.print(F(",\"overcurrent\":"));  Serial.print(st.overCurrent ? 1 : 0);
    Serial.print(F(",\"overpower\":"));    Serial.print(st.overPower ? 1 : 0);
    Serial.print(F(",\"t_ms\":"));         Serial.print(millis());
    Serial.println(F(",\"_note\":\"INA_monitor does not display this line\"}"));
  }
}
