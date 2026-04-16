/**
 * INA3221-Q1 – Advanced demo: CRI + WAR + PV + shunt voltage summation.
 *
 * =========================================================================
 * Module compatibility
 * =========================================================================
 * Adafruit INA3221 breakout and CJMCU-3221 / MCU-3221 modules expose
 * all four special-function pins: CRI, WRN (WAR), TC, VALID (PV).
 *
 * Pin functions:
 *   CRI   — Critical alert, open-drain active-low.  Fires when any armed
 *           channel exceeds its critical-current limit.
 *   WAR   — Warning alert, open-drain active-low.  Fires when the averaged
 *           measurement exceeds the per-channel warning limit.
 *   TC    — Timing Control, open-drain active-low.
 *   PV    — Power Valid, push-pull (needs VPU connected to logic level).
 *           HIGH when bus voltage is within the programmable window.
 *
 * NOTE: The INA3221-Q1 does NOT have a temperature sensor or energy/charge
 * accumulation.  Those features are exclusive to the INA228/229 family.
 *
 * =========================================================================
 * Wiring (ESP32-C3 defaults)
 * =========================================================================
 *   Module VCC    →  3.3 V or 5 V
 *   Module GND    →  GND
 *   Module SDA    →  GPIO 8  (I2C SDA)
 *   Module SCL    →  GPIO 9  (I2C SCL)
 *   Module CRI    →  GPIO 2  (10 kΩ pull-up to VCC)
 *   Module WRN    →  GPIO 3  (10 kΩ pull-up to VCC)
 *   Module VALID  →  GPIO 4  (connect VPU to 3.3 V first)
 *   Module TC     →  (optional) GPIO 5
 *   Module IN1+/- →  Channel 1 load  (shunt resistor between + and -)
 *   Module IN2+/- →  Channel 2 load
 *   Module IN3+/- →  Channel 3 load
 *
 * =========================================================================
 * What this example does
 * =========================================================================
 * 1. Starts the 3-channel JSONL bridge (NiusRobotLab_INA_monitor compatible).
 * 2. Registers a callback to include the shunt-voltage sum (all 3 channels)
 *    as an extra field in every measurement line.
 * 3. Configures:
 *      • Critical overcurrent on CH1 (CRI pin interrupt)
 *      • Warning overcurrent on CH1  (WAR pin interrupt)
 *      • Power-valid window          (PV pin, polled in loop)
 *      • Summation alert (sum of all channels vs. a limit)
 * 4. ISR flags for CRI and WAR; loop() reads Mask/Enable and reports.
 *
 * =========================================================================
 * How to change parameters at runtime (serial commands)
 * =========================================================================
 *   START / STOP / SR <Hz> / RSHUNT <ohm> / PING
 *   CRIT <ch:1-3> <A>   — set critical overcurrent
 *   WARN <ch:1-3> <A>   — set warning overcurrent
 *   PV <Vmin> <Vmax>    — set power-valid voltage window
 *   TC                   — read Timing Control flag
 *   DIAG                 — dump Mask/Enable + PV limits
 *
 * NiusRobotLab_INA_monitor compatibility:
 *   • Measurement lines: 3-channel JSONL with extra sum_shunt_uV (ignored).
 *   • ALERT_CFG / ALERT_EVENT lines: INA_monitor ignores them.
 */
#include <INA_Series_Sensor.h>

// ---- User configuration (edit these) ------------------------------------
static const int   CRI_PIN         = 2;      // GPIO → CRI pin
static const int   WAR_PIN         = 3;      // GPIO → WRN/WAR pin
static const int   PV_PIN          = 4;      // GPIO → PV/VALID pin
static const float RSHUNT_OHM      = 0.1f;   // shunt resistor (all channels)
static const float DEFAULT_CRIT_A  = 2.0f;   // CH1 critical limit (A)
static const float DEFAULT_WARN_A  = 1.0f;   // CH1 warning limit (A)
static const float DEFAULT_PV_MIN  = 4.5f;   // PV lower bound (V)
static const float DEFAULT_PV_MAX  = 5.5f;   // PV upper bound (V)
// -------------------------------------------------------------------------

#if defined(ESP32) || defined(ESP8266)
  #define INA_ISR_ATTR IRAM_ATTR
#else
  #define INA_ISR_ATTR
#endif

static InaBridge3221      sensor("INA3221-Q1", 0x40);
static Ina::I2cBus        g_i2c;
static Ina::Ina3221Driver g_drv(g_i2c, 0x40);
static volatile bool      g_criFlag = false;
static volatile bool      g_warFlag = false;
static bool               g_lastPvState = false;

static void INA_ISR_ATTR onCri() { g_criFlag = true; }
static void INA_ISR_ATTR onWar() { g_warFlag = true; }

// Callback: append shunt-voltage sum to every measurement line.
static void printExtraFields() {
  float sum_uV;
  if (g_drv.readShuntVoltageSum_uV(sum_uV).ok()) {
    Serial.print(F(",\"sum_shunt_uV\":"));
    Serial.print(sum_uV, 1);
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.begin(8, 9);
  sensor.setExtraFieldsPrinter(printExtraFields);

  // CRI + WAR interrupts
  pinMode(CRI_PIN, INPUT_PULLUP);
  pinMode(WAR_PIN, INPUT_PULLUP);
  pinMode(PV_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(CRI_PIN), onCri, FALLING);
  attachInterrupt(digitalPinToInterrupt(WAR_PIN), onWar, FALLING);

  // Configure alerts
  g_drv.enableCriticalOverCurrent_A(1, DEFAULT_CRIT_A, RSHUNT_OHM);
  g_drv.enableWarningOverCurrent_A(1, DEFAULT_WARN_A, RSHUNT_OHM);
  g_drv.enablePowerValidWindow_V(DEFAULT_PV_MIN, DEFAULT_PV_MAX);

  // Enable summation across all 3 channels (uses CRI alert for sum limit)
  float sumLimit_uV = DEFAULT_CRIT_A * RSHUNT_OHM * 1.0e6f * 3.0f;
  g_drv.enableSummationAlert_uV(sumLimit_uV, true, true, true);

  // Info line
  Serial.print(F("{\"v\":1,\"type\":\"ALERT_CFG\",\"chip\":\"INA3221-Q1\",\"addr\":\"0x40\""));
  Serial.print(F(",\"crit_A\":"));  Serial.print(DEFAULT_CRIT_A, 2);
  Serial.print(F(",\"warn_A\":"));  Serial.print(DEFAULT_WARN_A, 2);
  Serial.print(F(",\"pv_min_V\":"));  Serial.print(DEFAULT_PV_MIN, 1);
  Serial.print(F(",\"pv_max_V\":"));  Serial.print(DEFAULT_PV_MAX, 1);
  Serial.print(F(",\"cri_pin\":"));  Serial.print(CRI_PIN);
  Serial.print(F(",\"war_pin\":"));  Serial.print(WAR_PIN);
  Serial.print(F(",\"pv_pin\":"));   Serial.print(PV_PIN);
  Serial.println(F(",\"_note\":\"INA_monitor ignores this line\"}"));
}

void loop() {
  sensor.tick();

  // ---- Critical alert ----
  if (g_criFlag) {
    g_criFlag = false;
    Ina::Ina3221MaskEnable me;
    g_drv.readMaskEnable(me);

    Serial.print(F("{\"v\":1,\"type\":\"ALERT_EVENT\",\"chip\":\"INA3221-Q1\",\"addr\":\"0x40\""));
    Serial.print(F(",\"src\":\"CRI\",\"crit\":["));
    Serial.print(me.critFlag[0] ? 1 : 0); Serial.print(',');
    Serial.print(me.critFlag[1] ? 1 : 0); Serial.print(',');
    Serial.print(me.critFlag[2] ? 1 : 0);
    Serial.print(F("],\"sumFlag\":"));
    Serial.print(me.summationFlag ? 1 : 0);
    Serial.print(F(",\"t_ms\":"));  Serial.print(millis());
    Serial.println(F(",\"_note\":\"INA_monitor ignores this line\"}"));
  }

  // ---- Warning alert ----
  if (g_warFlag) {
    g_warFlag = false;
    Ina::Ina3221MaskEnable me;
    g_drv.readMaskEnable(me);

    Serial.print(F("{\"v\":1,\"type\":\"ALERT_EVENT\",\"chip\":\"INA3221-Q1\",\"addr\":\"0x40\""));
    Serial.print(F(",\"src\":\"WAR\",\"warn\":["));
    Serial.print(me.warnFlag[0] ? 1 : 0); Serial.print(',');
    Serial.print(me.warnFlag[1] ? 1 : 0); Serial.print(',');
    Serial.print(me.warnFlag[2] ? 1 : 0);
    Serial.print(F("],\"t_ms\":"));  Serial.print(millis());
    Serial.println(F(",\"_note\":\"INA_monitor ignores this line\"}"));
  }

  // ---- Power-valid polling (PV is push-pull, not interrupt) ----
  bool pvNow = digitalRead(PV_PIN) == HIGH;
  if (pvNow != g_lastPvState) {
    g_lastPvState = pvNow;
    Serial.print(F("{\"v\":1,\"type\":\"PV_EVENT\",\"chip\":\"INA3221-Q1\",\"addr\":\"0x40\""));
    Serial.print(F(",\"power_valid\":"));  Serial.print(pvNow ? 1 : 0);
    Serial.print(F(",\"t_ms\":"));         Serial.print(millis());
    Serial.println(F(",\"_note\":\"INA_monitor ignores this line\"}"));
  }
}
