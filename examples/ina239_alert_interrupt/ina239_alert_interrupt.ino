/**
 * INA239 – ALERT interrupt demo (bus over-voltage).
 *
 * =========================================================================
 * Module compatibility
 * =========================================================================
 * Adafruit INA228 (#5832, STEMMA QT) exposes the ALRT pin on the header.
 * Other INA239 modules may label it "ALERT" or "INT".
 *
 * =========================================================================
 * Wiring (ESP32-C3 defaults)
 * =========================================================================
 *   Module VIN / VCC  ->  3.3 V or 5 V
 *   Module GND        ->  GND
 *   Module SDA        ->  GPIO 8  (I2C SDA)
 *   Module SCL        ->  GPIO 9  (I2C SCL)
 *   Module ALRT       ->  GPIO 2  (open-drain, add 10 k pull-up to VCC)
 *   Module VIN+ / V+  ->  load high-side (power supply +)
 *   Module VIN- / V-  ->  load positive terminal
 *
 * =========================================================================
 * What this example does
 * =========================================================================
 * 1. Starts the JSONL bridge (NiusRobotLab_INA_monitor compatible).
 * 2. Configures a bus-over-voltage threshold (DEFAULT_BOV_V).
 * 3. Attaches an MCU interrupt to ALERT_PIN (falling edge).
 * 4. When bus voltage > threshold, ALERT fires (75 us response) -> ISR
 *    sets flag -> loop() reads alert status and prints ALERT_EVENT.
 *
 * =========================================================================
 * Additional INA228-family capabilities (beyond this demo)
 * =========================================================================
 * The INA228 digital family has DEDICATED threshold registers that allow
 * multiple simultaneous conditions (unlike INA226 which uses a single
 * Alert Limit register):
 *   - BOVL / BUVL  — bus over/under-voltage  (regs 0x0E / 0x0F)
 *   - SOVL / SUVL  — shunt over/under-voltage (regs 0x0C / 0x0D)
 *   - TEMP_LIMIT   — die temperature limit     (reg  0x10)
 *   - PWR_LIMIT    — power limit               (reg  0x11)
 *
 * The chip also has an integrated temperature sensor (±1 C accuracy).
 * For multi-threshold or temperature alerting, use the Pro API:
 *   g_drv.writeBovl(raw);  g_drv.writeTempLimit(raw);
 *   See Ina228Driver.h for all Pro methods.
 *
 * =========================================================================
 * How to change parameters at runtime (serial commands)
 * =========================================================================
 *   ALERT BOV <V>    — bus over-voltage   (e.g. ALERT BOV 5.5)
 *   ALERT BUV <V>    — bus under-voltage  (e.g. ALERT BUV 3.0)
 *   ALERT SOV <uV>   — shunt over-voltage / overcurrent proxy
 *   ALERT OFF         — disable alert
 *   DIAG              — read DIAG_ALRT + BOVL/BUVL/SOVL/PWR_LIMIT registers
 *   START / STOP / SR <Hz> / RSHUNT <ohm> / IMAX <A> / PING
 *
 * NiusRobotLab_INA_monitor compatibility:
 *   Measurement lines: standard JSONL (bus_V, current_A, power_W).
 *   ALERT_CFG / ALERT_EVENT are placeholders — INA_monitor ignores them.
 */
#include <INA_Series_Sensor.h>

// ---- User configuration (edit these) ------------------------------------
static const int   ALERT_PIN      = 2;      // GPIO wired to ALRT / ALERT
static const float DEFAULT_BOV_V  = 5.5f;   // bus over-voltage threshold (V)
// -------------------------------------------------------------------------

#if defined(ESP32) || defined(ESP8266)
  #define INA_ISR_ATTR IRAM_ATTR
#else
  #define INA_ISR_ATTR
#endif

static InaBridge228 sensor("INA239", 0x40, "TI INA239");
static Ina::I2cBus     g_i2c;
static Ina::Ina228Driver g_drv(g_i2c, 0x40);
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

  Serial.print(F("{\"v\":1,\"type\":\"ALERT_CFG\",\"chip\":\"INA239\",\"addr\":\"0x40\""));
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

    Serial.print(F("{\"v\":1,\"type\":\"ALERT_EVENT\",\"chip\":\"INA239\",\"addr\":\"0x40\""));
    Serial.print(F(",\"asserted\":"));    Serial.print(st.asserted ? 1 : 0);
    Serial.print(F(",\"overvoltage\":")); Serial.print(st.overVoltage ? 1 : 0);
    Serial.print(F(",\"undervoltage\":"));Serial.print(st.underVoltage ? 1 : 0);
    Serial.print(F(",\"overcurrent\":"));  Serial.print(st.overCurrent ? 1 : 0);
    Serial.print(F(",\"overpower\":"));    Serial.print(st.overPower ? 1 : 0);
    Serial.print(F(",\"t_ms\":"));         Serial.print(millis());
    Serial.println(F(",\"_note\":\"INA_monitor does not display this line\"}"));
  }
}
