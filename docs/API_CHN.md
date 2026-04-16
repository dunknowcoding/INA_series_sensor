# INA Series Sensor — API 参考手册

> **库版本说明**　本文档覆盖 `INA_Series_Sensor` Arduino 库的全部 Bridge 层公开 API。
>
> **支持芯片**　INA219 · INA220 · INA226 · INA228 · INA229(SPI) · INA230–239 · INA740X · INA2227 · INA3221 · INA4230 · INA4235
>
> **头文件**　`#include <INA_Series_Sensor.h>`　— 一次包含即可使用所有 Bridge 类。

---

## 目录

- [1. 概述](#1-概述)
- [2. 类总览与功能矩阵](#2-类总览与功能矩阵)
- [3. InaBridge228 — INA228/237/238/239/740X (I²C)](#3-inabridge228)
- [4. InaBridge219 — INA219/INA220 (I²C)](#4-inabridge219)
- [5. InaBridge226 — INA226/INA230–236 (I²C)](#5-inabridge226)
- [6. InaBridge229Spi — INA229/INA229-Q1 (SPI)](#6-inabridge229spi)
- [7. InaBridge3221 — INA3221/INA3221-Q1 (三通道 I²C)](#7-inabridge3221)
- [8. InaBridgeCh1 — INA2227/INA4230/INA4235 (仅通道 1)](#8-inabridgech1)
- [9. InaBridgeUnknown — 无传感器占位](#9-inabridgeunknown)
- [10. 串口协议命令](#10-串口协议命令)

---

## 1. 概述

本库为 TI INA 系列电流/电压/功率监测芯片提供统一的 Arduino 驱动，支持两种使用模式：

| 模式 | 说明 |
|------|------|
| **模式 A — JSONL 流式输出** | 在 `loop()` 中调用 `tick()`，上位机通过串口发送 `START`/`STOP`/`SR` 等命令，Bridge 以 JSON Lines 格式输出测量数据。兼容 `NiusRobotLab_INA_monitor`。 |
| **模式 B — 独立直接读取** | 调用 `begin()` 初始化后，直接调用 `readBusVoltage()`、`readCurrent()` 等方法获取测量值，无串口输出。可选配合 `dataReady()` 轮询转换完成状态。 |

两种模式可在同一 sketch 中同时使用。

### 快速入门

```cpp
#include <INA_Series_Sensor.h>

static InaBridge228 sensor("INA228", 0x40);

void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.begin(8, 9);       // SDA=8, SCL=9
}

void loop() {
  sensor.tick();             // 模式 A: JSONL 流式输出

  // 模式 B: 独立读取
  if (sensor.dataReady()) {
    float v = sensor.readBusVoltage();
    float i = sensor.readCurrent();
    float p = sensor.readPower();
  }
}
```

---

## 2. 类总览与功能矩阵

| 功能 | InaBridge228 | InaBridge219 | InaBridge226 | InaBridge229Spi | InaBridge3221 | InaBridgeCh1 | InaBridgeUnknown |
|------|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| 总线 | I²C | I²C | I²C | SPI | I²C | I²C | — |
| readBusVoltage | ✔ | ✔ | ✔ | ✔ | ✔ (多通道) | ✔ | ✘ |
| readShuntVoltage | ✔ | ✔ | ✔ | ✔ | ✔ (多通道) | ✔ | ✘ |
| readCurrent | ✔ | ✔ | ✔ | ✔ | ✔ (多通道) | ✔ | ✘ |
| readPower | ✔ | ✔ | ✔ | ✔ | ✔ (多通道) | ✔ | ✘ |
| readDieTemp | ✔ | ✘ | ✘ | ✔ | ✘ | ✘ | ✘ |
| readEnergy / readCharge | ✔ | ✘ | ✘ | ✔ | ✘ | ✘ | ✘ |
| resetAccumulators | ✔ | ✘ | ✘ | ✔ | ✘ | ✘ | ✘ |
| dataReady | ✔ | ✔ | ✔ | ✔ | ✔ | ✘ | ✘ |
| setImax | ✔ | ✔ | ✔ | ✔ | ✘ | ✘ | ✘ |
| setRshunt | ✔ | ✔ | ✔ | ✔ | ✔ (多通道) | ✔ | ✘ |
| setExtraFieldsPrinter | ✔ | ✘ | ✘ | ✔ | ✔ | ✘ | ✘ |
| enableChannel | ✘ | ✘ | ✘ | ✘ | ✔ | ✘ | ✘ |
| 最大采样率 (Hz) | 400 | — | — | 2000 | — | 400 | 400 |

---

## 3. InaBridge228

**支持芯片**　INA228 · INA228-Q1 · INA237 · INA237-Q1 · INA238 · INA238-Q1 · INA239 · INA239-Q1 · INA740X

**头文件**　`InaBridge228.h`

### 构造函数

```cpp
InaBridge228(const char* chipJson, uint8_t i2cAddr = 0x40, const char* ref = "TI INA228");
```

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `chipJson` | `const char*` | — | 芯片名称字符串，用于 JSONL 输出中的 `"chip"` 字段，如 `"INA228"` |
| `i2cAddr` | `uint8_t` | `0x40` | I²C 从设备地址 (7-bit)，范围 0x40–0x4F |
| `ref` | `const char*` | `"TI INA228"` | 参考手册标识，用于 INFO 输出 |

**示例**

```cpp
static InaBridge228 sensor("INA228", 0x40);
static InaBridge228 sensor2("INA237", 0x45, "TI INA237");
```

---

### begin

```cpp
void begin(int pinSda = 8, int pinScl = 9, uint32_t i2cHz = 400000);
```

初始化 I²C 总线、探测芯片、复位、配置连续转换模式（温度 + 总线电压 + 分流电压）、执行校准，并在串口打印 INFO 行。

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `pinSda` | `int` | `8` | SDA 引脚（ESP32：GPIO 重映射；其他平台：忽略，使用板载默认） |
| `pinScl` | `int` | `9` | SCL 引脚（ESP32：GPIO 重映射；其他平台：忽略，使用板载默认） |
| `i2cHz` | `uint32_t` | `400000` | I²C 时钟频率 (Hz)，默认 400 kHz |

**返回值**　无

**示例**

```cpp
void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.begin(8, 9);          // ESP32-C3: SDA=GPIO8, SCL=GPIO9
  sensor.begin();              // 使用所有默认值
  sensor.begin(21, 22, 100000); // ESP32: SDA=21, SCL=22, 100 kHz
}
```

> **注意**　必须在调用任何测量方法之前调用 `begin()`。

---

### beginI2c

```cpp
void beginI2c(int pinSda = 8, int pinScl = 9, uint32_t i2cHz = 400000);
```

`begin()` 的旧版别名，功能完全相同。建议新代码使用 `begin()`。

---

### tick

```cpp
void tick();
```

处理串口命令并在流式模式下发送 JSONL 数据。**必须在 `loop()` 中每次迭代调用一次。**

**返回值**　无

**示例**

```cpp
void loop() {
  sensor.tick();
}
```

> **注意**　即使不使用 JSONL 流式输出，调用 `tick()` 也是安全的（它会静默处理串口输入）。

---

### readBusVoltage

```cpp
float readBusVoltage();
```

读取总线电压。

**返回值**　`float` — 总线电压，单位为伏特 (V)。

**寄存器精度**　24 位，LSB = 195.3125 µV。

**示例**

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

读取分流电压。

**返回值**　`float` — 分流电压，单位为伏特 (V)。

**寄存器精度**　24 位。LSB 取决于 ADCRANGE 配置位：
- ADCRANGE = 0（默认，±163.84 mV）：LSB = 312.5 nV
- ADCRANGE = 1（±40.96 mV）：LSB = 78.125 nV

**示例**

```cpp
float shuntV = sensor.readShuntVoltage();
Serial.print("Shunt: ");
Serial.print(shuntV * 1e6, 1);  // 转换为 µV 显示
Serial.println(" uV");
```

---

### readCurrent

```cpp
float readCurrent();
```

读取电流值。需要有效校准（已调用 `begin()` 或手动设置过 `setRshunt` + `setImax`）。

**返回值**　`float` — 电流，单位为安培 (A)。

**示例**

```cpp
float current = sensor.readCurrent();
Serial.print("Current: ");
Serial.print(current, 4);
Serial.println(" A");
```

> **注意**　如果未正确校准，返回值将不准确。

---

### readPower

```cpp
float readPower();
```

读取功率值。

**返回值**　`float` — 功率，单位为瓦特 (W)。

**示例**

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

读取芯片内部（管芯）温度。

**返回值**　`float` — 温度，单位为摄氏度 (°C)。

**寄存器精度**　LSB = 7.8125 m°C。

**示例**

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

读取自上次复位以来的累积能量。

**返回值**　`float` — 累积能量，单位为焦耳 (J)。

**示例**

```cpp
float energy = sensor.readEnergy();
Serial.print("Energy: ");
Serial.print(energy, 6);
Serial.println(" J");
```

> **注意**　需要先调用 `resetAccumulators()` 清零，才能获得有意义的测量值。

---

### readCharge

```cpp
float readCharge();
```

读取自上次复位以来的累积电荷（有符号值，支持双向电流）。

**返回值**　`float` — 累积电荷，单位为库仑 (C)。

**示例**

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

将能量和电荷累积寄存器清零。

**返回值**　无

**示例**

```cpp
sensor.resetAccumulators();
// 从此刻开始计量能量和电荷
```

---

### dataReady

```cpp
bool dataReady();
```

检查 DIAG_ALRT 寄存器中的 CNVRF（转换就绪）标志位。

**返回值**　`bool` — `true` 表示有新的转换数据可读。

**示例**

```cpp
if (sensor.dataReady()) {
  float v = sensor.readBusVoltage();
  float i = sensor.readCurrent();
}
```

> **⚠ 重要**　读取 DIAG_ALRT 寄存器会**清除** CNVRF 标志。因此连续调用两次 `dataReady()` 时，第二次将返回 `false`。
>
> 当同时使用模式 A（流式输出）和模式 B（直接读取）时，`dataReady()` 可能在流式采样间隔内返回 `false`，因为流式代码也会读取同一寄存器并清除标志。

---

### startStreaming

```cpp
void startStreaming();
```

启动 JSONL 流式输出。等效于通过串口发送 `START` 命令。

**返回值**　无

---

### stopStreaming

```cpp
void stopStreaming();
```

停止 JSONL 流式输出。等效于通过串口发送 `STOP` 命令。

**返回值**　无

---

### isStreaming

```cpp
bool isStreaming() const;
```

**返回值**　`bool` — `true` 表示 JSONL 流式输出正在进行。

---

### setSampleRate

```cpp
void setSampleRate(int hz);
```

设置 JSONL 流式输出的采样率。

| 参数 | 类型 | 说明 |
|------|------|------|
| `hz` | `int` | 采样率 (Hz)，自动钳位到 **1–400** |

**返回值**　无

**示例**

```cpp
sensor.setSampleRate(100);  // 100 Hz
```

---

### setRshunt

```cpp
void setRshunt(float ohm);
```

设置分流电阻值并重新校准。

| 参数 | 类型 | 说明 |
|------|------|------|
| `ohm` | `float` | 分流电阻值（Ω），最小 0.0001 Ω |

**返回值**　无

**示例**

```cpp
sensor.setRshunt(0.01);  // 10 mΩ 分流电阻
```

---

### setImax

```cpp
void setImax(float ampere);
```

设置最大期望电流并重新校准。此值影响 `current_LSB` 的计算。

| 参数 | 类型 | 说明 |
|------|------|------|
| `ampere` | `float` | 最大期望电流 (A)，必须 > 0 |

**返回值**　无

**示例**

```cpp
sensor.setImax(20.0);  // 最大 20 A
```

---

### rshunt

```cpp
float rshunt() const;
```

**返回值**　`float` — 当前分流电阻值 (Ω)。默认 0.1 Ω。

---

### imax

```cpp
float imax() const;
```

**返回值**　`float` — 当前最大期望电流 (A)。默认 10.0 A。

---

### currentLsb

```cpp
float currentLsb() const;
```

**返回值**　`float` — 当前校准后的电流 LSB 值 (A/LSB)。

---

### address

```cpp
uint8_t address() const;
```

**返回值**　`uint8_t` — I²C 从设备地址 (7-bit)。

---

### setExtraFieldsPrinter

```cpp
typedef void (*ExtraFieldsFn)();
void setExtraFieldsPrinter(ExtraFieldsFn fn);
```

注册回调函数，在每条 JSONL 测量数据行关闭花括号之前被调用。可用于向 JSONL 输出注入额外字段。

| 参数 | 类型 | 说明 |
|------|------|------|
| `fn` | `ExtraFieldsFn` | 回调函数指针，签名为 `void (*)()` |

**返回值**　无

**示例**

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

> **注意**　回调函数必须直接向 `Serial` 输出，以逗号开头，写入合法的 JSON 键值对。`NiusRobotLab_INA_monitor` 会忽略不认识的额外字段。

---

## 4. InaBridge219

**支持芯片**　INA219 · INA220 · INA220-Q1

**头文件**　`InaBridge219.h`

### 构造函数

```cpp
InaBridge219(const char* chipJson, uint8_t i2cAddr = 0x40);
```

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `chipJson` | `const char*` | — | 芯片名称字符串，如 `"INA219"` |
| `i2cAddr` | `uint8_t` | `0x40` | I²C 从设备地址 (7-bit) |

**示例**

```cpp
static InaBridge219 sensor("INA219", 0x40);
```

---

### begin

```cpp
void begin(int pinSda = 8, int pinScl = 9, uint32_t i2cHz = 400000);
```

初始化 I²C 总线、配置 ADC、执行校准，并在串口打印 INFO 行。参数与 [InaBridge228::begin](#begin) 相同。

---

### beginI2c

```cpp
void beginI2c(int pinSda = 8, int pinScl = 9, uint32_t i2cHz = 400000);
```

`begin()` 的旧版别名。

---

### tick

```cpp
void tick();
```

处理串口命令并在流式模式下发送 JSONL 数据。**必须在 `loop()` 中每次迭代调用一次。**

---

### readBusVoltage

```cpp
float readBusVoltage();
```

读取总线电压。

**返回值**　`float` — 总线电压 (V)。

**寄存器精度**　16 位，LSB = 4 mV。

---

### readShuntVoltage

```cpp
float readShuntVoltage();
```

读取分流电压。

**返回值**　`float` — 分流电压 (V)。

**寄存器精度**　16 位有符号，LSB = 10 µV。

---

### readCurrent

```cpp
float readCurrent();
```

读取电流值。需要有效校准。

**返回值**　`float` — 电流 (A)。

**校准公式**　`current_LSB = imax / 32768`

---

### readPower

```cpp
float readPower();
```

读取功率值。

**返回值**　`float` — 功率 (W)。

**校准公式**　`power_LSB = 20 × current_LSB`

---

### dataReady

```cpp
bool dataReady();
```

检查 Bus Voltage 寄存器中的 CNVR（转换就绪）标志位（bit 1）。

**返回值**　`bool` — `true` 表示有新的转换数据可读。

> **⚠ 重要**　CNVR 标志不会被读取 Bus Voltage 寄存器本身清除；**读取 Power 寄存器会清除该标志**。因此 `readPower()` 调用后 `dataReady()` 将返回 `false`。

---

### startStreaming / stopStreaming / isStreaming

```cpp
void startStreaming();
void stopStreaming();
bool isStreaming() const;
```

功能与 [InaBridge228](#startstreaming) 对应方法相同。

---

### setSampleRate

```cpp
void setSampleRate(int hz);
```

设置 JSONL 流式输出的采样率，自动钳位到有效范围。

---

### setRshunt

```cpp
void setRshunt(float ohm);
```

设置分流电阻值并重新校准。参数说明同 [InaBridge228::setRshunt](#setrshunt)。

---

### setImax

```cpp
void setImax(float ampere);
```

设置最大期望电流并重新校准。参数说明同 [InaBridge228::setImax](#setimax)。

**默认值**　3.2 A（INA219 的满量程默认值）。

---

### rshunt / imax / address

```cpp
float rshunt() const;    // 默认 0.1 Ω
float imax() const;      // 默认 3.2 A
uint8_t address() const;
```

获取当前配置值，含义与 InaBridge228 对应方法相同。

---

## 5. InaBridge226

**支持芯片**　INA226 · INA226-Q1 · INA230 · INA231 · INA232 · INA233 · INA234 · INA235 · INA236

**头文件**　`InaBridge226.h`

### 构造函数

```cpp
InaBridge226(const char* chipJson, uint8_t i2cAddr = 0x40, const char* ref = "TI SLYSF02");
```

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `chipJson` | `const char*` | — | 芯片名称字符串，如 `"INA226"` |
| `i2cAddr` | `uint8_t` | `0x40` | I²C 从设备地址 (7-bit) |
| `ref` | `const char*` | `"TI SLYSF02"` | 参考手册标识 |

**示例**

```cpp
static InaBridge226 sensor("INA226", 0x40);
static InaBridge226 sensor2("INA234", 0x44, "TI INA234");
```

---

### begin

```cpp
void begin(int pinSda = 8, int pinScl = 9, uint32_t i2cHz = 400000);
```

初始化 I²C 总线、配置 ADC、执行校准，并在串口打印 INFO 行。参数同 [InaBridge228::begin](#begin)。

---

### beginI2c

```cpp
void beginI2c(int pinSda = 8, int pinScl = 9, uint32_t i2cHz = 400000);
```

`begin()` 的旧版别名。

---

### tick

```cpp
void tick();
```

处理串口命令并在流式模式下发送 JSONL 数据。**必须在 `loop()` 中每次迭代调用一次。**

---

### readBusVoltage

```cpp
float readBusVoltage();
```

读取总线电压。

**返回值**　`float` — 总线电压 (V)。

**寄存器精度**　16 位，LSB = 1.25 mV。

---

### readShuntVoltage

```cpp
float readShuntVoltage();
```

读取分流电压。

**返回值**　`float` — 分流电压 (V)。

**寄存器精度**　16 位有符号，LSB = 2.5 µV。

---

### readCurrent

```cpp
float readCurrent();
```

读取电流值。需要有效校准。

**返回值**　`float` — 电流 (A)。

**校准公式**　`current_LSB = imax / 32768`

---

### readPower

```cpp
float readPower();
```

读取功率值。

**返回值**　`float` — 功率 (W)。

**校准公式**　`power_LSB = 25 × current_LSB`

---

### dataReady

```cpp
bool dataReady();
```

检查 Mask/Enable 寄存器（0x06）中的 CVRF（转换就绪）标志位（bit 3）。

**返回值**　`bool` — `true` 表示有新的转换数据可读。

> **⚠ 重要**　读取 Mask/Enable 寄存器会**清除** CVRF 标志。连续调用两次 `dataReady()` 时，第二次将返回 `false`。

---

### startStreaming / stopStreaming / isStreaming

```cpp
void startStreaming();
void stopStreaming();
bool isStreaming() const;
```

功能与 [InaBridge228](#startstreaming) 对应方法相同。

---

### setSampleRate

```cpp
void setSampleRate(int hz);
```

设置 JSONL 流式输出的采样率，自动钳位到有效范围。

---

### setRshunt

```cpp
void setRshunt(float ohm);
```

设置分流电阻值并重新校准。参数说明同 [InaBridge228::setRshunt](#setrshunt)。

---

### setImax

```cpp
void setImax(float ampere);
```

设置最大期望电流并重新校准。

**默认值**　3.2 A。

---

### rshunt / imax / address

```cpp
float rshunt() const;    // 默认 0.1 Ω
float imax() const;      // 默认 3.2 A
uint8_t address() const;
```

获取当前配置值。

---

## 6. InaBridge229Spi

**支持芯片**　INA229 · INA229-Q1

**头文件**　`InaBridge229Spi.h`

**总线**　SPI（非 I²C）

> INA229 的测量数学与 INA228 完全相同，仅通信总线不同。

### 构造函数

```cpp
InaBridge229Spi(const char* chipJson, const char* ref,
                int pinCs = 7, int pinSck = 4, int pinMiso = 5, int pinMosi = 6);
```

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `chipJson` | `const char*` | — | 芯片名称字符串，如 `"INA229"` |
| `ref` | `const char*` | — | 参考手册标识，如 `"TI INA229"` |
| `pinCs` | `int` | `7` | SPI 片选 (CS) 引脚 |
| `pinSck` | `int` | `4` | SPI 时钟 (SCK) 引脚 |
| `pinMiso` | `int` | `5` | SPI MISO 引脚 |
| `pinMosi` | `int` | `6` | SPI MOSI 引脚 |

**示例**

```cpp
static InaBridge229Spi sensor("INA229", "TI INA229");
static InaBridge229Spi sensor2("INA229-Q1", "TI INA229-Q1", 10, 13, 12, 11);
```

---

### beginSpi

```cpp
void beginSpi(uint32_t spiHz = 10000000);
```

初始化 SPI 总线、探测芯片、复位、配置 ADC 模式、执行校准，并在串口打印 INFO 行。

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `spiHz` | `uint32_t` | `10000000` | SPI 时钟频率 (Hz)，默认 10 MHz |

**返回值**　无

**示例**

```cpp
void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.beginSpi();             // 默认 10 MHz
  sensor.beginSpi(1000000);      // 1 MHz
}
```

---

### begin

```cpp
void begin(uint32_t spiHz = 10000000);
```

`beginSpi()` 的别名，功能完全相同。

---

### tick

```cpp
void tick();
```

处理串口命令并在流式模式下发送 JSONL 数据。使用 `micros()` 计时以支持更高采样率。**必须在 `loop()` 中每次迭代调用一次。**

---

### readBusVoltage

```cpp
float readBusVoltage();
```

读取总线电压。

**返回值**　`float` — 总线电压 (V)。

**寄存器精度**　24 位，LSB = 195.3125 µV（与 INA228 相同）。

---

### readShuntVoltage

```cpp
float readShuntVoltage();
```

读取分流电压。

**返回值**　`float` — 分流电压 (V)。

**寄存器精度**　24 位，LSB 取决于 ADCRANGE（与 INA228 相同）。

---

### readCurrent

```cpp
float readCurrent();
```

**返回值**　`float` — 电流 (A)。需要有效校准。

---

### readPower

```cpp
float readPower();
```

**返回值**　`float` — 功率 (W)。

---

### readDieTemp

```cpp
float readDieTemp();
```

**返回值**　`float` — 芯片管芯温度 (°C)。LSB = 7.8125 m°C。

---

### readEnergy

```cpp
float readEnergy();
```

**返回值**　`float` — 自上次复位以来的累积能量 (J)。

---

### readCharge

```cpp
float readCharge();
```

**返回值**　`float` — 自上次复位以来的累积电荷 (C)（有符号）。

---

### resetAccumulators

```cpp
void resetAccumulators();
```

将能量和电荷累积寄存器清零。

---

### dataReady

```cpp
bool dataReady();
```

检查 DIAG_ALRT 寄存器中的 CNVRF 标志位。

**返回值**　`bool` — `true` 表示有新的转换数据可读。

> **⚠ 重要**　读取会清除 CNVRF 标志。行为与 [InaBridge228::dataReady](#dataready) 相同。

---

### startStreaming / stopStreaming / isStreaming

```cpp
void startStreaming();
void stopStreaming();
bool isStreaming() const;
```

功能与 InaBridge228 对应方法相同。

---

### setSampleRate

```cpp
void setSampleRate(int hz);
```

设置 JSONL 流式输出的采样率。

| 参数 | 类型 | 说明 |
|------|------|------|
| `hz` | `int` | 采样率 (Hz)，自动钳位到 **1–2000**（SPI 比 I²C 更快） |

---

### setRshunt

```cpp
void setRshunt(float ohm);
```

设置分流电阻值并重新校准。最小 0.0001 Ω。

---

### setImax

```cpp
void setImax(float ampere);
```

设置最大期望电流并重新校准。必须 > 0。

---

### rshunt / imax / currentLsb

```cpp
float rshunt() const;     // 默认 0.1 Ω
float imax() const;       // 默认 10.0 A
float currentLsb() const;
```

获取当前配置值。

> **注意**　InaBridge229Spi **没有** `address()` 方法（SPI 使用片选引脚而非地址）。

---

### setExtraFieldsPrinter

```cpp
typedef void (*ExtraFieldsFn)();
void setExtraFieldsPrinter(ExtraFieldsFn fn);
```

注册回调函数，用于向 JSONL 输出注入额外字段。用法同 [InaBridge228::setExtraFieldsPrinter](#setextrafieldsprinter)。

---

## 7. InaBridge3221

**支持芯片**　INA3221 · INA3221-Q1

**头文件**　`InaBridge3221.h`

**特点**　三独立测量通道，每通道有独立总线电压和分流电压读取。电流通过软件计算 `shuntV / rshunt`。每通道可配置独立分流电阻，可单独启用或禁用。

### 构造函数

```cpp
InaBridge3221(const char* chipJson, uint8_t i2cAddr = 0x40);
```

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `chipJson` | `const char*` | — | 芯片名称字符串，如 `"INA3221"` |
| `i2cAddr` | `uint8_t` | `0x40` | I²C 从设备地址 (7-bit) |

**示例**

```cpp
static InaBridge3221 sensor("INA3221", 0x40);
```

---

### begin

```cpp
void begin(int pinSda = 8, int pinScl = 9, uint32_t i2cHz = 400000);
```

初始化 I²C 总线、探测设备、应用默认配置，并在串口打印 INFO 行。参数同 [InaBridge228::begin](#begin)。

---

### beginI2c

```cpp
void beginI2c(int pinSda = 8, int pinScl = 9, uint32_t i2cHz = 400000);
```

`begin()` 的旧版别名。

---

### tick

```cpp
void tick();
```

处理串口命令并在流式模式下发送 JSONL 数据。**必须在 `loop()` 中每次迭代调用一次。**

---

### readBusVoltage

```cpp
float readBusVoltage(uint8_t ch);
```

读取指定通道的总线电压。

| 参数 | 类型 | 说明 |
|------|------|------|
| `ch` | `uint8_t` | 通道号：**1**、**2** 或 **3**。超出范围返回 0 |

**返回值**　`float` — 总线电压 (V)。

**示例**

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

读取指定通道的分流电压。

| 参数 | 类型 | 说明 |
|------|------|------|
| `ch` | `uint8_t` | 通道号：**1**、**2** 或 **3** |

**返回值**　`float` — 分流电压 (V)。

---

### readCurrent

```cpp
float readCurrent(uint8_t ch);
```

读取指定通道的电流，通过软件计算 `shuntVoltage / rshunt(ch)`。

| 参数 | 类型 | 说明 |
|------|------|------|
| `ch` | `uint8_t` | 通道号：**1**、**2** 或 **3** |

**返回值**　`float` — 电流 (A)。

**示例**

```cpp
sensor.setRshunt(1, 0.1);   // CH1: 100 mΩ
sensor.setRshunt(2, 0.05);  // CH2:  50 mΩ
sensor.setRshunt(3, 0.01);  // CH3:  10 mΩ

float i1 = sensor.readCurrent(1);  // 使用 CH1 的 rshunt
float i2 = sensor.readCurrent(2);  // 使用 CH2 的 rshunt
```

---

### readPower

```cpp
float readPower(uint8_t ch);
```

读取指定通道的功率，通过软件计算 `current(ch) × busVoltage(ch)`。

| 参数 | 类型 | 说明 |
|------|------|------|
| `ch` | `uint8_t` | 通道号：**1**、**2** 或 **3** |

**返回值**　`float` — 功率 (W)。

---

### dataReady

```cpp
bool dataReady();
```

检查 Mask/Enable 寄存器中的 CVRF（转换就绪）标志位。

**返回值**　`bool` — `true` 表示有新的转换数据可读。

> **⚠ 重要**　读取 Mask/Enable 寄存器会清除 CVRF 标志。

---

### enableChannel

```cpp
void enableChannel(uint8_t ch, bool enable = true);
```

启用或禁用指定测量通道。禁用的通道在转换时被跳过，可缩短总转换时间，允许更高采样率。

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `ch` | `uint8_t` | — | 通道号：**1**、**2** 或 **3** |
| `enable` | `bool` | `true` | `true` 启用，`false` 禁用 |

**返回值**　无

**示例**

```cpp
sensor.enableChannel(3, false);  // 禁用通道 3
sensor.enableChannel(1);         // 启用通道 1（默认 true）
```

---

### isChannelEnabled

```cpp
bool isChannelEnabled(uint8_t ch);
```

检查指定通道是否启用。

| 参数 | 类型 | 说明 |
|------|------|------|
| `ch` | `uint8_t` | 通道号：**1**、**2** 或 **3** |

**返回值**　`bool` — `true` 表示通道已启用。

---

### startStreaming / stopStreaming / isStreaming

```cpp
void startStreaming();
void stopStreaming();
bool isStreaming() const;
```

功能与 InaBridge228 对应方法相同。

---

### setSampleRate

```cpp
void setSampleRate(int hz);
```

设置 JSONL 流式输出的采样率。

---

### setRshunt（单通道）

```cpp
void setRshunt(uint8_t ch, float ohm);
```

设置指定通道的分流电阻值。

| 参数 | 类型 | 说明 |
|------|------|------|
| `ch` | `uint8_t` | 通道号：**1**、**2** 或 **3** |
| `ohm` | `float` | 分流电阻值 (Ω)，必须 > 0 |

**返回值**　无

**示例**

```cpp
sensor.setRshunt(1, 0.1);   // CH1: 100 mΩ
sensor.setRshunt(2, 0.05);  // CH2:  50 mΩ
sensor.setRshunt(3, 0.01);  // CH3:  10 mΩ
```

---

### setRshunt（全通道）

```cpp
void setRshunt(float ohm);
```

将三个通道设置为相同的分流电阻值。

| 参数 | 类型 | 说明 |
|------|------|------|
| `ohm` | `float` | 分流电阻值 (Ω)，必须 > 0 |

**返回值**　无

**示例**

```cpp
sensor.setRshunt(0.1);  // 所有通道: 100 mΩ
```

---

### rshunt

```cpp
float rshunt(uint8_t ch = 1) const;
```

获取指定通道的分流电阻值。

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `ch` | `uint8_t` | `1` | 通道号：**1**、**2** 或 **3** |

**返回值**　`float` — 分流电阻值 (Ω)。默认 0.1 Ω。

---

### address

```cpp
uint8_t address() const;
```

**返回值**　`uint8_t` — I²C 从设备地址 (7-bit)。

---

### setExtraFieldsPrinter

```cpp
typedef void (*ExtraFieldsFn)();
void setExtraFieldsPrinter(ExtraFieldsFn fn);
```

注册回调函数，用于向 JSONL 输出注入额外字段。用法同 [InaBridge228::setExtraFieldsPrinter](#setextrafieldsprinter)。

> **注意**　InaBridge3221 **没有** `setImax()` 方法（INA3221 无校准寄存器，电流通过软件除法计算）。

---

## 8. InaBridgeCh1

**支持芯片**　INA2227 · INA4230 · INA4235

**头文件**　`InaBridgeCh1.h`

**特点**　仅读取通道 1 的简化驱动。分流电压 LSB = 40 µV，总线电压 LSB = 8 mV。电流通过软件除法计算 `shuntV / Rshunt`。

> **限制**　这些芯片没有校准寄存器，也没有文档记载的转换就绪标志，因此不提供 `dataReady()`、`setImax()`、`setExtraFieldsPrinter()` 方法。

### 构造函数

```cpp
InaBridgeCh1(const char* chipJson, const char* infoMsg, uint8_t i2cAddr = 0x40);
```

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `chipJson` | `const char*` | — | 芯片名称字符串，如 `"INA2227"` |
| `infoMsg` | `const char*` | — | INFO 行的附加消息文本 |
| `i2cAddr` | `uint8_t` | `0x40` | I²C 从设备地址 (7-bit) |

**示例**

```cpp
static InaBridgeCh1 sensor("INA4230", "CH1 only driver", 0x40);
```

---

### begin

```cpp
void begin(int pinSda = 8, int pinScl = 9, uint32_t i2cHz = 400000);
```

初始化 I²C 总线、探测芯片、应用默认配置，并在串口打印 INFO 行。参数同 [InaBridge228::begin](#begin)。

---

### beginI2c

```cpp
void beginI2c(int pinSda = 8, int pinScl = 9, uint32_t i2cHz = 400000);
```

`begin()` 的旧版别名。

---

### tick

```cpp
void tick();
```

处理串口命令并在流式模式下发送 JSONL 数据。**必须在 `loop()` 中每次迭代调用一次。**

---

### readBusVoltage

```cpp
float readBusVoltage();
```

读取总线电压。

**返回值**　`float` — 总线电压 (V)。

**寄存器精度**　LSB = 8 mV。

---

### readShuntVoltage

```cpp
float readShuntVoltage();
```

读取分流电压。

**返回值**　`float` — 分流电压 (V)。

**寄存器精度**　16 位有符号，LSB = 40 µV。

---

### readCurrent

```cpp
float readCurrent();
```

读取电流值。通过软件除法计算 `shuntV / Rshunt`。

**返回值**　`float` — 电流 (A)。

---

### readPower

```cpp
float readPower();
```

读取功率值。通过软件计算 `current × busVoltage`。

**返回值**　`float` — 功率 (W)。

---

### startStreaming / stopStreaming / isStreaming

```cpp
void startStreaming();
void stopStreaming();
bool isStreaming() const;
```

功能与 InaBridge228 对应方法相同。

---

### setSampleRate

```cpp
void setSampleRate(int hz);
```

设置 JSONL 流式输出的采样率，自动钳位到 **1–400** Hz。

---

### setRshunt

```cpp
void setRshunt(float ohm);
```

设置分流电阻值。

| 参数 | 类型 | 说明 |
|------|------|------|
| `ohm` | `float` | 分流电阻值 (Ω)，最小 0.0001 Ω；低于此值将被钳位到 0.1 Ω |

**返回值**　无

---

### rshunt / address

```cpp
float rshunt() const;     // 默认 0.1 Ω
uint8_t address() const;
```

获取当前配置值。

---

## 9. InaBridgeUnknown

**用途**　未检测到传感器时的占位 Bridge。始终输出零值。接受与其他 Bridge 相同的 `START`/`STOP`/`SR` 串口命令，使上位机无需特殊处理。

**头文件**　`InaBridgeUnknown.h`

### begin

```cpp
void begin();
```

在串口打印 INFO 行。无需参数（无硬件初始化）。

**返回值**　无

**示例**

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

处理串口命令并在流式模式下输出全零 JSONL 数据。**必须在 `loop()` 中每次迭代调用一次。**

---

### startStreaming / stopStreaming / isStreaming

```cpp
void startStreaming();
void stopStreaming();
bool isStreaming() const;
```

功能与 InaBridge228 对应方法相同。

---

### setSampleRate

```cpp
void setSampleRate(int hz);
```

设置 JSONL 流式输出的采样率，自动钳位到 **1–400** Hz。

---

## 10. 串口协议命令

所有 Bridge 类通过 `tick()` 方法在 `loop()` 中自动处理串口输入命令。以下为支持的命令集（兼容 `NiusRobotLab_INA_monitor`）。

### 通用命令（所有 Bridge 支持）

| 命令 | 说明 | 示例 |
|------|------|------|
| `PING` | 连接性检查，返回响应 | `PING` |
| `START` | 开始 JSONL 流式输出 | `START` |
| `STOP` | 停止 JSONL 流式输出 | `STOP` |
| `SR <Hz>` | 设置采样率（Hz） | `SR 100` |

### 校准命令（InaBridge219 / 226 / 228 / 229Spi）

| 命令 | 说明 | 示例 |
|------|------|------|
| `IMAX <A>` | 设置最大期望电流 (A) | `IMAX 3.2` |
| `RSHUNT <ohm>` | 设置分流电阻值 (Ω) | `RSHUNT 0.01` |
| `DIAG` | 打印诊断寄存器信息 | `DIAG` |

### 报警命令 — INA219 / INA226

| 命令 | 说明 | 示例 |
|------|------|------|
| `ALERT OFF` | 禁用报警 | `ALERT OFF` |
| `ALERT CNVR` | 启用转换就绪报警 | `ALERT CNVR` |
| `ALERT BOV <V>` | 总线过压报警 | `ALERT BOV 5.5` |
| `ALERT BUV <V>` | 总线欠压报警 | `ALERT BUV 3.0` |
| `ALERT SOV <µV>` | 分流过压报警 | `ALERT SOV 80000` |
| `... LATCH 0/1` | 锁存模式（0=透明，1=锁存） | `ALERT BOV 5.5 LATCH 1` |
| `... POL 0/1` | 报警极性（0=低有效，1=高有效） | `ALERT BOV 5.5 POL 1` |

### 报警命令 — INA228

除上述命令外，INA228 家族还支持：

| 命令 | 说明 | 示例 |
|------|------|------|
| `ALERT BOV <V>` | 总线过压（独立阈值寄存器 BOVL） | `ALERT BOV 5.5` |
| `ALERT BUV <V>` | 总线欠压（BUVL） | `ALERT BUV 3.0` |
| `ALERT SOV <µV>` | 分流过压（SOVL） | `ALERT SOV 80000` |

### 报警命令 — INA229 SPI

| 命令 | 说明 | 示例 |
|------|------|------|
| `ALERT OFF` | 禁用报警 | `ALERT OFF` |
| `ALERT CNVR` | 启用转换就绪报警 | `ALERT CNVR` |

### 报警命令 — INA3221

| 命令 | 说明 | 示例 |
|------|------|------|
| `CRIT <ch> <A>` | 设置通道临界过流阈值 | `CRIT 1 2.0` |
| `WARN <ch> <A>` | 设置通道警告过流阈值 | `WARN 1 1.0` |
| `PV <Vmin> <Vmax>` | 设置电源有效电压窗口 | `PV 4.5 5.5` |
| `TC` | 读取 Timing Control 标志 | `TC` |
| `DIAG` | 打印 Mask/Enable + PV 限值 | `DIAG` |

### InaBridgeCh1 命令

仅支持：`PING`、`START`、`STOP`、`SR <Hz>`、`RSHUNT <ohm>`。

### InaBridgeUnknown 命令

仅支持：`PING`、`START`、`STOP`、`SR <Hz>`。

---

> **版权**　NiusRobotLab / dunknowcoding — [GitHub](https://github.com/dunknowcoding/INA_series_sensor)
