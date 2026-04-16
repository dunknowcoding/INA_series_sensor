/**
 * INA3221 – CRI (critical overcurrent) interrupt demo.
 *
 * =========================================================================
 * Module compatibility
 * =========================================================================
 * Adafruit INA3221 breakout exposes all 4 special pins:
 *   WRN (Warning) | CRI (Critical) | TC (Timing Control) | VALID (Power Valid)
 * CJMCU-3221 / MCU-3221 modules also break out these pins.
 *
 * The INA3221 has 4 independent special output pins:
 *   CRI   — Critical alert: fires when any armed channel exceeds its
 *           critical-current limit.  Open-drain, active-low.
 *   WAR   — Warning alert: fires when averaged measurement on any armed
 *           channel exceeds its warning-current limit.  Open-drain, active-low.
 *   TC    — Timing Control: signals if power supplies do not power up in the
 *           correct sequence.  Open-drain, active-low.
 *   PV    — Power Valid: goes HIGH when bus voltage is within the
 *           programmable [Vmin, Vmax] window.  Requires the VPU pin to be
 *           connected to your logic level (3.3 V / 5 V) for the output to work.
 *
 * =========================================================================
 * Wiring (ESP32-C3 defaults)
 * =========================================================================
 *   Module VCC    ->  3.3 V or 5 V
 *   Module GND    ->  GND
 *   Module SDA    ->  GPIO 8  (I2C SDA)
 *   Module SCL    ->  GPIO 9  (I2C SCL)
 *   Module CRI    ->  GPIO 2  (10 k pull-up to VCC)  [this example]
 *   Module WRN    ->  (optional) GPIO 3 for warning-level alerts
 *   Module VALID  ->  (optional) GPIO 4 — connect VPU to 3.3 V first
 *   Module TC     ->  (optional) GPIO 5
 *   Module IN1+/- ->  Channel 1 load        (shunt resistor between + and -)
 *   Module IN2+/- ->  Channel 2 load
 *   Module IN3+/- ->  Channel 3 load
 *
 * =========================================================================
 * What this example does
 * =========================================================================
 * 1. Starts the 3-channel JSONL bridge (NiusRobotLab_INA_monitor compatible).
 * 2. Configures a critical-overcurrent threshold on channel 1 using the
 *    shunt resistance (DEFAULT_CRIT_A * RSHUNT_OHM -> shunt voltage limit).
 * 3. Attaches an MCU interrupt to CRI_PIN (falling edge).
 * 4. When current on CH1 exceeds the threshold, the CRI pin goes low ->
 *    ISR flag -> loop() reads the Mask/Enable register and prints an
 *    ALERT_EVENT line showing per-channel Critical and Warning flags.
 *
 * =========================================================================
 * How to change parameters at runtime (serial commands)
 * =========================================================================
 *   CRIT <ch:1-3> <A>   — set critical overcurrent (e.g. CRIT 1 2.0)
 *   WARN <ch:1-3> <A>   — set warning overcurrent  (e.g. WARN 2 1.5)
 *   PV <Vmin> <Vmax>    — set power-valid voltage window (e.g. PV 4.5 5.5)
 *   TC                   — read Timing Control flag
 *   DIAG                 — dump Mask/Enable + PV limit registers
 *   START / STOP / SR <Hz> / RSHUNT <ohm> / PING
 *
 * NiusRobotLab_INA_monitor compatibility:
 *   Measurement lines: standard 3-channel JSONL (bus_V, channels[]).
 *   ALERT_CFG / ALERT_EVENT are placeholders — INA_monitor ignores them.
 */
#include <INA_Series_Sensor.h>

// ---- User configuration (edit these) ------------------------------------
static const int   CRI_PIN           = 2;      // GPIO wired to CRI output
static const float DEFAULT_CRIT_A    = 1.0f;   // critical current threshold (A)
static const float RSHUNT_OHM        = 0.1f;   // shunt resistor value (ohm)
// -------------------------------------------------------------------------

#if defined(ESP32) || defined(ESP8266)
  #define INA_ISR_ATTR IRAM_ATTR
#else
  #define INA_ISR_ATTR
#endif

static InaBridge3221   sensor("INA3221", 0x40);
static Ina::I2cBus       g_i2c;
static Ina::Ina3221Driver  g_drv(g_i2c, 0x40);
static volatile bool     g_criFlag = false;

static void INA_ISR_ATTR onCri() { g_criFlag = true; }

void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.begin(8, 9);

  pinMode(CRI_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(CRI_PIN), onCri, FALLING);

  g_drv.enableCriticalOverCurrent_A(1, DEFAULT_CRIT_A, RSHUNT_OHM);

  Serial.print(F("{\"v\":1,\"type\":\"ALERT_CFG\",\"chip\":\"INA3221\",\"addr\":\"0x40\""));
  Serial.print(F(",\"mode\":\"CRI\",\"ch\":1,\"threshold_A\":"));
  Serial.print(DEFAULT_CRIT_A, 3);
  Serial.print(F(",\"rshunt_ohm\":"));
  Serial.print(RSHUNT_OHM, 4);
  Serial.print(F(",\"pin\":"));
  Serial.print(CRI_PIN);
  Serial.println(F(",\"_note\":\"INA_monitor does not display this line\"}"));
}

void loop() {
  sensor.tick();

  if (g_criFlag) {
    g_criFlag = false;
    Ina::Ina3221MaskEnable me;
    g_drv.readMaskEnable(me);

    Serial.print(F("{\"v\":1,\"type\":\"ALERT_EVENT\",\"chip\":\"INA3221\",\"addr\":\"0x40\""));
    Serial.print(F(",\"crit\":["));
    Serial.print(me.critFlag[0] ? 1 : 0); Serial.print(',');
    Serial.print(me.critFlag[1] ? 1 : 0); Serial.print(',');
    Serial.print(me.critFlag[2] ? 1 : 0);
    Serial.print(F("],\"warn\":["));
    Serial.print(me.warnFlag[0] ? 1 : 0); Serial.print(',');
    Serial.print(me.warnFlag[1] ? 1 : 0); Serial.print(',');
    Serial.print(me.warnFlag[2] ? 1 : 0);
    Serial.print(F("],\"powerValid\":"));
    Serial.print(me.powerValidFlag ? 1 : 0);
    Serial.print(F(",\"t_ms\":"));  Serial.print(millis());
    Serial.println(F(",\"_note\":\"INA_monitor does not display this line\"}"));
  }
}
