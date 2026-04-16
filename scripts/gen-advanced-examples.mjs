/**
 * Generates comprehensive advanced examples that combine multiple high-level
 * features for INA chips that support them:
 *
 *   INA228 family (I2C): temperature + energy/charge + multi-threshold alert
 *   INA229 family (SPI): temperature + energy/charge + alert
 *   INA3221 family:      CRI/WAR/PV alerts + shunt voltage summation
 *
 * Run:  node scripts/gen-advanced-examples.mjs
 */
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const EXAMPLES = path.join(__dirname, "..", "examples");

function addrHex(addr) {
  return "0x" + addr.toString(16).toUpperCase().padStart(2, "0");
}

// ---------------------------------------------------------------------------
// Spec tables
// ---------------------------------------------------------------------------
const SPECS_228 = [
  { dir: "ina228_advanced", chip: "INA228",    bridgeArgs: '"INA228", 0x40',                     addr: 0x40 },
  { dir: "ina228_q1_advanced", chip: "INA228-Q1", bridgeArgs: '"INA228-Q1", 0x40, "TI INA228-Q1"', addr: 0x40 },
  { dir: "ina237_advanced", chip: "INA237",    bridgeArgs: '"INA237", 0x40, "TI INA237"',         addr: 0x40 },
  { dir: "ina237_q1_advanced", chip: "INA237-Q1", bridgeArgs: '"INA237-Q1", 0x40, "TI INA237-Q1"', addr: 0x40 },
  { dir: "ina238_advanced", chip: "INA238",    bridgeArgs: '"INA238", 0x40, "TI INA238"',         addr: 0x40 },
  { dir: "ina238_q1_advanced", chip: "INA238-Q1", bridgeArgs: '"INA238-Q1", 0x40, "TI INA238-Q1"', addr: 0x40 },
  { dir: "ina239_advanced", chip: "INA239",    bridgeArgs: '"INA239", 0x40, "TI INA239"',         addr: 0x40 },
  { dir: "ina239_q1_advanced", chip: "INA239-Q1", bridgeArgs: '"INA239-Q1", 0x40, "TI INA239-Q1"', addr: 0x40 },
  { dir: "ina740x_advanced", chip: "INA740X",  bridgeArgs: '"INA740X", 0x40, "TI INA740X"',       addr: 0x40 },
];

const SPECS_229 = [
  { dir: "ina229_advanced",    chip: "INA229",    ref: "TI INA229" },
  { dir: "ina229_q1_advanced", chip: "INA229-Q1", ref: "TI INA229-Q1" },
];

const SPECS_3221 = [
  { dir: "ina3221_advanced",    chip: "INA3221",    bridgeArgs: '"INA3221", 0x40',    addr: 0x40 },
  { dir: "ina3221_q1_advanced", chip: "INA3221-Q1", bridgeArgs: '"INA3221-Q1", 0x40', addr: 0x40 },
];

// ---------------------------------------------------------------------------
// INA228 family advanced template (I2C)
// ---------------------------------------------------------------------------
function ino228Advanced(s) {
  const ah = addrHex(s.addr);
  return `\
/**
 * ${s.chip} \u2013 Advanced demo: temperature + energy/charge + multi-threshold alert.
 *
 * =========================================================================
 * Module compatibility
 * =========================================================================
 * Adafruit INA228 (#5832, STEMMA QT) exposes ALRT pin on header.
 * CJMCU / generic modules may label it "ALERT" or "INT".
 * The ${s.chip} has an integrated die temperature sensor (\xB11 \xB0C accuracy)
 * and 40-bit energy/charge accumulation registers.
 *
 * =========================================================================
 * Wiring (ESP32-C3 defaults)
 * =========================================================================
 *   Module VCC  \u2192  3.3 V or 5 V
 *   Module GND  \u2192  GND
 *   Module SDA  \u2192  GPIO 8  (I2C SDA)
 *   Module SCL  \u2192  GPIO 9  (I2C SCL)
 *   Module ALRT \u2192  GPIO 2  (open-drain, 10 k\u03A9 pull-up to VCC)
 *   Module V+   \u2192  load high-side (power supply +)
 *   Module V-   \u2192  load positive terminal
 *
 * =========================================================================
 * What this example does (all features combined)
 * =========================================================================
 * 1. Starts the JSONL bridge (NiusRobotLab_INA_monitor compatible).
 * 2. Registers a callback so every measurement line includes "temp_C".
 *    INA_monitor ignores the extra field; the bridge's bus_V / current_A /
 *    power_W continue to display normally.
 * 3. Configures two simultaneous alert thresholds:
 *      \u2022 Bus over-voltage (BOVL)
 *      \u2022 Die temperature over-limit (TEMP_LIMIT)
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
 *   BOVL / BUVL  \u2014 bus over/under-voltage  (0x0E / 0x0F)
 *   SOVL / SUVL  \u2014 shunt over/under-voltage (0x0C / 0x0D)
 *   TEMP_LIMIT   \u2014 die temperature limit     (0x10)
 *   PWR_LIMIT    \u2014 power limit               (0x11)
 *
 * =========================================================================
 * How to change parameters at runtime (serial commands)
 * =========================================================================
 *   START / STOP      \u2014 start / stop JSONL measurement stream
 *   SR <Hz>           \u2014 sample rate (1\u2013400)
 *   RSHUNT <ohm>      \u2014 shunt resistance    (e.g. RSHUNT 0.1)
 *   IMAX <A>          \u2014 max expected current (e.g. IMAX 3.2)
 *   ALERT BOV <V>     \u2014 bus over-voltage     (e.g. ALERT BOV 5.5)
 *   ALERT BUV <V>     \u2014 bus under-voltage    (e.g. ALERT BUV 3.0)
 *   ALERT SOV <uV>    \u2014 shunt over-voltage
 *   ALERT OFF          \u2014 disable alert
 *   DIAG               \u2014 dump DIAG_ALRT + all threshold registers
 *   PING               \u2014 connectivity check
 *
 * NiusRobotLab_INA_monitor compatibility:
 *   \u2022 Measurement lines: standard JSONL with extra temp_C field (ignored).
 *   \u2022 ALERT_CFG / ALERT_EVENT / ENERGY lines: INA_monitor ignores them.
 */
#include <INA_Series_Sensor.h>

// ---- User configuration (edit these) ------------------------------------
static const int     ALERT_PIN         = 2;       // GPIO \u2192 ALRT pin
static const float   DEFAULT_BOV_V     = 5.5f;    // bus over-voltage (V)
static const float   DEFAULT_TEMP_LIM  = 85.0f;   // die temperature limit (\xB0C)
static const uint32_t ENERGY_REPORT_MS = 5000;     // energy/charge report interval
// -------------------------------------------------------------------------

#if defined(ESP32) || defined(ESP8266)
  #define INA_ISR_ATTR IRAM_ATTR
#else
  #define INA_ISR_ATTR
#endif

static InaBridge228      g_bridge(${s.bridgeArgs});
static Ina::I2cBus       g_i2c;
static Ina::Ina228Driver g_drv(g_i2c, ${ah});
static volatile bool     g_alertFlag = false;
static uint32_t          g_lastEnergyMs = 0;

static void INA_ISR_ATTR onAlert() { g_alertFlag = true; }

// Callback: bridge calls this before closing each measurement JSON line.
// Appends die temperature so INA_monitor still sees bus_V/current_A/power_W.
static void printExtraFields() {
  float temp;
  if (g_drv.readDieTemp_C(temp).ok()) {
    Serial.print(F(",\\"temp_C\\":"));
    Serial.print(temp, 2);
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  g_bridge.beginI2c(8, 9);
  g_bridge.setExtraFieldsPrinter(printExtraFields);

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
  Serial.print(F("{\\"v\\":1,\\"type\\":\\"ALERT_CFG\\",\\"chip\\":\\"${s.chip}\\",\\"addr\\":\\"${ah}\\""));
  Serial.print(F(",\\"bov_V\\":"));       Serial.print(DEFAULT_BOV_V, 1);
  Serial.print(F(",\\"temp_lim_C\\":"));  Serial.print(DEFAULT_TEMP_LIM, 1);
  Serial.print(F(",\\"pin\\":"));          Serial.print(ALERT_PIN);
  Serial.println(F(",\\"_note\\":\\"INA_monitor ignores this line\\"}"));
}

void loop() {
  g_bridge.tick();

  // ---- Alert event handling ----
  if (g_alertFlag) {
    g_alertFlag = false;
    Ina::AlertStatus st;
    g_drv.alertReadStatus(st);

    Serial.print(F("{\\"v\\":1,\\"type\\":\\"ALERT_EVENT\\",\\"chip\\":\\"${s.chip}\\",\\"addr\\":\\"${ah}\\""));
    Serial.print(F(",\\"overvoltage\\":"));  Serial.print(st.overVoltage ? 1 : 0);
    Serial.print(F(",\\"undervoltage\\":"));  Serial.print(st.underVoltage ? 1 : 0);
    Serial.print(F(",\\"overcurrent\\":"));  Serial.print(st.overCurrent ? 1 : 0);
    Serial.print(F(",\\"overpower\\":"));    Serial.print(st.overPower ? 1 : 0);
    Serial.print(F(",\\"overtemp\\":"));     Serial.print(st.overTemp ? 1 : 0);
    Serial.print(F(",\\"t_ms\\":"));         Serial.print(millis());
    Serial.println(F(",\\"_note\\":\\"INA_monitor ignores this line\\"}"));
  }

  // ---- Periodic energy/charge report ----
  uint32_t now = millis();
  if ((now - g_lastEnergyMs) >= ENERGY_REPORT_MS) {
    g_lastEnergyMs = now;
    float clsb = g_bridge.currentLsb();
    if (clsb > 0.0f) {
      float energy_J = 0, charge_C = 0;
      g_drv.readEnergy_J(energy_J, clsb);
      g_drv.readCharge_C(charge_C, clsb);

      Serial.print(F("{\\"v\\":1,\\"type\\":\\"ENERGY\\",\\"chip\\":\\"${s.chip}\\",\\"addr\\":\\"${ah}\\""));
      Serial.print(F(",\\"energy_J\\":"));  Serial.print(energy_J, 6);
      Serial.print(F(",\\"charge_C\\":"));  Serial.print(charge_C, 6);
      Serial.print(F(",\\"t_ms\\":"));      Serial.print(now);
      Serial.println(F(",\\"_note\\":\\"INA_monitor ignores this line\\"}"));
    }
  }
}
`;
}

// ---------------------------------------------------------------------------
// INA229 SPI advanced template
// ---------------------------------------------------------------------------
function ino229Advanced(s) {
  return `\
/**
 * ${s.chip} \u2013 Advanced demo (SPI): temperature + energy/charge + alert.
 *
 * =========================================================================
 * Module compatibility
 * =========================================================================
 * No widely-available breakout boards exist for the ${s.chip} as of 2025.
 * TI sells the INA229_239EVM evaluation module.  ALERT is chip pin 10.
 * The ${s.chip} has an integrated die temperature sensor and 40-bit
 * energy/charge accumulation, identical to the INA228 I2C family.
 *
 * =========================================================================
 * Wiring (ESP32-C3 defaults)
 * =========================================================================
 *   ${s.chip} CS     \u2192  GPIO 7
 *   ${s.chip} SCLK   \u2192  GPIO 4
 *   ${s.chip} MISO   \u2192  GPIO 5
 *   ${s.chip} MOSI   \u2192  GPIO 6
 *   ${s.chip} ALERT  \u2192  GPIO 2  (open-drain, 10 k\u03A9 pull-up)
 *   ${s.chip} VS     \u2192  3.3 V or 5 V
 *   ${s.chip} GND    \u2192  GND
 *   ${s.chip} INP    \u2192  load high-side     INM \u2192 load positive terminal
 *
 * =========================================================================
 * What this example does
 * =========================================================================
 * 1. Starts the SPI JSONL bridge (NiusRobotLab_INA_monitor compatible).
 * 2. Registers a callback to append die temperature to every measurement.
 * 3. Enables conversion-ready (CNVR) alert + temperature over-limit.
 * 4. Resets energy/charge accumulators; prints periodic report.
 * 5. Alert ISR \u2192 ALERT_EVENT with full status flags.
 *
 * =========================================================================
 * How to change parameters at runtime (serial commands)
 * =========================================================================
 *   START / STOP / SR <Hz> / RSHUNT <ohm> / IMAX <A> / PING
 *   ALERT CNVR / ALERT OFF
 *   DIAG
 */
#include <INA_Series_Sensor.h>

// ---- User configuration ----
static const int     ALERT_PIN         = 2;
static const float   DEFAULT_TEMP_LIM  = 85.0f;
static const uint32_t ENERGY_REPORT_MS = 5000;
// ----------------------------

#if defined(ESP32) || defined(ESP8266)
  #define INA_ISR_ATTR IRAM_ATTR
#else
  #define INA_ISR_ATTR
#endif

static InaBridge229Spi   g_bridge("${s.chip}", "${s.ref}");
static Ina::SpiBus       g_spi(7, 4, 5, 6, SPISettings(10000000, MSBFIRST, SPI_MODE1));
static Ina::Ina229Driver g_drv(g_spi);
static volatile bool     g_alertFlag = false;
static uint32_t          g_lastEnergyMs = 0;

static void INA_ISR_ATTR onAlert() { g_alertFlag = true; }

static void printExtraFields() {
  float temp;
  if (g_drv.readDieTemp_C(temp).ok()) {
    Serial.print(F(",\\"temp_C\\":"));
    Serial.print(temp, 2);
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  g_bridge.beginSpi();
  g_bridge.setExtraFieldsPrinter(printExtraFields);

  pinMode(ALERT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ALERT_PIN), onAlert, FALLING);

  Ina::AlertConfig cfg;
  cfg.latch = true;
  g_drv.alertEnableConversionReady(cfg);
  g_drv.alertEnableTempOver_C(DEFAULT_TEMP_LIM, cfg);
  g_drv.resetAccumulators();
  g_lastEnergyMs = millis();

  Serial.print(F("{\\"v\\":1,\\"type\\":\\"ALERT_CFG\\",\\"chip\\":\\"${s.chip}\\",\\"addr\\":\\"SPI\\""));
  Serial.print(F(",\\"mode\\":\\"CNVR+TEMP\\",\\"temp_lim_C\\":"));
  Serial.print(DEFAULT_TEMP_LIM, 1);
  Serial.print(F(",\\"pin\\":"));
  Serial.print(ALERT_PIN);
  Serial.println(F(",\\"_note\\":\\"INA_monitor ignores this line\\"}"));
}

void loop() {
  g_bridge.tick();

  if (g_alertFlag) {
    g_alertFlag = false;
    Ina::AlertStatus st;
    g_drv.alertReadStatus(st);

    Serial.print(F("{\\"v\\":1,\\"type\\":\\"ALERT_EVENT\\",\\"chip\\":\\"${s.chip}\\",\\"addr\\":\\"SPI\\""));
    Serial.print(F(",\\"convReady\\":"));    Serial.print(st.conversionReady ? 1 : 0);
    Serial.print(F(",\\"overvoltage\\":"));  Serial.print(st.overVoltage ? 1 : 0);
    Serial.print(F(",\\"overcurrent\\":"));  Serial.print(st.overCurrent ? 1 : 0);
    Serial.print(F(",\\"overtemp\\":"));     Serial.print(st.overTemp ? 1 : 0);
    Serial.print(F(",\\"t_ms\\":"));         Serial.print(millis());
    Serial.println(F(",\\"_note\\":\\"INA_monitor ignores this line\\"}"));
  }

  uint32_t now = millis();
  if ((now - g_lastEnergyMs) >= ENERGY_REPORT_MS) {
    g_lastEnergyMs = now;
    float clsb = g_bridge.currentLsb();
    if (clsb > 0.0f) {
      float energy_J = 0, charge_C = 0;
      g_drv.readEnergy_J(energy_J, clsb);
      g_drv.readCharge_C(charge_C, clsb);

      Serial.print(F("{\\"v\\":1,\\"type\\":\\"ENERGY\\",\\"chip\\":\\"${s.chip}\\",\\"addr\\":\\"SPI\\""));
      Serial.print(F(",\\"energy_J\\":"));  Serial.print(energy_J, 6);
      Serial.print(F(",\\"charge_C\\":"));  Serial.print(charge_C, 6);
      Serial.print(F(",\\"t_ms\\":"));      Serial.print(now);
      Serial.println(F(",\\"_note\\":\\"INA_monitor ignores this line\\"}"));
    }
  }
}
`;
}

// ---------------------------------------------------------------------------
// INA3221 advanced template
// ---------------------------------------------------------------------------
function ino3221Advanced(s) {
  const ah = addrHex(s.addr);
  return `\
/**
 * ${s.chip} \u2013 Advanced demo: CRI + WAR + PV + shunt voltage summation.
 *
 * =========================================================================
 * Module compatibility
 * =========================================================================
 * Adafruit INA3221 breakout and CJMCU-3221 / MCU-3221 modules expose
 * all four special-function pins: CRI, WRN (WAR), TC, VALID (PV).
 *
 * Pin functions:
 *   CRI   \u2014 Critical alert, open-drain active-low.  Fires when any armed
 *           channel exceeds its critical-current limit.
 *   WAR   \u2014 Warning alert, open-drain active-low.  Fires when the averaged
 *           measurement exceeds the per-channel warning limit.
 *   TC    \u2014 Timing Control, open-drain active-low.
 *   PV    \u2014 Power Valid, push-pull (needs VPU connected to logic level).
 *           HIGH when bus voltage is within the programmable window.
 *
 * NOTE: The ${s.chip} does NOT have a temperature sensor or energy/charge
 * accumulation.  Those features are exclusive to the INA228/229 family.
 *
 * =========================================================================
 * Wiring (ESP32-C3 defaults)
 * =========================================================================
 *   Module VCC    \u2192  3.3 V or 5 V
 *   Module GND    \u2192  GND
 *   Module SDA    \u2192  GPIO 8  (I2C SDA)
 *   Module SCL    \u2192  GPIO 9  (I2C SCL)
 *   Module CRI    \u2192  GPIO 2  (10 k\u03A9 pull-up to VCC)
 *   Module WRN    \u2192  GPIO 3  (10 k\u03A9 pull-up to VCC)
 *   Module VALID  \u2192  GPIO 4  (connect VPU to 3.3 V first)
 *   Module TC     \u2192  (optional) GPIO 5
 *   Module IN1+/- \u2192  Channel 1 load  (shunt resistor between + and -)
 *   Module IN2+/- \u2192  Channel 2 load
 *   Module IN3+/- \u2192  Channel 3 load
 *
 * =========================================================================
 * What this example does
 * =========================================================================
 * 1. Starts the 3-channel JSONL bridge (NiusRobotLab_INA_monitor compatible).
 * 2. Registers a callback to include the shunt-voltage sum (all 3 channels)
 *    as an extra field in every measurement line.
 * 3. Configures:
 *      \u2022 Critical overcurrent on CH1 (CRI pin interrupt)
 *      \u2022 Warning overcurrent on CH1  (WAR pin interrupt)
 *      \u2022 Power-valid window          (PV pin, polled in loop)
 *      \u2022 Summation alert (sum of all channels vs. a limit)
 * 4. ISR flags for CRI and WAR; loop() reads Mask/Enable and reports.
 *
 * =========================================================================
 * How to change parameters at runtime (serial commands)
 * =========================================================================
 *   START / STOP / SR <Hz> / RSHUNT <ohm> / PING
 *   CRIT <ch:1-3> <A>   \u2014 set critical overcurrent
 *   WARN <ch:1-3> <A>   \u2014 set warning overcurrent
 *   PV <Vmin> <Vmax>    \u2014 set power-valid voltage window
 *   TC                   \u2014 read Timing Control flag
 *   DIAG                 \u2014 dump Mask/Enable + PV limits
 *
 * NiusRobotLab_INA_monitor compatibility:
 *   \u2022 Measurement lines: 3-channel JSONL with extra sum_shunt_uV (ignored).
 *   \u2022 ALERT_CFG / ALERT_EVENT lines: INA_monitor ignores them.
 */
#include <INA_Series_Sensor.h>

// ---- User configuration (edit these) ------------------------------------
static const int   CRI_PIN         = 2;      // GPIO \u2192 CRI pin
static const int   WAR_PIN         = 3;      // GPIO \u2192 WRN/WAR pin
static const int   PV_PIN          = 4;      // GPIO \u2192 PV/VALID pin
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

static InaBridge3221      g_bridge(${s.bridgeArgs});
static Ina::I2cBus        g_i2c;
static Ina::Ina3221Driver g_drv(g_i2c, ${ah});
static volatile bool      g_criFlag = false;
static volatile bool      g_warFlag = false;
static bool               g_lastPvState = false;

static void INA_ISR_ATTR onCri() { g_criFlag = true; }
static void INA_ISR_ATTR onWar() { g_warFlag = true; }

// Callback: append shunt-voltage sum to every measurement line.
static void printExtraFields() {
  float sum_uV;
  if (g_drv.readShuntVoltageSum_uV(sum_uV).ok()) {
    Serial.print(F(",\\"sum_shunt_uV\\":"));
    Serial.print(sum_uV, 1);
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  g_bridge.beginI2c(8, 9);
  g_bridge.setExtraFieldsPrinter(printExtraFields);

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
  Serial.print(F("{\\"v\\":1,\\"type\\":\\"ALERT_CFG\\",\\"chip\\":\\"${s.chip}\\",\\"addr\\":\\"${ah}\\""));
  Serial.print(F(",\\"crit_A\\":"));  Serial.print(DEFAULT_CRIT_A, 2);
  Serial.print(F(",\\"warn_A\\":"));  Serial.print(DEFAULT_WARN_A, 2);
  Serial.print(F(",\\"pv_min_V\\":"));  Serial.print(DEFAULT_PV_MIN, 1);
  Serial.print(F(",\\"pv_max_V\\":"));  Serial.print(DEFAULT_PV_MAX, 1);
  Serial.print(F(",\\"cri_pin\\":"));  Serial.print(CRI_PIN);
  Serial.print(F(",\\"war_pin\\":"));  Serial.print(WAR_PIN);
  Serial.print(F(",\\"pv_pin\\":"));   Serial.print(PV_PIN);
  Serial.println(F(",\\"_note\\":\\"INA_monitor ignores this line\\"}"));
}

void loop() {
  g_bridge.tick();

  // ---- Critical alert ----
  if (g_criFlag) {
    g_criFlag = false;
    Ina::Ina3221MaskEnable me;
    g_drv.readMaskEnable(me);

    Serial.print(F("{\\"v\\":1,\\"type\\":\\"ALERT_EVENT\\",\\"chip\\":\\"${s.chip}\\",\\"addr\\":\\"${ah}\\""));
    Serial.print(F(",\\"src\\":\\"CRI\\",\\"crit\\":["));
    Serial.print(me.critFlag[0] ? 1 : 0); Serial.print(',');
    Serial.print(me.critFlag[1] ? 1 : 0); Serial.print(',');
    Serial.print(me.critFlag[2] ? 1 : 0);
    Serial.print(F("],\\"sumFlag\\":"));
    Serial.print(me.summationFlag ? 1 : 0);
    Serial.print(F(",\\"t_ms\\":"));  Serial.print(millis());
    Serial.println(F(",\\"_note\\":\\"INA_monitor ignores this line\\"}"));
  }

  // ---- Warning alert ----
  if (g_warFlag) {
    g_warFlag = false;
    Ina::Ina3221MaskEnable me;
    g_drv.readMaskEnable(me);

    Serial.print(F("{\\"v\\":1,\\"type\\":\\"ALERT_EVENT\\",\\"chip\\":\\"${s.chip}\\",\\"addr\\":\\"${ah}\\""));
    Serial.print(F(",\\"src\\":\\"WAR\\",\\"warn\\":["));
    Serial.print(me.warnFlag[0] ? 1 : 0); Serial.print(',');
    Serial.print(me.warnFlag[1] ? 1 : 0); Serial.print(',');
    Serial.print(me.warnFlag[2] ? 1 : 0);
    Serial.print(F("],\\"t_ms\\":"));  Serial.print(millis());
    Serial.println(F(",\\"_note\\":\\"INA_monitor ignores this line\\"}"));
  }

  // ---- Power-valid polling (PV is push-pull, not interrupt) ----
  bool pvNow = digitalRead(PV_PIN) == HIGH;
  if (pvNow != g_lastPvState) {
    g_lastPvState = pvNow;
    Serial.print(F("{\\"v\\":1,\\"type\\":\\"PV_EVENT\\",\\"chip\\":\\"${s.chip}\\",\\"addr\\":\\"${ah}\\""));
    Serial.print(F(",\\"power_valid\\":"));  Serial.print(pvNow ? 1 : 0);
    Serial.print(F(",\\"t_ms\\":"));         Serial.print(millis());
    Serial.println(F(",\\"_note\\":\\"INA_monitor ignores this line\\"}"));
  }
}
`;
}

// ---------------------------------------------------------------------------
// Generate all files
// ---------------------------------------------------------------------------
let count = 0;

for (const s of SPECS_228) {
  const dir = path.join(EXAMPLES, s.dir);
  const file = path.join(dir, s.dir + ".ino");
  fs.mkdirSync(dir, { recursive: true });
  fs.writeFileSync(file, ino228Advanced(s), "utf8");
  console.log("wrote", file);
  count++;
}

for (const s of SPECS_229) {
  const dir = path.join(EXAMPLES, s.dir);
  const file = path.join(dir, s.dir + ".ino");
  fs.mkdirSync(dir, { recursive: true });
  fs.writeFileSync(file, ino229Advanced(s), "utf8");
  console.log("wrote", file);
  count++;
}

for (const s of SPECS_3221) {
  const dir = path.join(EXAMPLES, s.dir);
  const file = path.join(dir, s.dir + ".ino");
  fs.mkdirSync(dir, { recursive: true });
  fs.writeFileSync(file, ino3221Advanced(s), "utf8");
  console.log("wrote", file);
  count++;
}

console.log(`\nDone \u2014 ${count} advanced examples generated.`);
