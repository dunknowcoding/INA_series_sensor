/**
 * INA237 – Advanced demo: temperature + energy/charge + multi-threshold alert.
 *
 * =========================================================================
 * Module compatibility
 * =========================================================================
 * Adafruit INA228 (#5832, STEMMA QT) exposes ALRT pin on header.
 * CJMCU / generic modules may label it "ALERT" or "INT".
 * The INA237 has an integrated die temperature sensor (±1 °C accuracy)
 * and 40-bit energy/charge accumulation registers.
 *
 * =========================================================================
 * Wiring (ESP32-C3 defaults)
 * =========================================================================
 *   Module VCC  →  3.3 V or 5 V
 *   Module GND  →  GND
 *   Module SDA  →  GPIO 8  (I2C SDA)
 *   Module SCL  →  GPIO 9  (I2C SCL)
 *   Module ALRT →  GPIO 2  (open-drain, 10 kΩ pull-up to VCC)
 *   Module V+   →  load high-side (power supply +)
 *   Module V-   →  load positive terminal
 *
 * =========================================================================
 * What this example does (all features combined)
 * =========================================================================
 * 1. Starts the JSONL bridge (NiusRobotLab_INA_monitor compatible).
 * 2. Registers a callback so every measurement line includes "temp_C".
 *    INA_monitor ignores the extra field; the bridge's bus_V / current_A /
 *    power_W continue to display normally.
 * 3. Configures two simultaneous alert thresholds:
 *      • Bus over-voltage (BOVL)
 *      • Die temperature over-limit (TEMP_LIMIT)
 *    Attaches an MCU interrupt to the ALERT pin.
 * 4. Resets the energy/charge accumulators and prints a periodic report
 *    (every ENERGY_REPORT_MS) as an info JSONL line that INA_monitor ignores.
 * 5. When ALERT fires, the ISR flag triggers loop() to read DIAG_ALRT and
 *    print an ALERT_EVENT line.
 *
 * =========================================================================
 * Threshold registers (INA228 family)
 * =========================================================================
 * Unlike INA226 (single Alert Limit register), the INA228 family has
 * DEDICATED threshold registers that allow multiple simultaneous conditions:
 *   BOVL / BUVL  — bus over/under-voltage  (0x0E / 0x0F)
 *   SOVL / SUVL  — shunt over/under-voltage (0x0C / 0x0D)
 *   TEMP_LIMIT   — die temperature limit     (0x10)
 *   PWR_LIMIT    — power limit               (0x11)
 *
 * =========================================================================
 * How to change parameters at runtime (serial commands)
 * =========================================================================
 *   START / STOP      — start / stop JSONL measurement stream
 *   SR <Hz>           — sample rate (1–400)
 *   RSHUNT <ohm>      — shunt resistance    (e.g. RSHUNT 0.1)
 *   IMAX <A>          — max expected current (e.g. IMAX 3.2)
 *   ALERT BOV <V>     — bus over-voltage     (e.g. ALERT BOV 5.5)
 *   ALERT BUV <V>     — bus under-voltage    (e.g. ALERT BUV 3.0)
 *   ALERT SOV <uV>    — shunt over-voltage
 *   ALERT OFF          — disable alert
 *   DIAG               — dump DIAG_ALRT + all threshold registers
 *   PING               — connectivity check
 *
 * NiusRobotLab_INA_monitor compatibility:
 *   • Measurement lines: standard JSONL with extra temp_C field (ignored).
 *   • ALERT_CFG / ALERT_EVENT / ENERGY lines: INA_monitor ignores them.
 */
#include <INA_Series_Sensor.h>

// ---- User configuration (edit these) ------------------------------------
static const int     ALERT_PIN         = 2;       // GPIO → ALRT pin
static const float   DEFAULT_BOV_V     = 5.5f;    // bus over-voltage (V)
static const float   DEFAULT_TEMP_LIM  = 85.0f;   // die temperature limit (°C)
static const uint32_t ENERGY_REPORT_MS = 5000;     // energy/charge report interval
// -------------------------------------------------------------------------

#if defined(ESP32) || defined(ESP8266)
  #define INA_ISR_ATTR IRAM_ATTR
#else
  #define INA_ISR_ATTR
#endif

static InaBridge228      sensor("INA237", 0x40, "TI INA237");
static Ina::I2cBus       g_i2c;
static Ina::Ina228Driver g_drv(g_i2c, 0x40);
static volatile bool     alertTriggered = false;
static uint32_t          g_lastEnergyMs = 0;

static void INA_ISR_ATTR onAlert() { alertTriggered = true; }

// Callback: bridge calls this before closing each measurement JSON line.
// Appends die temperature so INA_monitor still sees bus_V/current_A/power_W.
static void printExtraFields() {
  float temp;
  if (g_drv.readDieTemp_C(temp).ok()) {
    Serial.print(F(",\"temp_C\":"));
    Serial.print(temp, 2);
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.begin(8, 9);
  sensor.setExtraFieldsPrinter(printExtraFields);

  // --- Alert: bus over-voltage + temperature over-limit (simultaneous) ---
  pinMode(ALERT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ALERT_PIN), onAlert, FALLING);

  Ina::AlertConfig cfg;
  cfg.latch = true;
  g_drv.alertEnableBusOverVoltage_V(DEFAULT_BOV_V, cfg);
  g_drv.alertEnableTempOver_C(DEFAULT_TEMP_LIM, cfg);

  // Reset energy/charge accumulators to start fresh
  g_drv.resetAccumulators();
  g_lastEnergyMs = millis();

  // Info line (INA_monitor ignores)
  Serial.print(F("{\"v\":1,\"type\":\"ALERT_CFG\",\"chip\":\"INA237\",\"addr\":\"0x40\""));
  Serial.print(F(",\"bov_V\":"));       Serial.print(DEFAULT_BOV_V, 1);
  Serial.print(F(",\"temp_lim_C\":"));  Serial.print(DEFAULT_TEMP_LIM, 1);
  Serial.print(F(",\"pin\":"));          Serial.print(ALERT_PIN);
  Serial.println(F(",\"_note\":\"INA_monitor ignores this line\"}"));
}

void loop() {
  sensor.tick();

  // ---- Alert event handling ----
  if (alertTriggered) {
    alertTriggered = false;
    Ina::AlertStatus st;
    g_drv.alertReadStatus(st);

    Serial.print(F("{\"v\":1,\"type\":\"ALERT_EVENT\",\"chip\":\"INA237\",\"addr\":\"0x40\""));
    Serial.print(F(",\"overvoltage\":"));  Serial.print(st.overVoltage ? 1 : 0);
    Serial.print(F(",\"undervoltage\":"));  Serial.print(st.underVoltage ? 1 : 0);
    Serial.print(F(",\"overcurrent\":"));  Serial.print(st.overCurrent ? 1 : 0);
    Serial.print(F(",\"overpower\":"));    Serial.print(st.overPower ? 1 : 0);
    Serial.print(F(",\"overtemp\":"));     Serial.print(st.overTemp ? 1 : 0);
    Serial.print(F(",\"t_ms\":"));         Serial.print(millis());
    Serial.println(F(",\"_note\":\"INA_monitor ignores this line\"}"));
  }

  // ---- Periodic energy/charge report ----
  uint32_t now = millis();
  if ((now - g_lastEnergyMs) >= ENERGY_REPORT_MS) {
    g_lastEnergyMs = now;
    float clsb = sensor.currentLsb();
    if (clsb > 0.0f) {
      float energy_J = 0, charge_C = 0;
      g_drv.readEnergy_J(energy_J, clsb);
      g_drv.readCharge_C(charge_C, clsb);

      Serial.print(F("{\"v\":1,\"type\":\"ENERGY\",\"chip\":\"INA237\",\"addr\":\"0x40\""));
      Serial.print(F(",\"energy_J\":"));  Serial.print(energy_J, 6);
      Serial.print(F(",\"charge_C\":"));  Serial.print(charge_C, 6);
      Serial.print(F(",\"t_ms\":"));      Serial.print(now);
      Serial.println(F(",\"_note\":\"INA_monitor ignores this line\"}"));
    }
  }
}
