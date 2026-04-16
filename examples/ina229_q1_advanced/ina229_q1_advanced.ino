/**
 * INA229-Q1 – Advanced demo (SPI): temperature + energy/charge + alert.
 *
 * =========================================================================
 * Module compatibility
 * =========================================================================
 * No widely-available breakout boards exist for the INA229-Q1 as of 2025.
 * TI sells the INA229_239EVM evaluation module.  ALERT is chip pin 10.
 * The INA229-Q1 has an integrated die temperature sensor and 40-bit
 * energy/charge accumulation, identical to the INA228 I2C family.
 *
 * =========================================================================
 * Wiring (ESP32-C3 defaults)
 * =========================================================================
 *   INA229-Q1 CS     →  GPIO 7
 *   INA229-Q1 SCLK   →  GPIO 4
 *   INA229-Q1 MISO   →  GPIO 5
 *   INA229-Q1 MOSI   →  GPIO 6
 *   INA229-Q1 ALERT  →  GPIO 2  (open-drain, 10 kΩ pull-up)
 *   INA229-Q1 VS     →  3.3 V or 5 V
 *   INA229-Q1 GND    →  GND
 *   INA229-Q1 INP    →  load high-side     INM → load positive terminal
 *
 * =========================================================================
 * What this example does
 * =========================================================================
 * 1. Starts the SPI JSONL bridge (NiusRobotLab_INA_monitor compatible).
 * 2. Registers a callback to append die temperature to every measurement.
 * 3. Enables conversion-ready (CNVR) alert + temperature over-limit.
 * 4. Resets energy/charge accumulators; prints periodic report.
 * 5. Alert ISR → ALERT_EVENT with full status flags.
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

static InaBridge229Spi   sensor("INA229-Q1", "TI INA229-Q1");
static Ina::SpiBus       g_spi(7, 4, 5, 6, SPISettings(10000000, MSBFIRST, SPI_MODE1));
static Ina::Ina229Driver g_drv(g_spi);
static volatile bool     alertTriggered = false;
static uint32_t          g_lastEnergyMs = 0;

static void INA_ISR_ATTR onAlert() { alertTriggered = true; }

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
  sensor.beginSpi();
  sensor.setExtraFieldsPrinter(printExtraFields);

  pinMode(ALERT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ALERT_PIN), onAlert, FALLING);

  Ina::AlertConfig cfg;
  cfg.latch = true;
  g_drv.alertEnableConversionReady(cfg);
  g_drv.alertEnableTempOver_C(DEFAULT_TEMP_LIM, cfg);
  g_drv.resetAccumulators();
  g_lastEnergyMs = millis();

  Serial.print(F("{\"v\":1,\"type\":\"ALERT_CFG\",\"chip\":\"INA229-Q1\",\"addr\":\"SPI\""));
  Serial.print(F(",\"mode\":\"CNVR+TEMP\",\"temp_lim_C\":"));
  Serial.print(DEFAULT_TEMP_LIM, 1);
  Serial.print(F(",\"pin\":"));
  Serial.print(ALERT_PIN);
  Serial.println(F(",\"_note\":\"INA_monitor ignores this line\"}"));
}

void loop() {
  sensor.tick();

  if (alertTriggered) {
    alertTriggered = false;
    Ina::AlertStatus st;
    g_drv.alertReadStatus(st);

    Serial.print(F("{\"v\":1,\"type\":\"ALERT_EVENT\",\"chip\":\"INA229-Q1\",\"addr\":\"SPI\""));
    Serial.print(F(",\"convReady\":"));    Serial.print(st.conversionReady ? 1 : 0);
    Serial.print(F(",\"overvoltage\":"));  Serial.print(st.overVoltage ? 1 : 0);
    Serial.print(F(",\"overcurrent\":"));  Serial.print(st.overCurrent ? 1 : 0);
    Serial.print(F(",\"overtemp\":"));     Serial.print(st.overTemp ? 1 : 0);
    Serial.print(F(",\"t_ms\":"));         Serial.print(millis());
    Serial.println(F(",\"_note\":\"INA_monitor ignores this line\"}"));
  }

  uint32_t now = millis();
  if ((now - g_lastEnergyMs) >= ENERGY_REPORT_MS) {
    g_lastEnergyMs = now;
    float clsb = sensor.currentLsb();
    if (clsb > 0.0f) {
      float energy_J = 0, charge_C = 0;
      g_drv.readEnergy_J(energy_J, clsb);
      g_drv.readCharge_C(charge_C, clsb);

      Serial.print(F("{\"v\":1,\"type\":\"ENERGY\",\"chip\":\"INA229-Q1\",\"addr\":\"SPI\""));
      Serial.print(F(",\"energy_J\":"));  Serial.print(energy_J, 6);
      Serial.print(F(",\"charge_C\":"));  Serial.print(charge_C, 6);
      Serial.print(F(",\"t_ms\":"));      Serial.print(now);
      Serial.println(F(",\"_note\":\"INA_monitor ignores this line\"}"));
    }
  }
}
