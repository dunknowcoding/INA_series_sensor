# INA Series Sensor — User Guide

> **Target audience:** Beginners and intermediate users  
> **Library version:** 0.5.3+  
> **GitHub:** [dunknowcoding/INA_series_sensor](https://github.com/dunknowcoding/INA_series_sensor)

---

## Table of Contents

1. [Quick Start](#1-quick-start)
2. [Wiring Guide](#2-wiring-guide)
3. [Using the NiusRobotLab_INA_monitor Tool](#3-using-the-niusrobotlab_ina_monitor-tool)
4. [Standalone Usage (Without the Monitor Tool)](#4-standalone-usage-without-the-monitor-tool)
5. [Commercial INA Module Compatibility](#5-commercial-ina-module-compatibility)
6. [Notes and FAQ](#6-notes-and-faq)
7. [Example Index](#7-example-index)

---

## 1. Quick Start

### 1.1 Installing the Library

#### Arduino IDE

1. Download the `.zip` file from GitHub  
2. Open Arduino IDE → **Sketch → Include Library → Add .ZIP Library**  
3. Select the downloaded `.zip` file and confirm  
4. After installation, **INA Series Sensor** will appear at the bottom of the **File → Examples** menu

#### PlatformIO

Add the following to `platformio.ini`:

```ini
lib_deps = dunknowcoding/INA Series Sensor
```

Run `pio lib install` or simply build — PlatformIO will download the library automatically.

### 1.2 First Program: INA228 Basic Example

Copy and paste the following code into the Arduino IDE and upload it to your ESP32-C3 (or another development board):

```cpp
#include <INA_Series_Sensor.h>

static InaBridge228 sensor("INA228", 0x40);

void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.begin(8, 9);   // SDA=GPIO8, SCL=GPIO9 (ESP32-C3 default)
}

void loop() {
  sensor.tick();
}
```

**Notes:**
- `InaBridge228` is the bridge class for the INA228 series, handling JSONL streaming output and standalone readings
- `"INA228"` is the name used in the `chip` field of the JSONL output
- `0x40` is the I²C address (the default address for most modules)
- `begin(8, 9)` initializes the I²C bus (GPIO numbers are only effective on ESP32; AVR uses the board's default pins)
- `tick()` should be called once per `loop()` iteration — it processes serial commands and sends measurement data in streaming mode

After uploading, open the **Serial Monitor** (baud rate 115200) and you will see an INFO JSON line. Send `START` to begin sampling, and `STOP` to stop.

> **Different chip?** Simply replace the Bridge class and chip name. For example, use `InaBridge226` for INA226, or `InaBridge3221` for INA3221. See the [Example Index](#7-example-index) for the full mapping.

---

## 2. Wiring Guide

### 2.1 General High-Side Current Sensing Principle

All INA chips measure current by detecting the tiny voltage difference across a shunt resistor. The typical high-side wiring configuration:

```
                          Shunt Resistor (Rshunt)
                        ┌──────┤├──────┐
 [Power +] ──────────── VS+ (IN+)    VS- (IN-) ──────────── [Load +]
                                                              │
                                                           [Load -]
                                                              │
 [Power GND] ◄─────────────────────────────────────────────────┘
```

**Key Rules:**
- The shunt resistor must be connected in series between the power supply and the load
- VS+ (IN+) connects to the power supply side; VS- (IN-) connects to the load side
- **Do not** connect both high-side and low-side simultaneously — choose one

---

### 2.2 INA219 / INA220 (I²C)

**Electrical Specifications:** VCC 3–5.5V, max bus voltage 26V

```
 [Power +] ───→ [VS+ / IN+] ──┤Rshunt├── [VS- / IN-] ───→ [Load +] ───→ [Load -] ───→ [Power GND]
                    │                          │
                   VCC ──── 3.3V or 5V        GND
                    │                          │
                   SDA ←── 4.7kΩ pullup ──→ MCU SDA (GPIO 8)
                   SCL ←── 4.7kΩ pullup ──→ MCU SCL (GPIO 9)
                    │
                   A0 ──┐  Address select
                   A1 ──┘
```

**Address Pins (A0, A1):** 2 pins × 2 states = 4 addresses

| A1 | A0 | I²C Address |
|----|----|----------|
| GND | GND | 0x40 |
| GND | VS  | 0x41 |
| VS  | GND | 0x44 |
| VS  | VS  | 0x45 |

**ALERT Pin:** Open-drain output; requires an external pullup resistor. Most common modules (GY-219, CJMCU-219) **do not expose** the ALERT pin. If you need interrupt alert functionality, you will need to solder a wire directly to IC pin 3.

**Commercial Module Note (GY-219, etc.):** These typically include a built-in 0.1Ω shunt resistor and I²C pullup resistors — just connect VCC, GND, SDA, SCL, and IN+/IN-.

---

### 2.3 INA226 (I²C)

**Electrical Specifications:** VCC 2.7–5.5V, max bus voltage 36V

```
 [Power +] ───→ [IN+] ──┤Rshunt├── [IN-] ───→ [Load +] ───→ [Load -] ───→ [Power GND]
                  │                   │
                 VCC                 GND
                  │                   │
                 SDA ←── 4.7kΩ ──→ MCU SDA
                 SCL ←── 4.7kΩ ──→ MCU SCL
                  │
                 ALE (ALERT) ──── 10kΩ pullup ──→ MCU GPIO (interrupt)
                  │
                 A0 ──┐  Address select
                 A1 ──┘
```

**Address Pins (A0, A1):** Each pin has 4 states (GND / VS / SDA / SCL), yielding **16 addresses** (0x40–0x4F).

| A1 | A0 | Address | A1 | A0 | Address |
|----|----|------|----|----|------|
| GND | GND | 0x40 | SDA | GND | 0x48 |
| GND | VS  | 0x41 | SDA | VS  | 0x49 |
| GND | SDA | 0x42 | SDA | SDA | 0x4A |
| GND | SCL | 0x43 | SDA | SCL | 0x4B |
| VS  | GND | 0x44 | SCL | GND | 0x4C |
| VS  | VS  | 0x45 | SCL | VS  | 0x4D |
| VS  | SDA | 0x46 | SCL | SDA | 0x4E |
| VS  | SCL | 0x47 | SCL | SCL | 0x4F |

**ALERT Pin:** Open-drain output; requires an external 10kΩ pullup resistor. The CJMCU-226 module exposes this pin on its header (labeled ALE).

**Commercial Module Note (GY-226, etc.):** The built-in shunt resistor is typically 0.1Ω or 0.01Ω (check the silkscreen: R100 = 0.1Ω, R010 = 0.01Ω).

---

### 2.4 INA228 / INA237 / INA238 / INA239 (I²C Digital Series)

**Electrical Specifications:** VCC 2.7–5.5V, max bus voltage 85V (INA228), configurable ADC range

```
 [Power +] ───→ [VS+] ──┤Rshunt├── [VS-] ───→ [Load +] ───→ [Load -] ───→ [Power GND]
                  │                  │
                 VCC                GND
                  │                  │
                 SDA ←── 4.7kΩ ──→ MCU SDA (GPIO 8)
                 SCL ←── 4.7kΩ ──→ MCU SCL (GPIO 9)
                  │
                ALRT ──── 10kΩ pullup ──→ MCU GPIO 2 (interrupt)
                  │
                 A0 ──┐  Address select (same as INA226, 16 addresses)
                 A1 ──┘
```

**Key Features:**
- **Configurable ADC Range:** ±163.84mV (default, high range) or ±40.96mV (high precision mode)
- **Built-in Die Temperature Sensor:** ±1°C accuracy, readable via `readDieTemp()`
- **Energy / Charge Accumulation:** 40-bit registers, readable via `readEnergy()` / `readCharge()`
- **Multiple Simultaneous Alert Thresholds:** Bus Over-Voltage (BOVL), Bus Under-Voltage (BUVL), Shunt Over-Voltage (SOVL), Temperature Limit (TEMP_LIMIT), Power Limit (PWR_LIMIT) — can all be enabled simultaneously

---

### 2.5 INA229 (SPI)

**Electrical Specifications:** Same measurement capabilities as INA228, but uses an SPI interface

```
 [Power +] ───→ [INP] ──┤Rshunt├── [INM] ───→ [Load +] ───→ [Load -] ───→ [Power GND]
                  │                  │
                  VS                GND
                  │                  │
                  CS  ←─────────── MCU GPIO 7
                 SCK  ←─────────── MCU GPIO 4
                MISO (SDO) ──────→ MCU GPIO 5
                MOSI (SDI) ←────── MCU GPIO 6
                  │
                ALERT ── 10kΩ pullup ──→ MCU GPIO 2 (interrupt)
```

**SPI Parameters:**
- **SPI Mode 1** (CPOL=0, CPHA=1)
- Clock rate: up to 10 MHz
- ESP32-C3 default pins: CS=GPIO7, SCK=GPIO4, MISO=GPIO5, MOSI=GPIO6

> INA229 has the same die temperature sensor and energy/charge accumulation features as INA228.

---

### 2.6 INA3221 (I²C, 3-Channel)

**Electrical Specifications:** VCC 2.7–5.5V, max bus voltage 26V, 3 independent channels

```
 [Power1 +] ───→ [IN1+] ──┤R1├── [IN1-] ───→ [Load1]     Channel 1
 [Power2 +] ───→ [IN2+] ──┤R2├── [IN2-] ───→ [Load2]     Channel 2
 [Power3 +] ───→ [IN3+] ──┤R3├── [IN3-] ───→ [Load3]     Channel 3
                  │                │
                 VCC              GND
                  │                │
                 SDA ←─ 4.7kΩ ──→ MCU SDA
                 SCL ←─ 4.7kΩ ──→ MCU SCL
                  │
                 CRI ──── 10kΩ pullup ──→ MCU GPIO 2  (critical alert, open-drain)
                 WRN ──── 10kΩ pullup ──→ MCU GPIO 3  (warning alert, open-drain)
                  PV ──── VPU to 3.3V ──→ MCU GPIO 4  (power valid, push-pull)
                  TC ──── (optional) ────────→ MCU GPIO 5  (timing control)
                  │
                 A0 ── Address select
```

**Address Pin (A0):** Only 1 pin

| A0 | I²C Address |
|----|----------|
| GND | 0x40 |
| VS  | 0x41 |

**Special Pin Descriptions:**
- **CRI (Critical):** Pulled low when any channel's current exceeds the critical limit; open-drain output, requires external pullup
- **WRN (Warning):** Pulled low when the averaged measurement exceeds the warning limit; open-drain output, requires external pullup
- **PV (Power Valid):** Outputs high when the bus voltage of all enabled channels is within the configured window (push-pull output; VPU must be tied to logic level)
- **TC (Timing Control):** Pulled low on abnormal power-up sequencing; open-drain output

---

### 2.7 Commercial Module Quick Wiring

Most commercial modules have built-in shunt resistors and I²C pullup resistors, making wiring very simple:

```
 [Module VCC] ──── 3.3V or 5V
 [Module GND] ──── GND
 [Module SDA] ──── MCU SDA
 [Module SCL] ──── MCU SCL
 [Module IN+ / V+] ──── Power supply positive
 [Module IN- / V-] ──── Load positive

 Power (+) ──→ Module IN+ ──┤Built-in Rshunt├── Module IN- ──→ Load (+)
                                                         │
 Power GND ◄──────────────────────────────── Load (-) ◄───┘
```

> **Important:** Do not connect IN+ to both the high side and the low side — choose only one configuration. High-side sensing (IN+ on the power supply side) is the most common approach.

---

## 3. Using the NiusRobotLab_INA_monitor Tool

[NiusRobotLab_INA_monitor](https://github.com/dunknowcoding/NiusRobotLab_INA_monitor) is a companion desktop real-time monitoring tool that graphically displays voltage, current, and power curves.

### 3.1 Usage Steps

**Step 1: Upload an Example Sketch**

Upload any `*_basic` example to your development board. For example, `ina228_basic`:

```cpp
#include <INA_Series_Sensor.h>

static InaBridge228 sensor("INA228", 0x40);

void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.begin(8, 9);
}

void loop() {
  sensor.tick();
}
```

**Step 2: Download INA_monitor**

Download from GitHub: https://github.com/dunknowcoding/NiusRobotLab_INA_monitor

**Step 3: Connect**

1. Open the INA_monitor desktop application
2. Select the serial port (COM port) corresponding to your development board
3. Ensure the baud rate is set to **115200**

**Step 4: Start Monitoring**

Click the **Start** button to see real-time graphs:
- Bus voltage (bus_V)
- Current (current_A)
- Power (power_W)

**Step 5: Adjust Parameters**

INA_monitor can send serial commands to control sampling:
- `SR <Hz>` — Set the sample rate (1–400 Hz)
- `STOP` — Pause sampling
- `RSHUNT <ohm>` — Set the shunt resistance value
- `IMAX <A>` — Set the maximum expected current

### 3.2 JSONL Protocol Description

The development board outputs each measurement sample in **JSON Lines** format, one JSON object per line:

```json
{"v":1,"chip":"INA228","bus_V":3.3,"current_A":0.15,"power_W":0.495}
```

**Fields recognized by INA_monitor:**

| Field | Type | Description |
|------|------|------|
| `v` | int | Protocol version (fixed at 1) |
| `chip` | string | Chip name |
| `bus_V` | float | Bus voltage (V) |
| `current_A` | float | Current (A) |
| `power_W` | float | Power (W) |

**Fields ignored by INA_monitor:**
- `type` (e.g. `"ALERT_CFG"`, `"ALERT_EVENT"`, `"ENERGY"`, etc.)
- `temp_C` (die temperature)
- `shunt_uV` (shunt voltage)
- `_note` (annotation)
- Any custom fields appended via `setExtraFieldsPrinter()`

This means you can add extra fields to the JSONL output without affecting INA_monitor's display.

### 3.3 Serial Command List

Send the following commands via INA_monitor or the Serial Monitor:

| Command | Description |
|------|------|
| `START` | Begin JSONL streaming output |
| `STOP` | Stop streaming output |
| `SR <Hz>` | Set sample rate (I²C: 1–400 Hz, SPI: 1–2000 Hz) |
| `RSHUNT <ohm>` | Set shunt resistance value (e.g. `RSHUNT 0.1`) |
| `IMAX <A>` | Set maximum expected current (e.g. `IMAX 10`) |
| `PING` | Connection test |
| `ALERT BOV <V>` | Bus over-voltage threshold (INA226/228 series) |
| `ALERT BUV <V>` | Bus under-voltage threshold |
| `ALERT SOV <uV>` | Shunt over-voltage threshold |
| `ALERT CNVR` | Conversion-ready interrupt (INA228/229) |
| `ALERT OFF` | Disable alerts |
| `DIAG` | Read diagnostic register |

---

## 4. Standalone Usage (Without the Monitor Tool)

This library supports two usage modes that can be used independently or simultaneously:

- **Mode A: JSONL Streaming** — Call `tick()`; the host controls sampling via `START` / `STOP`
- **Mode B: Direct Reading** — Call `readBusVoltage()` and other APIs directly; no Serial output is generated

### 4.1 Mode A Only: JSONL Streaming

This is the default mode for all `*_basic` examples. Simply call `tick()` in `loop()`.

```cpp
#include <INA_Series_Sensor.h>

static InaBridge228 sensor("INA228", 0x40);

void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.begin(8, 9);
}

void loop() {
  sensor.tick();
}
```

You can also programmatically control streaming output:

```cpp
void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.begin(8, 9);
  sensor.setSampleRate(50);      // 50 Hz sampling
  sensor.startStreaming();        // No need to wait for the START command
}
```

### 4.2 Mode B Only: Direct Reading

Do not call `tick()` — use the measurement APIs directly:

```cpp
#include <INA_Series_Sensor.h>

static InaBridge228 sensor("INA228", 0x40);

void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.begin(8, 9);
  sensor.setRshunt(0.1);    // 100 mΩ shunt resistor
  sensor.setImax(10.0);     // 10 A max current
}

void loop() {
  if (sensor.dataReady()) {
    float voltage = sensor.readBusVoltage();   // V
    float current = sensor.readCurrent();      // A
    float power   = sensor.readPower();        // W

    Serial.print("V="); Serial.print(voltage, 3);
    Serial.print(" I="); Serial.print(current, 4);
    Serial.print(" P="); Serial.println(power, 4);
  }
  delay(100);
}
```

### 4.3 Using Both Modes Simultaneously

`tick()` handles JSONL streaming, while your code can also call `readBusVoltage()` and other APIs:

```cpp
#include <INA_Series_Sensor.h>

static InaBridge228 sensor("INA228", 0x40);

void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.begin(8, 9);
  sensor.setRshunt(0.1);
  sensor.setImax(10.0);
}

void loop() {
  sensor.tick();    // Mode A: INA_monitor can send START/STOP

  // Mode B: Your own logic
  if (sensor.dataReady()) {
    float v = sensor.readBusVoltage();
    float i = sensor.readCurrent();
    if (i > 5.0) {
      digitalWrite(LED_BUILTIN, HIGH);   // Over-current indicator
    }
  }
}
```

> **Note:** When using both modes simultaneously, `dataReady()` may return `false` between streaming samples, because the streaming code also reads and clears the conversion-ready flag.

### 4.4 Code Example: INA228 Temperature and Energy Monitoring

```cpp
#include <INA_Series_Sensor.h>

static InaBridge228      sensor("INA228", 0x40);
static Ina::I2cBus       g_i2c;
static Ina::Ina228Driver g_drv(g_i2c, 0x40);

uint32_t lastReport = 0;

void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.begin(8, 9);
  g_drv.resetAccumulators();           // Reset energy/charge accumulators
}

void loop() {
  sensor.tick();

  if (millis() - lastReport >= 5000) {
    lastReport = millis();

    float temp;
    g_drv.readDieTemp_C(temp);
    Serial.print("Die temp: "); Serial.print(temp, 1); Serial.println(" °C");

    float clsb = sensor.currentLsb();
    if (clsb > 0) {
      float energy_J = 0, charge_C = 0;
      g_drv.readEnergy_J(energy_J, clsb);
      g_drv.readCharge_C(charge_C, clsb);
      Serial.print("Energy: "); Serial.print(energy_J, 4); Serial.println(" J");
      Serial.print("Charge: "); Serial.print(charge_C, 4); Serial.println(" C");
    }
  }
}
```

### 4.5 Code Example: INA3221 Multi-Channel Independent Shunt Resistors

```cpp
#include <INA_Series_Sensor.h>

static InaBridge3221 sensor("INA3221", 0x40);

void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.begin(8, 9);

  sensor.setRshunt(1, 0.100);   // CH1: 100 mΩ
  sensor.setRshunt(2, 0.050);   // CH2:  50 mΩ
  sensor.setRshunt(3, 0.010);   // CH3:  10 mΩ
}

void loop() {
  if (sensor.dataReady()) {
    for (uint8_t ch = 1; ch <= 3; ch++) {
      Serial.print("CH"); Serial.print(ch); Serial.print(": ");
      Serial.print(sensor.readBusVoltage(ch), 3); Serial.print(" V  ");
      Serial.print(sensor.readCurrent(ch), 4);    Serial.println(" A");
    }
    Serial.println();
  }
  delay(500);
}
```

### 4.6 Code Example: Alert Interrupt (INA228 Bus Over-Voltage)

```cpp
#include <INA_Series_Sensor.h>

static const int   ALERT_PIN     = 2;
static const float THRESHOLD_V   = 5.5;

#if defined(ESP32) || defined(ESP8266)
  #define INA_ISR_ATTR IRAM_ATTR
#else
  #define INA_ISR_ATTR
#endif

static InaBridge228      sensor("INA228", 0x40);
static Ina::I2cBus       g_i2c;
static Ina::Ina228Driver g_drv(g_i2c, 0x40);
static volatile bool     alertTriggered = false;

static void INA_ISR_ATTR onAlert() { alertTriggered = true; }

void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.begin(8, 9);

  pinMode(ALERT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ALERT_PIN), onAlert, FALLING);

  Ina::AlertConfig cfg;
  cfg.latch = true;
  g_drv.alertEnableBusOverVoltage_V(THRESHOLD_V, cfg);
}

void loop() {
  sensor.tick();

  if (alertTriggered) {
    alertTriggered = false;
    Ina::AlertStatus st;
    g_drv.alertReadStatus(st);

    if (st.overVoltage) {
      Serial.println("Over-voltage alert triggered!");
    }
  }
}
```

**Interrupt Alert Mode Notes:**
- The Bridge object (`InaBridge228`) handles JSONL output and `tick()`
- The Driver object (`Ina::Ina228Driver`) handles alert configuration and status reading
- Both share the same I²C address and can coexist
- The ALERT pin is an open-drain output and requires an external pullup resistor (10kΩ)

### 4.7 Code Example: INA3221 Critical Over-Current Interrupt

```cpp
#include <INA_Series_Sensor.h>

static const int   CRI_PIN    = 2;
static const float CRIT_A     = 1.0;
static const float RSHUNT_OHM = 0.1;

#if defined(ESP32) || defined(ESP8266)
  #define INA_ISR_ATTR IRAM_ATTR
#else
  #define INA_ISR_ATTR
#endif

static InaBridge3221      sensor("INA3221", 0x40);
static Ina::I2cBus        g_i2c;
static Ina::Ina3221Driver g_drv(g_i2c, 0x40);
static volatile bool      g_criFlag = false;

static void INA_ISR_ATTR onCri() { g_criFlag = true; }

void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.begin(8, 9);

  pinMode(CRI_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(CRI_PIN), onCri, FALLING);

  g_drv.enableCriticalOverCurrent_A(1, CRIT_A, RSHUNT_OHM);
}

void loop() {
  sensor.tick();

  if (g_criFlag) {
    g_criFlag = false;
    Ina::Ina3221MaskEnable me;
    g_drv.readMaskEnable(me);

    for (int ch = 0; ch < 3; ch++) {
      if (me.critFlag[ch]) {
        Serial.print("CH"); Serial.print(ch + 1);
        Serial.println(" critical over-current!");
      }
    }
  }
}
```

### 4.8 Code Example: One-Shot Measurement

Suitable for low-power scenarios — configure, then read a single value:

```cpp
#include <INA_Series_Sensor.h>

static InaBridge228 sensor("INA228", 0x40);

void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.begin(8, 9);
  sensor.setRshunt(0.1);
  sensor.setImax(10.0);
}

void loop() {
  if (sensor.dataReady()) {
    float v = sensor.readBusVoltage();
    float i = sensor.readCurrent();
    float p = sensor.readPower();

    Serial.print("V="); Serial.print(v, 3);
    Serial.print(" I="); Serial.print(i, 4);
    Serial.print(" P="); Serial.println(p, 4);
  }

  delay(1000);   // Read once per second
}
```

---

## 5. Commercial INA Module Compatibility

### 5.1 Module Comparison Table

| Module Name | Chip | Built-in Rshunt | Default I²C Address | Built-in Pullup | Power Supply |
|----------|------|-------------|---------------|---------|------|
| GY-INA219 / CJMCU-219 | INA219 | 0.1 Ω | 0x40 | ✅ | 3–5V |
| Adafruit INA219 (#904) | INA219 | 0.1 Ω | 0x40 | ✅ | 3–5V |
| GY-INA226 / CJMCU-226 | INA226 | 0.1 Ω or 0.01 Ω | 0x40 | ✅ | 3–5V |
| INA3221 Breakout (CJMCU-3221) | INA3221 | 3× 0.1 Ω | 0x40 | ✅ | 3–5V |
| Adafruit INA3221 | INA3221 | 3× 0.1 Ω | 0x40 | ✅ | 3–5V |
| Adafruit INA228 (#5832) | INA228 | 0.015 Ω | 0x40 | ✅ | 3–5V |

### 5.2 Configuration Recommendations per Module

#### GY-INA219 / CJMCU-219 / Adafruit INA219

```cpp
static InaBridge219 sensor("INA219", 0x40);

void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.begin(8, 9);
  sensor.setRshunt(0.1);    // Module's built-in 0.1Ω shunt resistor
  sensor.setImax(3.2);      // Max 3.2A (0.1Ω × 3.2A = 320mV, within range)
}
```

- **setRshunt:** `0.1` (standard R100 chip resistor on the module)
- **setImax:** Recommended `3.2` (320mV shunt voltage is within range with optimal accuracy)
- **I²C Address:** Default 0x40 (A0=GND, A1=GND)
- **Built-in Pullup:** Yes — no additional pullups needed

#### GY-INA226 / CJMCU-226

```cpp
static InaBridge226 sensor("INA226", 0x40);

void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.begin(8, 9);
  sensor.setRshunt(0.1);    // Check your module! May be 0.1Ω or 0.01Ω
  sensor.setImax(8.0);      // Set according to actual requirements
}
```

- **setRshunt:** `0.1` or `0.01` (check the shunt resistor silkscreen on your module: R100 = 0.1Ω, R010 = 0.01Ω)
- **setImax:** Set according to your application. A 0.01Ω shunt can measure higher currents
- **I²C Address:** Default 0x40
- **Built-in Pullup:** Yes

#### INA3221 Breakout (CJMCU-3221 / Adafruit)

```cpp
static InaBridge3221 sensor("INA3221", 0x40);

void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.begin(8, 9);
  sensor.setRshunt(0.1);    // Set all three channels at once
  // Or set individually:
  // sensor.setRshunt(1, 0.100);
  // sensor.setRshunt(2, 0.100);
  // sensor.setRshunt(3, 0.100);
}
```

- **setRshunt:** `0.1` (each of the three channels has a 0.1Ω shunt resistor)
- **I²C Address:** 0x40 (A0=GND); can be changed to 0x41 (A0=VS)
- **Built-in Pullup:** Yes

#### Adafruit INA228 (#5832, STEMMA QT)

```cpp
static InaBridge228 sensor("INA228", 0x40);

void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.begin(8, 9);
  sensor.setRshunt(0.015);   // Module uses a 15mΩ shunt resistor
  sensor.setImax(10.0);      // Set according to actual requirements
}
```

- **setRshunt:** `0.015` (15mΩ — please verify with the Adafruit module documentation)
- **setImax:** Set according to your load
- **I²C Address:** Default 0x40 (solder pads on the back of the module allow address changes)
- **Built-in Pullup:** Yes (STEMMA QT connector)
- **ALERT Pin:** Exposed on the header

---

## 6. Notes and FAQ

### 6.1 I²C pin remapping (ESP32 & Nordic)

The pin parameters in `begin(SDA_pin, SCL_pin)` are **effective on ESP32 and Nordic nRF52/nRF53 Arduino cores** that expose `Wire.begin(sda, scl)`.

- **ESP32 / ESP32-S3 / ESP32-C3:** You can freely specify the GPIO numbers for SDA and SCL
- **ArduinoNRF nRF52** (ProMicro, SuperMini, XIAO, …): Use board silk `D*` pin numbers (e.g. ProMicro default I²C is D6/D7)
- **Adafruit nRF52 / Mbed Nano 33 BLE / nRF5340:** Pin remap when the core supports it; otherwise the board default `Wire` pins are used
- **Arduino AVR (Uno / Mega / Nano):** Pin parameters are ignored; the board's default I²C pins are used (Uno: SDA=A4, SCL=A5)
- **RP2040 / SAMD / STM32:** Uses the board's default pins unless the core exposes pin-mapped `Wire.begin()`

### 6.2 Serial Baud Rate

**You must use 115200 baud.** All examples and the INA_monitor tool use 115200 as the standard.

```cpp
Serial.begin(115200);   // Required
```

### 6.3 No Serial Output

If you see no output after opening the Serial Monitor:

1. **Check the USB cable:** Make sure it is a data cable, not a charge-only cable (charge-only cables only have VCC and GND, no data lines)
2. **Check the baud rate:** The Serial Monitor must be set to **115200**
3. **Check the COM port:** Make sure you have selected the correct port
4. **ESP32 USB CDC:** Some ESP32 boards require enabling **USB CDC On Boot** in the Arduino IDE
5. **Upload the `diag_serial_only` example:** This example requires no INA hardware and only tests whether USB serial communication is working
6. **Close INA_monitor:** If INA_monitor is already occupying the serial port, the Arduino Serial Monitor cannot connect. Only one program can open the serial port at a time

### 6.4 ESP32 Interrupt Service Routines (ISR)

On ESP32 / ESP8266, interrupt service functions **must** use `IRAM_ATTR` (to place the function in IRAM):

```cpp
#if defined(ESP32) || defined(ESP8266)
  #define INA_ISR_ATTR IRAM_ATTR
#else
  #define INA_ISR_ATTR
#endif

static void INA_ISR_ATTR onAlert() { alertTriggered = true; }
```

All `*_alert_interrupt` examples already include this compatibility code.

### 6.5 Multiple INAs on the Same I²C Bus

You can connect multiple INA chips to the same I²C bus, but each chip **must use a different I²C address**:

```cpp
static InaBridge228 sensor1("INA228-A", 0x40);
static InaBridge228 sensor2("INA228-B", 0x41);

void setup() {
  Serial.begin(115200);
  delay(500);
  sensor1.begin(8, 9);
  sensor2.begin(8, 9);   // Shares the same SDA/SCL
}

void loop() {
  sensor1.tick();
  sensor2.tick();
}
```

Set different addresses using the A0 / A1 pins.

### 6.6 `dataReady()` Always Returns False

**Possible Causes:**
- If you are simultaneously using streaming mode (`tick()` + `START`), the streaming code reads the DIAG_ALRT register and clears the conversion-ready flag. `dataReady()` will return `false` until the next conversion completes
- **Solution:** If you only need direct readings, do not call `tick()` or do not send the `START` command

### 6.7 Current Reads Zero

**Troubleshooting Steps:**
1. **Check calibration:** Confirm that `setRshunt()` and `setImax()` have been called with correct values
2. **Check wiring:** The shunt resistor must be connected **in series** in the current path, not in parallel
3. **Confirm the load draws current:** If the load is unpowered or draws negligible current, a reading near 0 is expected
4. **Check the shunt resistance value:** The parameter passed to `setRshunt()` must match the actual shunt resistor on your hardware

### 6.8 Large Flash Usage on AVR Platforms

This library includes code for all Bridge classes at compile time. On AVR platforms (e.g. Arduino Uno, 32KB Flash):

- **Flash usage of approximately 50–62%** is normal
- If your project code is large, it may exceed the Flash capacity
- **Recommendation:** For projects with tight Flash constraints, consider using platforms with larger capacity such as ESP32 or RP2040

### 6.9 Limitations of InaBridgeCh1

`InaBridgeCh1` (used for INA2227 / INA4230 / INA4235) has two special limitations:

- **No `dataReady()` method:** These chips lack a reliable conversion-ready flag
- **No `setImax()` method:** No calibration register; current = shunt voltage / Rshunt (calculated automatically)

---

## 7. Example Index

This library includes **69** ready-to-upload examples.

> **Naming Conventions:**
> - `*_basic` — Basic JSONL bridge, minimal code
> - `*_advanced` — Advanced features (temperature/energy/custom fields + multi-threshold alerts)
> - `*_alert_interrupt` — Hardware interrupt alerts
> - `*_q1_*` — Automotive-grade (Q1) variants, same code structure

### Diagnostics and Placeholder

| # | Example Name | Chip | Type | Bridge Class | Description |
|---|---------|------|------|-----------|---------|
| 1 | `diag_serial_only` | — | Diagnostic | — | USB serial test (no I²C); troubleshoot communication issues |
| 2 | `unknown_basic` | — | Placeholder | `InaBridgeUnknown` | No-sensor placeholder; outputs all zeros; validates JSONL protocol |

### INA219 (2)

| # | Example Name | Chip | Type | Bridge Class | Description |
|---|---------|------|------|-----------|---------|
| 3 | `ina219_basic` | INA219 | basic | `InaBridge219` | Basic JSONL bridge |
| 4 | `ina219_alert_interrupt` | INA219 | alert_interrupt | `InaBridge219` | Bus over-voltage interrupt + ALERT_EVENT |

### INA220 (4)

| # | Example Name | Chip | Type | Bridge Class | Description |
|---|---------|------|------|-----------|---------|
| 5 | `ina220_basic` | INA220 | basic | `InaBridge219` | Basic JSONL bridge |
| 6 | `ina220_alert_interrupt` | INA220 | alert_interrupt | `InaBridge219` | Bus over-voltage interrupt |
| 7 | `ina220_q1_basic` | INA220-Q1 | basic | `InaBridge219` | Q1 automotive-grade variant |
| 8 | `ina220_q1_alert_interrupt` | INA220-Q1 | alert_interrupt | `InaBridge219` | Q1 bus over-voltage interrupt |

### INA226 (4)

| # | Example Name | Chip | Type | Bridge Class | Description |
|---|---------|------|------|-----------|---------|
| 9 | `ina226_basic` | INA226 | basic | `InaBridge226` | Basic JSONL bridge |
| 10 | `ina226_alert_interrupt` | INA226 | alert_interrupt | `InaBridge226` | Bus over-voltage interrupt |
| 11 | `ina226_q1_basic` | INA226-Q1 | basic | `InaBridge226` | Q1 automotive-grade variant |
| 12 | `ina226_q1_alert_interrupt` | INA226-Q1 | alert_interrupt | `InaBridge226` | Q1 bus over-voltage interrupt |

### INA228 (6)

| # | Example Name | Chip | Type | Bridge Class | Description |
|---|---------|------|------|-----------|---------|
| 13 | `ina228_basic` | INA228 | basic | `InaBridge228` | Basic JSONL bridge |
| 14 | `ina228_advanced` | INA228 | advanced | `InaBridge228` | Temperature + energy/charge + multi-threshold alerts |
| 15 | `ina228_alert_interrupt` | INA228 | alert_interrupt | `InaBridge228` | Bus over-voltage interrupt |
| 16 | `ina228_q1_basic` | INA228-Q1 | basic | `InaBridge228` | Q1 automotive-grade variant |
| 17 | `ina228_q1_advanced` | INA228-Q1 | advanced | `InaBridge228` | Q1 temperature + energy/charge + alerts |
| 18 | `ina228_q1_alert_interrupt` | INA228-Q1 | alert_interrupt | `InaBridge228` | Q1 bus over-voltage interrupt |

### INA229 — SPI (6)

| # | Example Name | Chip | Type | Bridge Class | Description |
|---|---------|------|------|-----------|---------|
| 19 | `ina229_basic` | INA229 | basic | `InaBridge229Spi` | SPI JSONL bridge |
| 20 | `ina229_advanced` | INA229 | advanced | `InaBridge229Spi` | SPI temperature + energy/charge + CNVR alert |
| 21 | `ina229_alert_interrupt` | INA229 | alert_interrupt | `InaBridge229Spi` | SPI conversion-ready (CNVR) interrupt |
| 22 | `ina229_q1_basic` | INA229-Q1 | basic | `InaBridge229Spi` | Q1 SPI bridge |
| 23 | `ina229_q1_advanced` | INA229-Q1 | advanced | `InaBridge229Spi` | Q1 SPI temperature + energy + alerts |
| 24 | `ina229_q1_alert_interrupt` | INA229-Q1 | alert_interrupt | `InaBridge229Spi` | Q1 SPI CNVR interrupt |

### INA230–234, INA236 (12)

| # | Example Name | Chip | Type | Bridge Class | Description |
|---|---------|------|------|-----------|---------|
| 25 | `ina230_basic` | INA230 | basic | `InaBridge226` | Basic JSONL bridge |
| 26 | `ina230_alert_interrupt` | INA230 | alert_interrupt | `InaBridge226` | Bus over-voltage interrupt |
| 27 | `ina231_basic` | INA231 | basic | `InaBridge226` | Basic JSONL bridge |
| 28 | `ina231_alert_interrupt` | INA231 | alert_interrupt | `InaBridge226` | Bus over-voltage interrupt |
| 29 | `ina232_basic` | INA232 | basic | `InaBridge226` | Basic JSONL bridge |
| 30 | `ina232_alert_interrupt` | INA232 | alert_interrupt | `InaBridge226` | Bus over-voltage interrupt |
| 31 | `ina233_basic` | INA233 | basic | `InaBridge226` | Basic JSONL bridge |
| 32 | `ina233_alert_interrupt` | INA233 | alert_interrupt | `InaBridge226` | Bus over-voltage interrupt |
| 33 | `ina234_basic` | INA234 | basic | `InaBridge226` | Basic JSONL bridge |
| 34 | `ina234_alert_interrupt` | INA234 | alert_interrupt | `InaBridge226` | Bus over-voltage interrupt |
| 35 | `ina236_basic` | INA236 | basic | `InaBridge226` | Basic JSONL bridge |
| 36 | `ina236_alert_interrupt` | INA236 | alert_interrupt | `InaBridge226` | Bus over-voltage interrupt |

### INA237 (6)

| # | Example Name | Chip | Type | Bridge Class | Description |
|---|---------|------|------|-----------|---------|
| 37 | `ina237_basic` | INA237 | basic | `InaBridge228` | Basic JSONL bridge |
| 38 | `ina237_advanced` | INA237 | advanced | `InaBridge228` | Temperature + energy/charge + multi-threshold alerts |
| 39 | `ina237_alert_interrupt` | INA237 | alert_interrupt | `InaBridge228` | Bus over-voltage interrupt |
| 40 | `ina237_q1_basic` | INA237-Q1 | basic | `InaBridge228` | Q1 automotive-grade variant |
| 41 | `ina237_q1_advanced` | INA237-Q1 | advanced | `InaBridge228` | Q1 temperature + energy + alerts |
| 42 | `ina237_q1_alert_interrupt` | INA237-Q1 | alert_interrupt | `InaBridge228` | Q1 bus over-voltage interrupt |

### INA238 (6)

| # | Example Name | Chip | Type | Bridge Class | Description |
|---|---------|------|------|-----------|---------|
| 43 | `ina238_basic` | INA238 | basic | `InaBridge228` | Basic JSONL bridge |
| 44 | `ina238_advanced` | INA238 | advanced | `InaBridge228` | Temperature + energy/charge + multi-threshold alerts |
| 45 | `ina238_alert_interrupt` | INA238 | alert_interrupt | `InaBridge228` | Bus over-voltage interrupt |
| 46 | `ina238_q1_basic` | INA238-Q1 | basic | `InaBridge228` | Q1 automotive-grade variant |
| 47 | `ina238_q1_advanced` | INA238-Q1 | advanced | `InaBridge228` | Q1 temperature + energy + alerts |
| 48 | `ina238_q1_alert_interrupt` | INA238-Q1 | alert_interrupt | `InaBridge228` | Q1 bus over-voltage interrupt |

### INA239 (6)

| # | Example Name | Chip | Type | Bridge Class | Description |
|---|---------|------|------|-----------|---------|
| 49 | `ina239_basic` | INA239 | basic | `InaBridge228` | Basic JSONL bridge |
| 50 | `ina239_advanced` | INA239 | advanced | `InaBridge228` | Temperature + energy/charge + multi-threshold alerts |
| 51 | `ina239_alert_interrupt` | INA239 | alert_interrupt | `InaBridge228` | Bus over-voltage interrupt |
| 52 | `ina239_q1_basic` | INA239-Q1 | basic | `InaBridge228` | Q1 automotive-grade variant |
| 53 | `ina239_q1_advanced` | INA239-Q1 | advanced | `InaBridge228` | Q1 temperature + energy + alerts |
| 54 | `ina239_q1_alert_interrupt` | INA239-Q1 | alert_interrupt | `InaBridge228` | Q1 bus over-voltage interrupt |

### INA3221 (6)

| # | Example Name | Chip | Type | Bridge Class | Description |
|---|---------|------|------|-----------|---------|
| 55 | `ina3221_basic` | INA3221 | basic | `InaBridge3221` | 3-channel JSONL bridge |
| 56 | `ina3221_advanced` | INA3221 | advanced | `InaBridge3221` | CRI + WAR + PV + shunt voltage summation |
| 57 | `ina3221_alert_interrupt` | INA3221 | alert_interrupt | `InaBridge3221` | CRI critical over-current interrupt |
| 58 | `ina3221_q1_basic` | INA3221-Q1 | basic | `InaBridge3221` | Q1 automotive-grade variant |
| 59 | `ina3221_q1_advanced` | INA3221-Q1 | advanced | `InaBridge3221` | Q1 CRI + WAR + PV + summation |
| 60 | `ina3221_q1_alert_interrupt` | INA3221-Q1 | alert_interrupt | `InaBridge3221` | Q1 CRI interrupt |

### INA2227 (2)

| # | Example Name | Chip | Type | Bridge Class | Description |
|---|---------|------|------|-----------|---------|
| 61 | `ina2227_basic` | INA2227 | basic | `InaBridgeCh1` | CH1 JSONL bridge |
| 62 | `ina2227_alert_interrupt` | INA2227 | alert_interrupt | `InaBridgeCh1` | CH1 alert interrupt |

### INA4230 / INA4235 (4)

| # | Example Name | Chip | Type | Bridge Class | Description |
|---|---------|------|------|-----------|---------|
| 63 | `ina4230_basic` | INA4230 | basic | `InaBridgeCh1` | CH1 JSONL bridge |
| 64 | `ina4230_alert_interrupt` | INA4230 | alert_interrupt | `InaBridgeCh1` | CH1 alert interrupt |
| 65 | `ina4235_basic` | INA4235 | basic | `InaBridgeCh1` | CH1 JSONL bridge |
| 66 | `ina4235_alert_interrupt` | INA4235 | alert_interrupt | `InaBridgeCh1` | CH1 alert interrupt |

### INA740X (3)

| # | Example Name | Chip | Type | Bridge Class | Description |
|---|---------|------|------|-----------|---------|
| 67 | `ina740x_basic` | INA740X | basic | `InaBridge228` | Basic JSONL bridge |
| 68 | `ina740x_advanced` | INA740X | advanced | `InaBridge228` | Temperature + energy/charge + multi-threshold alerts |
| 69 | `ina740x_alert_interrupt` | INA740X | alert_interrupt | `InaBridge228` | Bus over-voltage interrupt |

### Bridge Class Quick Reference

| Bridge Class | Supported Chips | Interface | Channels | Advanced Features |
|-----------|---------|------|------|---------|
| `InaBridge219` | INA219, INA220, INA220-Q1 | I²C | 1 | — |
| `InaBridge226` | INA226, INA226-Q1, INA230–234, INA236 | I²C | 1 | — |
| `InaBridge228` | INA228, INA228-Q1, INA237–239 (+Q1), INA740X | I²C | 1 | Temperature, energy, charge |
| `InaBridge229Spi` | INA229, INA229-Q1 | SPI | 1 | Temperature, energy, charge |
| `InaBridge3221` | INA3221, INA3221-Q1 | I²C | 3 | Per-channel control |
| `InaBridgeCh1` | INA2227, INA4230, INA4235 | I²C | 1 (CH1) | — |
| `InaBridgeUnknown` | — | — | — | Placeholder (no sensor) |

---

> **More Information:** For the complete API reference, see [API Reference (EN)](API_EN.md) | [API 参考手册 (中文)](API_CHN.md)  
> **INA Monitor Tool:** [NiusRobotLab_INA_monitor](https://github.com/dunknowcoding/NiusRobotLab_INA_monitor)  
> **Bug Reports:** [GitHub Issues](https://github.com/dunknowcoding/INA_series_sensor/issues)
