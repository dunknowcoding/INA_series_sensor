/**
 * INA229-Q1 – ALERT interrupt demo (conversion ready, SPI).
 *
 * =========================================================================
 * Module compatibility
 * =========================================================================
 * No widely-available breakout boards exist for the INA229 as of 2025.
 * TI sells the INA229_239EVM evaluation module (requires a TI Sensor
 * Control Board).  For custom designs, ALERT is chip pin 10 (TSSOP-16).
 *
 * =========================================================================
 * Wiring (ESP32-C3 defaults)
 * =========================================================================
 *   INA229-Q1 CS     ->  GPIO 7
 *   INA229-Q1 SCLK   ->  GPIO 4
 *   INA229-Q1 MISO   ->  GPIO 5
 *   INA229-Q1 MOSI   ->  GPIO 6
 *   INA229-Q1 ALERT  ->  GPIO 2  (open-drain, add 10 k pull-up to VCC)
 *   INA229-Q1 VS     ->  3.3 V or 5 V
 *   INA229-Q1 GND    ->  GND
 *   INA229-Q1 INP    ->  load high-side     INM -> load positive terminal
 *
 * =========================================================================
 * What this example does
 * =========================================================================
 * 1. Starts the SPI JSONL bridge (NiusRobotLab_INA_monitor compatible).
 * 2. Enables the conversion-ready (CNVR) alert — each time the ADC finishes
 *    a conversion the ALERT pin pulses low, and the ISR sets a flag.
 * 3. loop() reads the alert status and prints an ALERT_EVENT placeholder.
 *
 * =========================================================================
 * Full threshold support (Pro API)
 * =========================================================================
 * The INA229-Q1 shares the same DIAG_ALRT / BOVL / BUVL / SOVL / SUVL /
 * TEMP_LIMIT / PWR_LIMIT register layout as the INA228 I2C family.
 * The current Ina229Driver Simple API only exposes CNVR and OFF, but you
 * can configure voltage / temperature / power thresholds via the Pro API:
 *
 *   uint16_t diag;
 *   g_drv.readDiagAlrt(diag);
 *   diag |= 0x0010;  // enable BUSOL (bus over-limit) bit
 *   g_drv.writeDiagAlrt(diag);
 *   // Then write the BOVL register at 0x0E via SpiBus::writeU16
 *
 * =========================================================================
 * How to change parameters at runtime (serial commands)
 * =========================================================================
 *   ALERT CNVR   — enable conversion-ready interrupt
 *   ALERT OFF     — disable alert
 *   DIAG          — read DIAG_ALRT register
 *   START / STOP / SR <Hz> / RSHUNT <ohm> / IMAX <A> / PING
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

static InaBridge229Spi sensor("INA229-Q1", "TI INA229-Q1");
static Ina::SpiBus     g_spi(7, 4, 5, 6, SPISettings(10000000, MSBFIRST, SPI_MODE1));
static Ina::Ina229Driver g_drv(g_spi);
static volatile bool   alertTriggered = false;

static void INA_ISR_ATTR onAlert() { alertTriggered = true; }

void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.beginSpi();

  pinMode(ALERT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ALERT_PIN), onAlert, FALLING);

  Ina::AlertConfig cfg;
  cfg.latch = true;
  g_drv.alertEnableConversionReady(cfg);

  Serial.print(F("{\"v\":1,\"type\":\"ALERT_CFG\",\"chip\":\"INA229-Q1\",\"addr\":\"SPI\""));
  Serial.print(F(",\"mode\":\"CNVR\",\"pin\":"));
  Serial.print(ALERT_PIN);
  Serial.println(F(",\"_note\":\"INA_monitor does not display this line\"}"));
}

void loop() {
  sensor.tick();

  if (alertTriggered) {
    alertTriggered = false;
    Ina::AlertStatus st;
    g_drv.alertReadStatus(st);

    Serial.print(F("{\"v\":1,\"type\":\"ALERT_EVENT\",\"chip\":\"INA229-Q1\",\"addr\":\"SPI\""));
    Serial.print(F(",\"conversionReady\":")); Serial.print(st.conversionReady ? 1 : 0);
    Serial.print(F(",\"t_ms\":"));             Serial.print(millis());
    Serial.println(F(",\"_note\":\"INA_monitor does not display this line\"}"));
  }
}
