# INA Series Sensor — API Reference

> **Library Version Note**　This document covers all Bridge-layer public APIs of the `INA_Series_Sensor` Arduino library.
>
> **Supported Chips**　INA219 · INA220 · INA226 · INA228 · INA229(SPI) · INA230–239 · INA740X · INA2227 · INA3221 · INA4230 · INA4235
>
> **Header File**　`#include <INA_Series_Sensor.h>`　— a single include gives access to all Bridge classes.

---

## Table of Contents

- [1. Overview](#1-overview)
- [2. Class Overview and Feature Matrix](#2-class-overview-and-feature-matrix)
- [3. InaBridge228 — INA228/237/238/239/740X (I²C)](#3-inabridge228)
- [4. InaBridge219 — INA219/INA220 (I²C)](#4-inabridge219)
- [5. InaBridge226 — INA226/INA230–236 (I²C)](#5-inabridge226)
- [6. InaBridge229Spi — INA229/INA229-Q1 (SPI)](#6-inabridge229spi)
- [7. InaBridge3221 — INA3221/INA3221-Q1 (Three-Channel I²C)](#7-inabridge3221)
- [8. InaBridgeCh1 — INA2227/INA4230/INA4235 (Channel 1 Only)](#8-inabridgech1)
- [9. InaBridgeUnknown — No-Sensor Placeholder](#9-inabridgeunknown)
- [10. Serial Protocol Commands](#10-serial-protocol-commands)

---

## 1. Overview

This library provides a unified Arduino driver for TI INA series current/voltage/power monitoring chips, supporting two usage modes:

| Mode | Description |
|------|-------------|
| **Mode A — JSONL Streaming** | Call `tick()` in `loop()`. The host sends `START`/`STOP`/`SR` commands over serial, and the Bridge outputs measurement data in JSON Lines format. Compatible with `NiusRobotLab_INA_monitor`. |
| **Mode B — Standalone Direct Read** | After calling `begin()` to initialize, directly call `readBusVoltage()`, `readCurrent()`, etc. to obtain measurements without serial output. Optionally combine with `dataReady()` to poll for conversion completion. |

Both modes can be used simultaneously in the same sketch.

### Quick Start

```cpp
#include <INA_Series_Sensor.h>

static InaBridge228 sensor("INA228", 0x40);

void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.begin(8, 9);       // SDA=8, SCL=9
}

void loop() {
  sensor.tick();             // Mode A: JSONL streaming

  // Mode B: standalone read
  if (sensor.dataReady()) {
    float v = sensor.readBusVoltage();
    float i = sensor.readCurrent();
    float p = sensor.readPower();
  }
}
```

---

## 2. Class Overview and Feature Matrix

| Feature | InaBridge228 | InaBridge219 | InaBridge226 | InaBridge229Spi | InaBridge3221 | InaBridgeCh1 | InaBridgeUnknown |
|---------|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| Bus | I²C | I²C | I²C | SPI | I²C | I²C | — |
| readBusVoltage | ✔ | ✔ | ✔ | ✔ | ✔ (multi-ch) | ✔ | ✘ |
| readShuntVoltage | ✔ | ✔ | ✔ | ✔ | ✔ (multi-ch) | ✔ | ✘ |
| readCurrent | ✔ | ✔ | ✔ | ✔ | ✔ (multi-ch) | ✔ | ✘ |
| readPower | ✔ | ✔ | ✔ | ✔ | ✔ (multi-ch) | ✔ | ✘ |
| readDieTemp | ✔ | ✘ | ✘ | ✔ | ✘ | ✘ | ✘ |
| readEnergy / readCharge | ✔ | ✘ | ✘ | ✔ | ✘ | ✘ | ✘ |
| resetAccumulators | ✔ | ✘ | ✘ | ✔ | ✘ | ✘ | ✘ |
| dataReady | ✔ | ✔ | ✔ | ✔ | ✔ | ✘ | ✘ |
| setImax | ✔ | ✔ | ✔ | ✔ | ✘ | ✘ | ✘ |
| setRshunt | ✔ | ✔ | ✔ | ✔ | ✔ (multi-ch) | ✔ | ✘ |
| setExtraFieldsPrinter | ✔ | ✘ | ✘ | ✔ | ✔ | ✘ | ✘ |
| enableChannel | ✘ | ✘ | ✘ | ✘ | ✔ | ✘ | ✘ |
| Max sample rate (Hz) | 400 | — | — | 2000 | — | 400 | 400 |

---

## 3. InaBridge228

**Supported Chips**　INA228 · INA228-Q1 · INA237 · INA237-Q1 · INA238 · INA238-Q1 · INA239 · INA239-Q1 · INA740X

**Header File**　`InaBridge228.h`

### Constructor

```cpp
InaBridge228(const char* chipJson, uint8_t i2cAddr = 0x40, const char* ref = "TI INA228");
```

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `chipJson` | `const char*` | — | Chip name string, used as the `"chip"` field in JSONL output, e.g. `"INA228"` |
| `i2cAddr` | `uint8_t` | `0x40` | I²C slave address (7-bit), range 0x40–0x4F |
| `ref` | `const char*` | `"TI INA228"` | Reference manual identifier, used in INFO output |

**Example**

```cpp
static InaBridge228 sensor("INA228", 0x40);
static InaBridge228 sensor2("INA237", 0x45, "TI INA237");
```

---

### begin

```cpp
void begin(int pinSda = 8, int pinScl = 9, uint32_t i2cHz = 400000);
```

Initializes the I²C bus, probes the chip, resets, configures continuous conversion mode (temperature + bus voltage + shunt voltage), performs calibration, and prints an INFO line to serial.

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `pinSda` | `int` | `8` | SDA pin (ESP32: GPIO remapping; other platforms: ignored, uses board default) |
| `pinScl` | `int` | `9` | SCL pin (ESP32: GPIO remapping; other platforms: ignored, uses board default) |
| `i2cHz` | `uint32_t` | `400000` | I²C clock frequency (Hz), default 400 kHz |

**Return**　None

**Example**

```cpp
void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.begin(8, 9);          // ESP32-C3: SDA=GPIO8, SCL=GPIO9
  sensor.begin();              // Use all defaults
  sensor.begin(21, 22, 100000); // ESP32: SDA=21, SCL=22, 100 kHz
}
```

> **Note**　`begin()` must be called before invoking any measurement method.

---

### beginI2c

```cpp
void beginI2c(int pinSda = 8, int pinScl = 9, uint32_t i2cHz = 400000);
```

Legacy alias for `begin()`; functionally identical. New code should use `begin()`.

---

### tick

```cpp
void tick();
```

Processes serial commands and sends JSONL data in streaming mode. **Must be called once per `loop()` iteration.**

**Return**　None

**Example**

```cpp
void loop() {
  sensor.tick();
}
```

> **Note**　Even if you are not using JSONL streaming, calling `tick()` is safe (it silently handles serial input).

---

### readBusVoltage

```cpp
float readBusVoltage();
```

Reads the bus voltage.

**Return**　`float` — Bus voltage in volts (V).

**Register precision**　24-bit, LSB = 195.3125 µV.

**Example**

```cpp
float busV = sensor.readBusVoltage();
Serial.print("Bus: ");
Serial.print(busV, 4);
Serial.println(" V");
```

---

### readShuntVoltage

```cpp
float readShuntVoltage();
```

Reads the shunt voltage.

**Return**　`float` — Shunt voltage in volts (V).

**Register precision**　24-bit. LSB depends on the ADCRANGE configuration bit:
- ADCRANGE = 0 (default, ±163.84 mV): LSB = 312.5 nV
- ADCRANGE = 1 (±40.96 mV): LSB = 78.125 nV

**Example**

```cpp
float shuntV = sensor.readShuntVoltage();
Serial.print("Shunt: ");
Serial.print(shuntV * 1e6, 1);  // Convert to µV for display
Serial.println(" uV");
```

---

### readCurrent

```cpp
float readCurrent();
```

Reads the current value. Requires valid calibration (either `begin()` has been called, or `setRshunt` + `setImax` have been manually configured).

**Return**　`float` — Current in amperes (A).

**Example**

```cpp
float current = sensor.readCurrent();
Serial.print("Current: ");
Serial.print(current, 4);
Serial.println(" A");
```

> **Note**　If calibration is incorrect, the returned value will be inaccurate.

---

### readPower

```cpp
float readPower();
```

Reads the power value.

**Return**　`float` — Power in watts (W).

**Example**

```cpp
float power = sensor.readPower();
Serial.print("Power: ");
Serial.print(power, 4);
Serial.println(" W");
```

---

### readDieTemp

```cpp
float readDieTemp();
```

Reads the internal (die) temperature of the chip.

**Return**　`float` — Temperature in degrees Celsius (°C).

**Register precision**　LSB = 7.8125 m°C.

**Example**

```cpp
float temp = sensor.readDieTemp();
Serial.print("Die Temp: ");
Serial.print(temp, 2);
Serial.println(" °C");
```

---

### readEnergy

```cpp
float readEnergy();
```

Reads the accumulated energy since the last reset.

**Return**　`float` — Accumulated energy in joules (J).

**Example**

```cpp
float energy = sensor.readEnergy();
Serial.print("Energy: ");
Serial.print(energy, 6);
Serial.println(" J");
```

> **Note**　You must call `resetAccumulators()` to clear the accumulators first in order to obtain meaningful measurements.

---

### readCharge

```cpp
float readCharge();
```

Reads the accumulated charge since the last reset (signed value, supports bidirectional current).

**Return**　`float` — Accumulated charge in coulombs (C).

**Example**

```cpp
float charge = sensor.readCharge();
Serial.print("Charge: ");
Serial.print(charge, 6);
Serial.println(" C");
```

---

### resetAccumulators

```cpp
void resetAccumulators();
```

Clears the energy and charge accumulation registers to zero.

**Return**　None

**Example**

```cpp
sensor.resetAccumulators();
// Energy and charge measurement starts from this point
```

---

### dataReady

```cpp
bool dataReady();
```

Checks the CNVRF (conversion ready) flag in the DIAG_ALRT register.

**Return**　`bool` — `true` indicates new conversion data is available for reading.

**Example**

```cpp
if (sensor.dataReady()) {
  float v = sensor.readBusVoltage();
  float i = sensor.readCurrent();
}
```

> **Important**　Reading the DIAG_ALRT register **clears** the CNVRF flag. Therefore, calling `dataReady()` twice in succession will cause the second call to return `false`.
>
> When using both Mode A (streaming) and Mode B (direct read) simultaneously, `dataReady()` may return `false` within the streaming sample interval because the streaming code also reads the same register and clears the flag.

---

### startStreaming

```cpp
void startStreaming();
```

Starts JSONL streaming output. Equivalent to sending the `START` command over serial.

**Return**　None

---

### stopStreaming

```cpp
void stopStreaming();
```

Stops JSONL streaming output. Equivalent to sending the `STOP` command over serial.

**Return**　None

---

### isStreaming

```cpp
bool isStreaming() const;
```

**Return**　`bool` — `true` indicates JSONL streaming is in progress.

---

### setSampleRate

```cpp
void setSampleRate(int hz);
```

Sets the sample rate for JSONL streaming output.

| Parameter | Type | Description |
|-----------|------|-------------|
| `hz` | `int` | Sample rate (Hz), automatically clamped to **1–400** |

**Return**　None

**Example**

```cpp
sensor.setSampleRate(100);  // 100 Hz
```

---

### setRshunt

```cpp
void setRshunt(float ohm);
```

Sets the shunt resistor value and recalibrates.

| Parameter | Type | Description |
|-----------|------|-------------|
| `ohm` | `float` | Shunt resistance (Ω), minimum 0.0001 Ω |

**Return**　None

**Example**

```cpp
sensor.setRshunt(0.01);  // 10 mΩ shunt resistor
```

---

### setImax

```cpp
void setImax(float ampere);
```

Sets the maximum expected current and recalibrates. This value affects the `current_LSB` calculation.

| Parameter | Type | Description |
|-----------|------|-------------|
| `ampere` | `float` | Maximum expected current (A), must be > 0 |

**Return**　None

**Example**

```cpp
sensor.setImax(20.0);  // Maximum 20 A
```

---

### rshunt

```cpp
float rshunt() const;
```

**Return**　`float` — Current shunt resistance value (Ω). Default 0.1 Ω.

---

### imax

```cpp
float imax() const;
```

**Return**　`float` — Current maximum expected current (A). Default 10.0 A.

---

### currentLsb

```cpp
float currentLsb() const;
```

**Return**　`float` — Current calibrated current LSB value (A/LSB).

---

### address

```cpp
uint8_t address() const;
```

**Return**　`uint8_t` — I²C slave address (7-bit).

---

### setExtraFieldsPrinter

```cpp
typedef void (*ExtraFieldsFn)();
void setExtraFieldsPrinter(ExtraFieldsFn fn);
```

Registers a callback function that is invoked before the closing brace of each JSONL measurement data line. Can be used to inject additional fields into the JSONL output.

| Parameter | Type | Description |
|-----------|------|-------------|
| `fn` | `ExtraFieldsFn` | Callback function pointer with signature `void (*)()` |

**Return**　None

**Example**

```cpp
static Ina::I2cBus       g_i2c;
static Ina::Ina228Driver g_drv(g_i2c, 0x40);

static void printExtraFields() {
  float temp;
  if (g_drv.readDieTemp_C(temp).ok()) {
    Serial.print(F(",\"temp_C\":"));
    Serial.print(temp, 2);
  }
}

void setup() {
  Serial.begin(115200);
  sensor.begin(8, 9);
  sensor.setExtraFieldsPrinter(printExtraFields);
}
```

> **Note**　The callback function must write directly to `Serial`, starting with a comma, and output valid JSON key-value pairs. `NiusRobotLab_INA_monitor` ignores unrecognized extra fields.

---

## 4. InaBridge219

**Supported Chips**　INA219 · INA220 · INA220-Q1

**Header File**　`InaBridge219.h`

### Constructor

```cpp
InaBridge219(const char* chipJson, uint8_t i2cAddr = 0x40);
```

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `chipJson` | `const char*` | — | Chip name string, e.g. `"INA219"` |
| `i2cAddr` | `uint8_t` | `0x40` | I²C slave address (7-bit) |

**Example**

```cpp
static InaBridge219 sensor("INA219", 0x40);
```

---

### begin

```cpp
void begin(int pinSda = 8, int pinScl = 9, uint32_t i2cHz = 400000);
```

Initializes the I²C bus, configures the ADC, performs calibration, and prints an INFO line to serial. Parameters are the same as [InaBridge228::begin](#begin).

---

### beginI2c

```cpp
void beginI2c(int pinSda = 8, int pinScl = 9, uint32_t i2cHz = 400000);
```

Legacy alias for `begin()`.

---

### tick

```cpp
void tick();
```

Processes serial commands and sends JSONL data in streaming mode. **Must be called once per `loop()` iteration.**

---

### readBusVoltage

```cpp
float readBusVoltage();
```

Reads the bus voltage.

**Return**　`float` — Bus voltage (V).

**Register precision**　16-bit, LSB = 4 mV.

---

### readShuntVoltage

```cpp
float readShuntVoltage();
```

Reads the shunt voltage.

**Return**　`float` — Shunt voltage (V).

**Register precision**　16-bit signed, LSB = 10 µV.

---

### readCurrent

```cpp
float readCurrent();
```

Reads the current value. Requires valid calibration.

**Return**　`float` — Current (A).

**Calibration formula**　`current_LSB = imax / 32768`

---

### readPower

```cpp
float readPower();
```

Reads the power value.

**Return**　`float` — Power (W).

**Calibration formula**　`power_LSB = 20 × current_LSB`

---

### dataReady

```cpp
bool dataReady();
```

Checks the CNVR (conversion ready) flag (bit 1) in the Bus Voltage register.

**Return**　`bool` — `true` indicates new conversion data is available for reading.

> **Important**　The CNVR flag is not cleared by reading the Bus Voltage register itself; **reading the Power register clears the flag**. Therefore, `dataReady()` will return `false` after a `readPower()` call.

---

### startStreaming / stopStreaming / isStreaming

```cpp
void startStreaming();
void stopStreaming();
bool isStreaming() const;
```

Functionally identical to the corresponding [InaBridge228](#startstreaming) methods.

---

### setSampleRate

```cpp
void setSampleRate(int hz);
```

Sets the sample rate for JSONL streaming output, automatically clamped to the valid range.

---

### setRshunt

```cpp
void setRshunt(float ohm);
```

Sets the shunt resistor value and recalibrates. See [InaBridge228::setRshunt](#setrshunt) for parameter details.

---

### setImax

```cpp
void setImax(float ampere);
```

Sets the maximum expected current and recalibrates. See [InaBridge228::setImax](#setimax) for parameter details.

**Default**　3.2 A (full-scale default for INA219).

---

### rshunt / imax / address

```cpp
float rshunt() const;    // Default 0.1 Ω
float imax() const;      // Default 3.2 A
uint8_t address() const;
```

Returns current configuration values; semantics are the same as the corresponding InaBridge228 methods.

---

## 5. InaBridge226

**Supported Chips**　INA226 · INA226-Q1 · INA230 · INA231 · INA232 · INA233 · INA234 · INA235 · INA236

**Header File**　`InaBridge226.h`

### Constructor

```cpp
InaBridge226(const char* chipJson, uint8_t i2cAddr = 0x40, const char* ref = "TI SLYSF02");
```

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `chipJson` | `const char*` | — | Chip name string, e.g. `"INA226"` |
| `i2cAddr` | `uint8_t` | `0x40` | I²C slave address (7-bit) |
| `ref` | `const char*` | `"TI SLYSF02"` | Reference manual identifier |

**Example**

```cpp
static InaBridge226 sensor("INA226", 0x40);
static InaBridge226 sensor2("INA234", 0x44, "TI INA234");
```

---

### begin

```cpp
void begin(int pinSda = 8, int pinScl = 9, uint32_t i2cHz = 400000);
```

Initializes the I²C bus, configures the ADC, performs calibration, and prints an INFO line to serial. Parameters are the same as [InaBridge228::begin](#begin).

---

### beginI2c

```cpp
void beginI2c(int pinSda = 8, int pinScl = 9, uint32_t i2cHz = 400000);
```

Legacy alias for `begin()`.

---

### tick

```cpp
void tick();
```

Processes serial commands and sends JSONL data in streaming mode. **Must be called once per `loop()` iteration.**

---

### readBusVoltage

```cpp
float readBusVoltage();
```

Reads the bus voltage.

**Return**　`float` — Bus voltage (V).

**Register precision**　16-bit, LSB = 1.25 mV.

---

### readShuntVoltage

```cpp
float readShuntVoltage();
```

Reads the shunt voltage.

**Return**　`float` — Shunt voltage (V).

**Register precision**　16-bit signed, LSB = 2.5 µV.

---

### readCurrent

```cpp
float readCurrent();
```

Reads the current value. Requires valid calibration.

**Return**　`float` — Current (A).

**Calibration formula**　`current_LSB = imax / 32768`

---

### readPower

```cpp
float readPower();
```

Reads the power value.

**Return**　`float` — Power (W).

**Calibration formula**　`power_LSB = 25 × current_LSB`

---

### dataReady

```cpp
bool dataReady();
```

Checks the CVRF (conversion ready) flag (bit 3) in the Mask/Enable register (0x06).

**Return**　`bool` — `true` indicates new conversion data is available for reading.

> **Important**　Reading the Mask/Enable register **clears** the CVRF flag. Calling `dataReady()` twice in succession will cause the second call to return `false`.

---

### startStreaming / stopStreaming / isStreaming

```cpp
void startStreaming();
void stopStreaming();
bool isStreaming() const;
```

Functionally identical to the corresponding [InaBridge228](#startstreaming) methods.

---

### setSampleRate

```cpp
void setSampleRate(int hz);
```

Sets the sample rate for JSONL streaming output, automatically clamped to the valid range.

---

### setRshunt

```cpp
void setRshunt(float ohm);
```

Sets the shunt resistor value and recalibrates. See [InaBridge228::setRshunt](#setrshunt) for parameter details.

---

### setImax

```cpp
void setImax(float ampere);
```

Sets the maximum expected current and recalibrates.

**Default**　3.2 A.

---

### rshunt / imax / address

```cpp
float rshunt() const;    // Default 0.1 Ω
float imax() const;      // Default 3.2 A
uint8_t address() const;
```

Returns current configuration values.

---

## 6. InaBridge229Spi

**Supported Chips**　INA229 · INA229-Q1

**Header File**　`InaBridge229Spi.h`

**Bus**　SPI (not I²C)

> The measurement math of the INA229 is identical to the INA228; only the communication bus differs.

### Constructor

```cpp
InaBridge229Spi(const char* chipJson, const char* ref,
                int pinCs = 7, int pinSck = 4, int pinMiso = 5, int pinMosi = 6);
```

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `chipJson` | `const char*` | — | Chip name string, e.g. `"INA229"` |
| `ref` | `const char*` | — | Reference manual identifier, e.g. `"TI INA229"` |
| `pinCs` | `int` | `7` | SPI chip select (CS) pin |
| `pinSck` | `int` | `4` | SPI clock (SCK) pin |
| `pinMiso` | `int` | `5` | SPI MISO pin |
| `pinMosi` | `int` | `6` | SPI MOSI pin |

**Example**

```cpp
static InaBridge229Spi sensor("INA229", "TI INA229");
static InaBridge229Spi sensor2("INA229-Q1", "TI INA229-Q1", 10, 13, 12, 11);
```

---

### beginSpi

```cpp
void beginSpi(uint32_t spiHz = 10000000);
```

Initializes the SPI bus, probes the chip, resets, configures ADC mode, performs calibration, and prints an INFO line to serial.

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `spiHz` | `uint32_t` | `10000000` | SPI clock frequency (Hz), default 10 MHz |

**Return**　None

**Example**

```cpp
void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.beginSpi();             // Default 10 MHz
  sensor.beginSpi(1000000);      // 1 MHz
}
```

---

### begin

```cpp
void begin(uint32_t spiHz = 10000000);
```

Alias for `beginSpi()`; functionally identical.

---

### tick

```cpp
void tick();
```

Processes serial commands and sends JSONL data in streaming mode. Uses `micros()` timing to support higher sample rates. **Must be called once per `loop()` iteration.**

---

### readBusVoltage

```cpp
float readBusVoltage();
```

Reads the bus voltage.

**Return**　`float` — Bus voltage (V).

**Register precision**　24-bit, LSB = 195.3125 µV (same as INA228).

---

### readShuntVoltage

```cpp
float readShuntVoltage();
```

Reads the shunt voltage.

**Return**　`float` — Shunt voltage (V).

**Register precision**　24-bit, LSB depends on ADCRANGE (same as INA228).

---

### readCurrent

```cpp
float readCurrent();
```

**Return**　`float` — Current (A). Requires valid calibration.

---

### readPower

```cpp
float readPower();
```

**Return**　`float` — Power (W).

---

### readDieTemp

```cpp
float readDieTemp();
```

**Return**　`float` — Die temperature (°C). LSB = 7.8125 m°C.

---

### readEnergy

```cpp
float readEnergy();
```

**Return**　`float` — Accumulated energy since last reset (J).

---

### readCharge

```cpp
float readCharge();
```

**Return**　`float` — Accumulated charge since last reset (C) (signed).

---

### resetAccumulators

```cpp
void resetAccumulators();
```

Clears the energy and charge accumulation registers to zero.

---

### dataReady

```cpp
bool dataReady();
```

Checks the CNVRF flag in the DIAG_ALRT register.

**Return**　`bool` — `true` indicates new conversion data is available for reading.

> **Important**　Reading clears the CNVRF flag. Behavior is the same as [InaBridge228::dataReady](#dataready).

---

### startStreaming / stopStreaming / isStreaming

```cpp
void startStreaming();
void stopStreaming();
bool isStreaming() const;
```

Functionally identical to the corresponding InaBridge228 methods.

---

### setSampleRate

```cpp
void setSampleRate(int hz);
```

Sets the sample rate for JSONL streaming output.

| Parameter | Type | Description |
|-----------|------|-------------|
| `hz` | `int` | Sample rate (Hz), automatically clamped to **1–2000** (SPI is faster than I²C) |

---

### setRshunt

```cpp
void setRshunt(float ohm);
```

Sets the shunt resistor value and recalibrates. Minimum 0.0001 Ω.

---

### setImax

```cpp
void setImax(float ampere);
```

Sets the maximum expected current and recalibrates. Must be > 0.

---

### rshunt / imax / currentLsb

```cpp
float rshunt() const;     // Default 0.1 Ω
float imax() const;       // Default 10.0 A
float currentLsb() const;
```

Returns current configuration values.

> **Note**　InaBridge229Spi does **not** have an `address()` method (SPI uses chip select pins instead of addresses).

---

### setExtraFieldsPrinter

```cpp
typedef void (*ExtraFieldsFn)();
void setExtraFieldsPrinter(ExtraFieldsFn fn);
```

Registers a callback function for injecting additional fields into JSONL output. Usage is the same as [InaBridge228::setExtraFieldsPrinter](#setextrafieldsprinter).

---

## 7. InaBridge3221

**Supported Chips**　INA3221 · INA3221-Q1

**Header File**　`InaBridge3221.h`

**Features**　Three independent measurement channels, each with separate bus voltage and shunt voltage readings. Current is calculated in software as `shuntV / rshunt`. Each channel can have an independently configured shunt resistor and can be individually enabled or disabled.

### Constructor

```cpp
InaBridge3221(const char* chipJson, uint8_t i2cAddr = 0x40);
```

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `chipJson` | `const char*` | — | Chip name string, e.g. `"INA3221"` |
| `i2cAddr` | `uint8_t` | `0x40` | I²C slave address (7-bit) |

**Example**

```cpp
static InaBridge3221 sensor("INA3221", 0x40);
```

---

### begin

```cpp
void begin(int pinSda = 8, int pinScl = 9, uint32_t i2cHz = 400000);
```

Initializes the I²C bus, probes the device, applies the default configuration, and prints an INFO line to serial. Parameters are the same as [InaBridge228::begin](#begin).

---

### beginI2c

```cpp
void beginI2c(int pinSda = 8, int pinScl = 9, uint32_t i2cHz = 400000);
```

Legacy alias for `begin()`.

---

### tick

```cpp
void tick();
```

Processes serial commands and sends JSONL data in streaming mode. **Must be called once per `loop()` iteration.**

---

### readBusVoltage

```cpp
float readBusVoltage(uint8_t ch);
```

Reads the bus voltage of the specified channel.

| Parameter | Type | Description |
|-----------|------|-------------|
| `ch` | `uint8_t` | Channel number: **1**, **2**, or **3**. Out-of-range returns 0 |

**Return**　`float` — Bus voltage (V).

**Example**

```cpp
float v1 = sensor.readBusVoltage(1);
float v2 = sensor.readBusVoltage(2);
float v3 = sensor.readBusVoltage(3);
```

---

### readShuntVoltage

```cpp
float readShuntVoltage(uint8_t ch);
```

Reads the shunt voltage of the specified channel.

| Parameter | Type | Description |
|-----------|------|-------------|
| `ch` | `uint8_t` | Channel number: **1**, **2**, or **3** |

**Return**　`float` — Shunt voltage (V).

---

### readCurrent

```cpp
float readCurrent(uint8_t ch);
```

Reads the current of the specified channel, calculated in software as `shuntVoltage / rshunt(ch)`.

| Parameter | Type | Description |
|-----------|------|-------------|
| `ch` | `uint8_t` | Channel number: **1**, **2**, or **3** |

**Return**　`float` — Current (A).

**Example**

```cpp
sensor.setRshunt(1, 0.1);   // CH1: 100 mΩ
sensor.setRshunt(2, 0.05);  // CH2:  50 mΩ
sensor.setRshunt(3, 0.01);  // CH3:  10 mΩ

float i1 = sensor.readCurrent(1);  // Uses CH1 rshunt
float i2 = sensor.readCurrent(2);  // Uses CH2 rshunt
```

---

### readPower

```cpp
float readPower(uint8_t ch);
```

Reads the power of the specified channel, calculated in software as `current(ch) × busVoltage(ch)`.

| Parameter | Type | Description |
|-----------|------|-------------|
| `ch` | `uint8_t` | Channel number: **1**, **2**, or **3** |

**Return**　`float` — Power (W).

---

### dataReady

```cpp
bool dataReady();
```

Checks the CVRF (conversion ready) flag in the Mask/Enable register.

**Return**　`bool` — `true` indicates new conversion data is available for reading.

> **Important**　Reading the Mask/Enable register clears the CVRF flag.

---

### enableChannel

```cpp
void enableChannel(uint8_t ch, bool enable = true);
```

Enables or disables the specified measurement channel. Disabled channels are skipped during conversion, reducing total conversion time and allowing higher sample rates.

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `ch` | `uint8_t` | — | Channel number: **1**, **2**, or **3** |
| `enable` | `bool` | `true` | `true` to enable, `false` to disable |

**Return**　None

**Example**

```cpp
sensor.enableChannel(3, false);  // Disable channel 3
sensor.enableChannel(1);         // Enable channel 1 (default true)
```

---

### isChannelEnabled

```cpp
bool isChannelEnabled(uint8_t ch);
```

Checks whether the specified channel is enabled.

| Parameter | Type | Description |
|-----------|------|-------------|
| `ch` | `uint8_t` | Channel number: **1**, **2**, or **3** |

**Return**　`bool` — `true` indicates the channel is enabled.

---

### startStreaming / stopStreaming / isStreaming

```cpp
void startStreaming();
void stopStreaming();
bool isStreaming() const;
```

Functionally identical to the corresponding InaBridge228 methods.

---

### setSampleRate

```cpp
void setSampleRate(int hz);
```

Sets the sample rate for JSONL streaming output.

---

### setRshunt (Single Channel)

```cpp
void setRshunt(uint8_t ch, float ohm);
```

Sets the shunt resistor value for the specified channel.

| Parameter | Type | Description |
|-----------|------|-------------|
| `ch` | `uint8_t` | Channel number: **1**, **2**, or **3** |
| `ohm` | `float` | Shunt resistance (Ω), must be > 0 |

**Return**　None

**Example**

```cpp
sensor.setRshunt(1, 0.1);   // CH1: 100 mΩ
sensor.setRshunt(2, 0.05);  // CH2:  50 mΩ
sensor.setRshunt(3, 0.01);  // CH3:  10 mΩ
```

---

### setRshunt (All Channels)

```cpp
void setRshunt(float ohm);
```

Sets the same shunt resistor value for all three channels.

| Parameter | Type | Description |
|-----------|------|-------------|
| `ohm` | `float` | Shunt resistance (Ω), must be > 0 |

**Return**　None

**Example**

```cpp
sensor.setRshunt(0.1);  // All channels: 100 mΩ
```

---

### rshunt

```cpp
float rshunt(uint8_t ch = 1) const;
```

Returns the shunt resistor value for the specified channel.

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `ch` | `uint8_t` | `1` | Channel number: **1**, **2**, or **3** |

**Return**　`float` — Shunt resistance (Ω). Default 0.1 Ω.

---

### address

```cpp
uint8_t address() const;
```

**Return**　`uint8_t` — I²C slave address (7-bit).

---

### setExtraFieldsPrinter

```cpp
typedef void (*ExtraFieldsFn)();
void setExtraFieldsPrinter(ExtraFieldsFn fn);
```

Registers a callback function for injecting additional fields into JSONL output. Usage is the same as [InaBridge228::setExtraFieldsPrinter](#setextrafieldsprinter).

> **Note**　InaBridge3221 does **not** have a `setImax()` method (the INA3221 has no calibration register; current is calculated via software division).

---

## 8. InaBridgeCh1

**Supported Chips**　INA2227 · INA4230 · INA4235

**Header File**　`InaBridgeCh1.h`

**Features**　Simplified driver that reads channel 1 only. Shunt voltage LSB = 40 µV, bus voltage LSB = 8 mV. Current is calculated in software as `shuntV / Rshunt`.

> **Limitations**　These chips have no calibration register and no documented conversion-ready flag, so `dataReady()`, `setImax()`, and `setExtraFieldsPrinter()` methods are not provided.

### Constructor

```cpp
InaBridgeCh1(const char* chipJson, const char* infoMsg, uint8_t i2cAddr = 0x40);
```

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `chipJson` | `const char*` | — | Chip name string, e.g. `"INA2227"` |
| `infoMsg` | `const char*` | — | Additional message text for the INFO line |
| `i2cAddr` | `uint8_t` | `0x40` | I²C slave address (7-bit) |

**Example**

```cpp
static InaBridgeCh1 sensor("INA4230", "CH1 only driver", 0x40);
```

---

### begin

```cpp
void begin(int pinSda = 8, int pinScl = 9, uint32_t i2cHz = 400000);
```

Initializes the I²C bus, probes the chip, applies the default configuration, and prints an INFO line to serial. Parameters are the same as [InaBridge228::begin](#begin).

---

### beginI2c

```cpp
void beginI2c(int pinSda = 8, int pinScl = 9, uint32_t i2cHz = 400000);
```

Legacy alias for `begin()`.

---

### tick

```cpp
void tick();
```

Processes serial commands and sends JSONL data in streaming mode. **Must be called once per `loop()` iteration.**

---

### readBusVoltage

```cpp
float readBusVoltage();
```

Reads the bus voltage.

**Return**　`float` — Bus voltage (V).

**Register precision**　LSB = 8 mV.

---

### readShuntVoltage

```cpp
float readShuntVoltage();
```

Reads the shunt voltage.

**Return**　`float` — Shunt voltage (V).

**Register precision**　16-bit signed, LSB = 40 µV.

---

### readCurrent

```cpp
float readCurrent();
```

Reads the current value. Calculated in software as `shuntV / Rshunt`.

**Return**　`float` — Current (A).

---

### readPower

```cpp
float readPower();
```

Reads the power value. Calculated in software as `current × busVoltage`.

**Return**　`float` — Power (W).

---

### startStreaming / stopStreaming / isStreaming

```cpp
void startStreaming();
void stopStreaming();
bool isStreaming() const;
```

Functionally identical to the corresponding InaBridge228 methods.

---

### setSampleRate

```cpp
void setSampleRate(int hz);
```

Sets the sample rate for JSONL streaming output, automatically clamped to **1–400** Hz.

---

### setRshunt

```cpp
void setRshunt(float ohm);
```

Sets the shunt resistor value.

| Parameter | Type | Description |
|-----------|------|-------------|
| `ohm` | `float` | Shunt resistance (Ω), minimum 0.0001 Ω; values below this are clamped to 0.1 Ω |

**Return**　None

---

### rshunt / address

```cpp
float rshunt() const;     // Default 0.1 Ω
uint8_t address() const;
```

Returns current configuration values.

---

## 9. InaBridgeUnknown

**Purpose**　Placeholder Bridge used when no sensor is detected. Always outputs zero values. Accepts the same `START`/`STOP`/`SR` serial commands as other Bridges, so the host application requires no special handling.

**Header File**　`InaBridgeUnknown.h`

### begin

```cpp
void begin();
```

Prints an INFO line to serial. No parameters required (no hardware initialization).

**Return**　None

**Example**

```cpp
static InaBridgeUnknown fallback;

void setup() {
  Serial.begin(115200);
  fallback.begin();
}
```

---

### tick

```cpp
void tick();
```

Processes serial commands and outputs all-zero JSONL data in streaming mode. **Must be called once per `loop()` iteration.**

---

### startStreaming / stopStreaming / isStreaming

```cpp
void startStreaming();
void stopStreaming();
bool isStreaming() const;
```

Functionally identical to the corresponding InaBridge228 methods.

---

### setSampleRate

```cpp
void setSampleRate(int hz);
```

Sets the sample rate for JSONL streaming output, automatically clamped to **1–400** Hz.

---

## 10. Serial Protocol Commands

All Bridge classes automatically process serial input commands via the `tick()` method in `loop()`. Below is the supported command set (compatible with `NiusRobotLab_INA_monitor`).

### Common Commands (Supported by All Bridges)

| Command | Description | Example |
|---------|-------------|---------|
| `PING` | Connectivity check, returns a response | `PING` |
| `START` | Start JSONL streaming output | `START` |
| `STOP` | Stop JSONL streaming output | `STOP` |
| `SR <Hz>` | Set sample rate (Hz) | `SR 100` |

### Calibration Commands (InaBridge219 / 226 / 228 / 229Spi)

| Command | Description | Example |
|---------|-------------|---------|
| `IMAX <A>` | Set maximum expected current (A) | `IMAX 3.2` |
| `RSHUNT <ohm>` | Set shunt resistance (Ω) | `RSHUNT 0.01` |
| `DIAG` | Print diagnostic register information | `DIAG` |

### Alert Commands — INA219 / INA226

| Command | Description | Example |
|---------|-------------|---------|
| `ALERT OFF` | Disable alert | `ALERT OFF` |
| `ALERT CNVR` | Enable conversion-ready alert | `ALERT CNVR` |
| `ALERT BOV <V>` | Bus over-voltage alert | `ALERT BOV 5.5` |
| `ALERT BUV <V>` | Bus under-voltage alert | `ALERT BUV 3.0` |
| `ALERT SOV <µV>` | Shunt over-voltage alert | `ALERT SOV 80000` |
| `... LATCH 0/1` | Latch mode (0=transparent, 1=latched) | `ALERT BOV 5.5 LATCH 1` |
| `... POL 0/1` | Alert polarity (0=active-low, 1=active-high) | `ALERT BOV 5.5 POL 1` |

### Alert Commands — INA228

In addition to the commands above, the INA228 family also supports:

| Command | Description | Example |
|---------|-------------|---------|
| `ALERT BOV <V>` | Bus over-voltage (dedicated threshold register BOVL) | `ALERT BOV 5.5` |
| `ALERT BUV <V>` | Bus under-voltage (BUVL) | `ALERT BUV 3.0` |
| `ALERT SOV <µV>` | Shunt over-voltage (SOVL) | `ALERT SOV 80000` |

### Alert Commands — INA229 SPI

| Command | Description | Example |
|---------|-------------|---------|
| `ALERT OFF` | Disable alert | `ALERT OFF` |
| `ALERT CNVR` | Enable conversion-ready alert | `ALERT CNVR` |

### Alert Commands — INA3221

| Command | Description | Example |
|---------|-------------|---------|
| `CRIT <ch> <A>` | Set channel critical over-current threshold | `CRIT 1 2.0` |
| `WARN <ch> <A>` | Set channel warning over-current threshold | `WARN 1 1.0` |
| `PV <Vmin> <Vmax>` | Set power-valid voltage window | `PV 4.5 5.5` |
| `TC` | Read Timing Control flags | `TC` |
| `DIAG` | Print Mask/Enable + PV limits | `DIAG` |

### InaBridgeCh1 Commands

Supports only: `PING`, `START`, `STOP`, `SR <Hz>`, `RSHUNT <ohm>`.

### InaBridgeUnknown Commands

Supports only: `PING`, `START`, `STOP`, `SR <Hz>`.

---

> **Copyright**　NiusRobotLab / dunknowcoding — [GitHub](https://github.com/dunknowcoding/INA_series_sensor)
