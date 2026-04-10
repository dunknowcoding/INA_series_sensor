# INA Series Sensor — Usage Guide

This guide describes how to install the library, initialize each bridge class, interpret JSON Lines output, and use the INA Monitor host application. It assumes **Arduino IDE 2.x** or **Arduino CLI** with a supported core (ESP32, Arduino-Pico for RP2040, AVR, etc.).

---

## Table of contents

1. [Installation](#1-installation)
2. [Include and minimal sketch](#2-include-and-minimal-sketch)
3. [Bridge classes](#3-bridge-classes)
4. [Lifecycle: `begin`, `tick`, streaming](#4-lifecycle-begin-tick-streaming)
5. [Serial output (JSON Lines)](#5-serial-output-json-lines)
6. [Host commands](#6-host-commands)
7. [INA Monitor desktop app](#7-ina-monitor-desktop-app)
8. [Calibration: `IMAX` and `RSHUNT`](#8-calibration-imax-and-rshunt)
9. [Advanced: protocol helpers](#9-advanced-protocol-helpers)
10. [Troubleshooting](#10-troubleshooting)

---

## 1. Installation

1. Copy the folder **`INA_series_sensor`** into your sketchbook **`libraries`** directory:
   - **Windows:** `Documents\Arduino\libraries\INA_series_sensor`
   - **macOS / Linux:** `~/Arduino/libraries/INA_series_sensor`
2. Restart the Arduino IDE or refresh the Library Manager index.
3. Confirm **Sketch → Include Library** lists **INA Series Sensor**.

The umbrella header is:

```cpp
#include <INA_Series_Sensor.h>
```

This pulls in all bridge classes plus **`InaJsonlProtocol.h`**, **`InaWireCompat.h`**, and **`InaSpiCompat.h`**.

---

## 2. Include and minimal sketch

Every example follows the same pattern:

1. **`Serial.begin(115200)`** — JSON Lines use **115200 baud** by default (INA Monitor expects this unless you change both sides).
2. A short **`delay(100)`** after boot so USB CDC can enumerate before the first `INFO` line (optional but common).
3. **`beginI2c(...)`**, **`beginSpi()`**, or **`begin()`** on the bridge object.
4. **`tick()`** in **`loop()`** — handles incoming serial **commands** and emits **samples** when streaming is enabled.

**I²C example (INA228 family):**

```cpp
#include <INA_Series_Sensor.h>

static InaBridge228 g_bridge("INA228", 0x40, "TI INA228");

void setup() {
  Serial.begin(115200);
  delay(100);
  g_bridge.beginI2c(8, 9);  // SDA, SCL — see WIRING.md for your board
}

void loop() {
  g_bridge.tick();
}
```

**SPI example (INA229):**

```cpp
#include <INA_Series_Sensor.h>

static InaBridge229Spi g_bridge("INA229", "TI INA229", 7, 4, 5, 6);

void setup() {
  Serial.begin(115200);
  delay(100);
  g_bridge.beginSpi();
}

void loop() {
  g_bridge.tick();
}
```

**Placeholder (no hardware):**

```cpp
#include <INA_Series_Sensor.h>

static InaBridgeUnknown g_bridge;

void setup() {
  Serial.begin(115200);
  delay(100);
  g_bridge.begin();
}

void loop() {
  g_bridge.tick();
}
```

---

## 3. Bridge classes

Choose the class that matches your **device register map** and **bus**. The **first string argument** is the **`chip` field** in JSON and must match the **INA Monitor** chip name (e.g. `"INA237-Q1"`, not a loose description).

| Class | Typical parts | Bus | JSON notes |
|--------|----------------|-----|------------|
| **`InaBridge219`** | INA219, INA220 | I²C | Includes **`shunt_uV`**. Calibration factor **0.04096** (datasheet). |
| **`InaBridge226`** | INA226, INA230–234, INA236 | I²C | Includes **`shunt_uV`**. Calibration **0.00512**. |
| **`InaBridge228`** | INA228, INA237/238/239, INA740X, … | I²C | No **`shunt_uV`** in sample line. Verifies **MFG ID 0x5449** @ **0x3E**. |
| **`InaBridge3221`** | INA3221, INA3221-Q1 | I²C | **`channels`** array (3 entries), aggregate **`bus_V`**. |
| **`InaBridge229Spi`** | INA229, INA229-Q1 | SPI | Same conversion idea as INA228; JSON **`addr`** is **`"SPI"`**. |
| **`InaBridgeCh1`** | INA2227, INA4230, INA4235 | I²C | Uses **CH1** registers only; custom INFO **`msg`** string. |
| **`InaBridgeUnknown`** | UI id `UNKNOWN` | — | Zeros only; for UI testing without a sensor. |

### Constructors (summary)

- **`InaBridge219(const char* chipJson, uint8_t i2cAddr = 0x40)`**
- **`InaBridge226(const char* chipJson, uint8_t i2cAddr = 0x40, const char* ref = "TI SLYSF02")`**
- **`InaBridge228(const char* chipJson, uint8_t i2cAddr = 0x40, const char* ref = "TI INA228")`**
- **`InaBridge3221(const char* chipJson, uint8_t i2cAddr = 0x40)`**
- **`InaBridge229Spi(const char* chipJson, const char* ref, int pinCs = 7, int pinSck = 4, int pinMiso = 5, int pinMosi = 6)`**
- **`InaBridgeCh1(const char* chipJson, const char* infoMsg, uint8_t i2cAddr = 0x40)`**
- **`InaBridgeUnknown`** — no constructor arguments.

### `begin` methods

| Method | Purpose |
|--------|---------|
| **`void beginI2c(int pinSda, int pinScl, uint32_t i2cHz = 400000)`** | Initialize **`Wire`**, apply chip config/calibration, print **`INFO`** JSON line. Used by all I²C bridges. |
| **`void beginSpi(uint32_t spiHz = 10000000)`** | Initialize **`SPI`** with pins from constructor; used by **`InaBridge229Spi`** only. |
| **`void begin()`** | **`InaBridgeUnknown`** only — prints **`INFO`**, no bus. |

---

## 4. Lifecycle: `begin`, `tick`, streaming

1. After **`begin*`**, the firmware emits one **`INFO`** object so the host can show the bridge name and **I²C address** (or **`SPI`**).
2. **`tick()`** must run frequently (typically every **`loop()`**):
   - Reads a **line** from **`Serial`** if present and parses **commands** (`PING`, `START`, `STOP`, etc.).
   - If streaming is **on** and the sample interval has elapsed, emits one **measurement** JSON object.

3. **Streaming is off** until the host sends **`START`** (or you add code to enable it — not done in stock examples). INA Monitor sends **`START`** when you begin a capture session.

4. Default **nominal sample rate** is **10 Hz**, adjustable with **`SR <hz>`** (clamped **1–200** Hz in firmware).

---

## 5. Serial output (JSON Lines)

- One **JSON object** per **line**, terminated by **LF** (`\n`).
- Protocol version field: **`"v":1`**.

### `INFO` (boot)

Emitted once after successful init. Shape varies slightly by class; always includes **`"type":"INFO"`** and author metadata.

### Measurement sample (single-channel)

Typical fields:

| Field | Meaning |
|-------|---------|
| **`chip`** | Must match INA Monitor selection. |
| **`addr`** | I²C address string (e.g. **`"0x40"`**) or **`"SPI"`**. |
| **`seq`** | Incrementing sequence number. |
| **`t_ms`** | **`millis()`** on the MCU. |
| **`bus_V`** | Bus voltage in volts. |
| **`shunt_uV`** | Shunt voltage in microvolts (219/226 only when present). |
| **`current_A`** | Current in amperes. |
| **`power_W`** | Power in watts. |

### INA3221 (multi-channel)

Adds **`channels`**: array of objects with **`bus_V`**, **`current_A`**, **`power_W`** per channel. An aggregate **`bus_V`** may appear for display averaging — keep field names unchanged for INA Monitor.

### Control / status lines

| Type | Typical use |
|------|-------------|
| **`PONG`** | Reply to **`PING`**. |
| **`ACK`** | Confirms **`START`**, **`STOP`**, **`SR`**, **`IMAX`**, **`RSHUNT`**. |
| **`ERR`** | Unknown command or parse issue; may include **`line`** echo. |

Do **not** rename measurement fields if you need **INA Monitor** compatibility.

---

## 6. Host commands

Send **ASCII lines** terminated by **newline** (`\n`). Matching is case-insensitive where noted in firmware.

| Command | Behavior |
|---------|----------|
| **`PING`** | Response: **`PONG`**. |
| **`START`** | Enable streaming samples. |
| **`STOP`** | Disable streaming. |
| **`SR <hz>`** | Set nominal rate **1–200** Hz. |
| **`IMAX <A>`** | Set expected max current for **Calibration** math (classes that implement it). Invalid/zero may reset to a default. |
| **`RSHUNT <ohm>`** | Set shunt resistance. Must be **> 0** where implemented. |

**Note:** **`InaBridge3221`** and **`InaBridgeCh1`** do not implement **`IMAX`** in command handlers (INA3221 example uses **`RSHUNT`** for channel current derivation). **`InaBridgeUnknown`** accepts **`PING`**, **`START`**, **`STOP`**, **`SR`** only.

---

## 7. INA Monitor desktop app

1. Connect the board via **USB** (virtual COM / CDC).
2. Open **INA Monitor**, choose the **Serial** data source and the **COM port** at **115200** baud.
3. Select the **same chip name** as in your sketch’s **`chipJson`** string (e.g. **`INA238-Q1`**).
4. Start monitoring — the app sends **`START`**, **`STOP`**, **`SR`**, and calibration commands as designed.

If the chip name in JSON does not match the UI selection, charts and scaling may not align with expectations.

---

## 8. Calibration: `IMAX` and `RSHUNT`

These commands adjust internal scaling so **current** and **power** match your **shunt resistor** and expected **full-scale current**:

- **`InaBridge219` / `InaBridge226`**: **`Calibration`** register derived from **`IMAX`** and **`RSHUNT`** per TI formulas used in code.
- **`InaBridge228` / `InaBridge229Spi`**: **`SHUNT_CAL`** updated from **`IMAX`** and **`RSHUNT`** (digital family).
- **`InaBridge3221`**: **`RSHUNT`** affects per-channel **current** from shunt voltage; **`IMAX`** not used in the command handler.
- **`InaBridgeCh1`**: **`RSHUNT`** only.

Always use a shunt rated for your application; verify **bus voltage** and **common-mode** limits per the **TI datasheet**.

---

## 9. Advanced: protocol helpers

For a **custom** bridge that must stay compatible with INA Monitor:

- Include **`InaJsonlProtocol.h`** and use **`InaJsonl::pong()`**, **`ackStart()`**, **`ackSr()`**, etc., so **ACK**/**ERR** lines stay identical.
- Use **`InaWireBeginMapped()`** / **`InaSpiBeginMapped()`** from **`InaWireCompat.h`** / **`InaSpiCompat.h`** for portable **`Wire`**/**`SPI`** setup.

---

## 10. Troubleshooting

| Symptom | Things to check |
|---------|------------------|
| No **`INFO` line** | **`Serial` baud** (115200), **`begin*`** actually reached, **I²C/SPI** wiring and power. |
| **`INFO` but no samples** | Host must send **`START`**. Raw Serial terminal: type **`START`** + Enter. |
| **`ERR` / MFG ID mismatch (228/229)** | Wrong chip or bad bus; verify **address**, **wiring**, **3V3** to INA. |
| Garbage on serial | **Baud mismatch**; use **115200** on both sides. |
| Wrong current scale | **`IMAX`**, **`RSHUNT`**, and physical **shunt value** must match. |
| INA Monitor empty graph | **Chip name** in JSON must match UI; **JSON** must be one object per line. |

For **pin mapping** and **per-board** connections, see **[WIRING.md](./WIRING.md)**.
