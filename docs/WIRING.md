# INA Series Sensor — Wiring Reference

This document describes **power**, **logic levels**, **I²C** and **SPI** connections, **address selection**, and **example pin mappings** for common development boards. Always follow the **Texas Instruments datasheet** for your specific INA part (absolute maximum ratings, shunt placement, filtering, and layout).

---

## Table of contents

1. [General rules](#1-general-rules)
2. [Power and ground](#2-power-and-ground)
3. [Logic levels (3V3 vs 5V)](#3-logic-levels-3v3-vs-5v)
4. [I²C wiring](#4-i2c-wiring)
5. [SPI wiring (INA229)](#5-spi-wiring-ina229)
6. [Board-specific pin tables](#6-board-specific-pin-tables)
7. [Alternate wiring scenarios](#7-alternate-wiring-scenarios)
8. [INA3221 three-channel notes](#8-ina3221-three-channel-notes)
9. [CH1-only parts (InaBridgeCh1)](#9-ch1-only-parts-inabridgech1)
10. [Checklist before first power-on](#10-checklist-before-first-power-on)

---

## 1. General rules

- **INA supply (`V+` / `Vs`):** Use the voltage your part allows (many evaluation setups use **3.3 V** for logic-related rails; high-voltage bus sensing is separate — see the datasheet).
- **Common ground:** MCU **GND**, INA **GND**, and your **load return** must share a consistent **reference** per your measurement topology (high-side vs low-side shunt).
- **I²C pull-ups:** **SDA** and **SCL** need **pull-up resistors** (typically **2.2 kΩ–10 kΩ** to **3.3 V** on short runs). Many MCU boards include weak pull-ups; add external resistors if the bus is noisy or long.
- **Decoupling:** Place a **0.1 µF** (and often **10 µF**) ceramic near the INA **V+** pin as recommended by TI.

---

## 2. Power and ground

| Connection | Notes |
|------------|--------|
| **MCU ↔ INA** | Tie **GND** together. |
| **VCC to INA** | Per datasheet pin name (e.g. **3.3 V** digital supply for logic and internal regulator where applicable). |
| **Bus voltage** | **IN+** / **IN−** (or **VBUS** / shunt nodes) connect to the **circuit under test** — respect **common-mode range** and **maximum** **V+**/**V−** in the datasheet. |

**Never** exceed **absolute maximum** voltages on any pin.

---

## 3. Logic levels (3V3 vs 5V)

- Most **ESP32** and **RP2040** I/O is **3.3 V** tolerant; **I²C** and **SPI** in this library are used at **3.3 V** in typical examples.
- **5 V Arduino** (Uno/Nano **AVR**): **I²C** on **A4/A5** is **5 V** on a 5 V board. **Do not** connect a **3.3 V-only** INA or bus directly without **level shifting** or a **5 V-tolerant** configuration per datasheet.
- If your INA runs at **3.3 V** and the MCU is **5 V**, use a dedicated **I²C level shifter** (or **SPI** level shifter for INA229) on **SDA**, **SCL**, and (for SPI) **MOSI**, **MISO**, **SCK**, **CS** as required.

---

## 4. I²C wiring

### Signals

| Signal | Function |
|--------|----------|
| **SDA** | Data (open-drain, pull-up) |
| **SCL** | Clock (open-drain or push-pull per device, pull-up) |
| **GND** | Common reference |

### Address pins (7-bit)

Default sketches assume **0x40**. Many INA devices use **A0** / **A1** (or similar) strapping:

| Address (7-bit) | Typical strapping (example pattern — confirm on your part) |
|-----------------|------------------------------------------------------------|
| **0x40** | Default / all low |
| **0x41**–**0x4F** | See TI pinout for your package |

The constructor **`i2cAddr`** must match the **physical** address (e.g. **`0x40`**, **`0x41`**).

### Clock speed

Examples call **`beginI2c(sda, scl, 400000)`** — **400 kHz** **Fast-mode**. If the bus is long or noisy, lower the clock in code (e.g. **100 kHz**) by changing the third argument.

---

## 5. SPI wiring (INA229)

**`InaBridge229Spi`** uses **SPI mode** and **software CS** (GPIO driven high/low around transactions).

| MCU pin role | Connect to INA229 |
|--------------|---------------------|
| **SCK** | Serial clock |
| **MOSI** | Master out → slave in |
| **MISO** | Master in ← slave out |
| **CS** | Chip select (active low during access — firmware drives it) |
| **GND** | Common ground |
| **3V3** | INA supply per datasheet |

Default example pins (**ESP32-C3** style): **CS=7**, **SCK=4**, **MISO=5**, **MOSI=6** — change the constructor to match **your** PCB.

**Library behavior:**

- **ESP32 (Arduino-ESP32):** **`SPI.begin(SCK, MISO, MOSI, CS)`** via **`InaSpiBeginMapped`**.
- **RP2040 (Arduino-Pico):** **`SPI.setRX` / `setTX` / `setSCK`** then **`SPI.begin(false)`** (software CS).
- **AVR / others:** **`SPI.begin()`** uses **hardware SPI** pins; set **CS** in the constructor to the GPIO you wired to **CS**.

---

## 6. Board-specific pin tables

The following tables give **common** pin choices. **Your** board silkscreen wins — always verify the **pinout diagram** for your exact module.

### ESP32-C3 (e.g. SuperMini-style modules)

Used in many bundled examples.

| Role | GPIO (example) |
|------|----------------|
| **I²C SDA** | **8** |
| **I²C SCL** | **9** |
| **SPI CS** | **7** |
| **SPI SCK** | **4** |
| **SPI MISO** | **5** |
| **SPI MOSI** | **6** |

### ESP32 (classic, e.g. DevKitC)

I²C is often routed to:

| Role | GPIO (common — **not universal**) |
|------|-------------------------------------|
| **SDA** | **21** |
| **SCL** | **22** |

Some boards use **GPIO 18/19** or others — set **`beginI2c(sda, scl)`** accordingly.

### Raspberry Pi Pico (RP2040, Arduino-Pico core)

I²C **GPIO numbers** (not physical pin numbers on the PCB header):

| Role | Typical GPIO (example) |
|------|-------------------------|
| **I²C0 SDA** | **8** (GP8) |
| **I²C0 SCL** | **9** (GP9) |

Cross-check the **pinout** of your carrier: **GP8** might map to a specific header pin.

SPI: assign **free** GPIOs consistent with your **SPI** peripheral and pass them into **`InaBridge229Spi(..., cs, sck, miso, mosi)`**.

### Arduino Uno / Nano (AVR, 5V)

| Role | Pin |
|------|-----|
| **I²C SDA** | **A4** |
| **I²C SCL** | **A5** |

The library’s **`InaWireBeginMapped()`** calls **`Wire.begin()`** without pin numbers on AVR — **you must wire SDA/SCL to A4/A5**. You can still write **`beginI2c(A4, A5)`** in the sketch for documentation; pins may be ignored by the core.

**SPI (hardware):**

| Role | Uno/Nano |
|------|----------|
| **SCK** | **13** |
| **MISO** | **12** |
| **MOSI** | **11** |
| **CS** | User **GPIO** (e.g. **10**) — pass as **first** pin argument after **`chip`** and **`ref`** in **`InaBridge229Spi`**. |

**Level shifting:** If the INA is **3.3 V** only, do not drive it with **5 V** SPI without translation.

### Arduino Nano ESP32 / other hybrid boards

Treat as **ESP32** for **GPIO numbering** — use the board documentation; **`beginI2c`** uses the **ESP32**-style **`Wire.begin(SDA, SCL)`** path.

---

## 7. Alternate wiring scenarios

### Remapping I²C in software (ESP32 / RP2040)

Change only the **`beginI2c`** call:

```cpp
g_bridge.beginI2c(21, 22);   // example: ESP32 DevKit-style SDA/SCL
```

No library source edit required.

### Multiple INA devices on one I²C bus

- Use **different 7-bit addresses** (strap **A0**/**A1** pins).
- Instantiate **two bridge objects** with **different** addresses and **chip** strings if you extend the sketch (stock examples use **one** device).

### Long I²C cable

- Lower **clock** (e.g. **100 kHz**).
- Stronger **pull-ups** (within I²C spec).
- Optional **I²C buffer** / **repeater** IC for very long runs.

### Using an external USB–serial adapter (no USB MCU)

- Any MCU that exposes **UART** at **3.3 V** TTL can stream the same JSON Lines if you map **`Serial`** to that UART in your core — the stock examples assume **`Serial`** is the USB console.

---

## 8. INA3221 three-channel notes

- **Three** shunt / bus channel pairs — wiring must match **CH1**, **CH2**, **CH3** inputs on the IC.
- **`RSHUNT`** in firmware applies as a **single** value for **all** channels in the example math (same resistor value on all channels is assumed). If your shunts differ, adapt the sketch or post-process in software.

---

## 9. CH1-only parts (InaBridgeCh1)

Parts like **INA2227**, **INA4230**, **INA4235** may expose **multiple** channels in hardware; this library’s examples use **channel 1** registers only. Wire **one** sense path to the **CH1** inputs per the datasheet and set **`RSHUNT`** to that shunt.

---

## 10. Checklist before first power-on

- [ ] **Supply** voltage and **pinout** match the **TI datasheet** for your package.
- [ ] **GND** common between MCU and INA (and consistent with measurement topology).
- [ ] **I²C**/**SPI** pins match **`beginI2c`** / **`InaBridge229Spi`** arguments.
- [ ] **Pull-ups** on **SDA**/**SCL** for I²C.
- [ ] **115200** baud on host for **`Serial`**.
- [ ] **Chip name string** in code matches **INA Monitor** selection.

For **API** and **serial commands**, see **[USAGE.md](./USAGE.md)**.
