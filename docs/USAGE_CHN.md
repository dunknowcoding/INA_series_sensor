# INA Series Sensor — 使用指南

> **适用对象：** 初学者与中级用户  
> **库版本：** 0.2.5+  
> **GitHub：** [dunknowcoding/INA_series_sensor](https://github.com/dunknowcoding/INA_series_sensor)

---

## 目录

1. [快速入门](#1-快速入门)
2. [接线指南](#2-接线指南)
3. [使用 NiusRobotLab_INA_monitor 工具](#3-使用-niusrobotlab_ina_monitor-工具)
4. [独立使用（不使用 Monitor 工具）](#4-独立使用不使用-monitor-工具)
5. [市面常见 INA 模块兼容性](#5-市面常见-ina-模块兼容性)
6. [注意事项与常见问题](#6-注意事项与常见问题)
7. [示例一览表](#7-示例一览表)

---

## 1. 快速入门

### 1.1 安装库

#### Arduino IDE

1. 从 GitHub 下载 `.zip` 文件  
2. 打开 Arduino IDE → **Sketch → Include Library → Add .ZIP Library**  
3. 选择下载的 `.zip` 文件，点击确认  
4. 安装完成后，**File → Examples** 菜单底部会出现 **INA Series Sensor**

#### PlatformIO

在 `platformio.ini` 中添加：

```ini
lib_deps = dunknowcoding/INA Series Sensor
```

执行 `pio lib install` 或直接编译，PlatformIO 会自动下载。

### 1.2 第一个程序：INA228 基础示例

将以下代码复制粘贴到 Arduino IDE，烧录到你的 ESP32-C3（或其他开发板）：

```cpp
#include <INA_Series_Sensor.h>

static InaBridge228 sensor("INA228", 0x40);

void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.begin(8, 9);   // SDA=GPIO8, SCL=GPIO9（ESP32-C3 默认）
}

void loop() {
  sensor.tick();
}
```

**说明：**
- `InaBridge228` 是 INA228 系列的桥接类，负责 JSONL 流式输出与独立读数
- `"INA228"` 是 JSONL 中 `chip` 字段的名称
- `0x40` 是 I²C 地址（绝大多数模块的默认地址）
- `begin(8, 9)` 初始化 I²C 总线（GPIO 编号仅在 ESP32 上有效，AVR 使用板载默认引脚）
- `tick()` 每次 `loop()` 调用一次，处理串口命令并在流式模式下发送测量数据

烧录后打开 **串口监视器**（波特率 115200），你会看到一行 INFO JSON。发送 `START` 开始采样，发送 `STOP` 停止。

> **不同芯片？** 只需替换 Bridge 类和芯片名称即可。例如 INA226 用 `InaBridge226`，INA3221 用 `InaBridge3221`。完整对应关系见 [示例一览表](#7-示例一览表)。

---

## 2. 接线指南

### 2.1 通用高侧电流检测原理

所有 INA 芯片都通过检测分流电阻（shunt resistor）两端的微小电压差来计算电流。典型的高侧接线方式：

```
                          分流电阻 (Rshunt)
                        ┌──────┤├──────┐
 [电源 +] ──────────── VS+ (IN+)    VS- (IN-) ──────────── [负载 +]
                                                              │
                                                           [负载 -]
                                                              │
 [电源 GND] ◄─────────────────────────────────────────────────┘
```

**关键规则：**
- 分流电阻必须串联在电源与负载之间
- VS+（IN+）接电源侧，VS-（IN-）接负载侧
- **不要**同时接高侧和低侧 —— 选一个

---

### 2.2 INA219 / INA220（I²C）

**电气参数：** VCC 3–5.5V，总线电压最大 26V

```
 [电源 +] ───→ [VS+ / IN+] ──┤Rshunt├── [VS- / IN-] ───→ [负载 +] ───→ [负载 -] ───→ [电源 GND]
                    │                          │
                   VCC ──── 3.3V 或 5V        GND
                    │                          │
                   SDA ←── 4.7kΩ 上拉 ──→ MCU SDA (GPIO 8)
                   SCL ←── 4.7kΩ 上拉 ──→ MCU SCL (GPIO 9)
                    │
                   A0 ──┐  地址选择
                   A1 ──┘
```

**地址引脚 (A0, A1)：** 2 引脚 × 2 状态 = 4 个地址

| A1 | A0 | I²C 地址 |
|----|----|----------|
| GND | GND | 0x40 |
| GND | VS  | 0x41 |
| VS  | GND | 0x44 |
| VS  | VS  | 0x45 |

**ALERT 引脚：** 开漏输出，需外接上拉。大多数常见模块（GY-219、CJMCU-219）**未引出** ALERT 引脚。若需要中断告警功能，需飞线焊接到 IC 引脚 3。

**商用模块提示（GY-219 等）：** 通常内建 0.1Ω 分流电阻和 I²C 上拉电阻，只需连接 VCC、GND、SDA、SCL 和 IN+/IN- 即可。

---

### 2.3 INA226（I²C）

**电气参数：** VCC 2.7–5.5V，总线电压最大 36V

```
 [电源 +] ───→ [IN+] ──┤Rshunt├── [IN-] ───→ [负载 +] ───→ [负载 -] ───→ [电源 GND]
                  │                   │
                 VCC                 GND
                  │                   │
                 SDA ←── 4.7kΩ ──→ MCU SDA
                 SCL ←── 4.7kΩ ──→ MCU SCL
                  │
                 ALE (ALERT) ──── 10kΩ 上拉 ──→ MCU GPIO (中断)
                  │
                 A0 ──┐  地址选择
                 A1 ──┘
```

**地址引脚 (A0, A1)：** 每个引脚有 4 种状态（GND / VS / SDA / SCL），共 **16 个地址**（0x40–0x4F）。

| A1 | A0 | 地址 | A1 | A0 | 地址 |
|----|----|------|----|----|------|
| GND | GND | 0x40 | SDA | GND | 0x48 |
| GND | VS  | 0x41 | SDA | VS  | 0x49 |
| GND | SDA | 0x42 | SDA | SDA | 0x4A |
| GND | SCL | 0x43 | SDA | SCL | 0x4B |
| VS  | GND | 0x44 | SCL | GND | 0x4C |
| VS  | VS  | 0x45 | SCL | VS  | 0x4D |
| VS  | SDA | 0x46 | SCL | SDA | 0x4E |
| VS  | SCL | 0x47 | SCL | SCL | 0x4F |

**ALERT 引脚：** 开漏输出，需外接 10kΩ 上拉。CJMCU-226 模块已在排针上引出（标注 ALE）。

**商用模块提示（GY-226 等）：** 内建分流电阻通常为 0.1Ω 或 0.01Ω（检查丝印 R100 / R010）。

---

### 2.4 INA228 / INA237 / INA238 / INA239（I²C 数字系列）

**电气参数：** VCC 2.7–5.5V，总线电压最大 85V（INA228），ADC 可配置量程

```
 [电源 +] ───→ [VS+] ──┤Rshunt├── [VS-] ───→ [负载 +] ───→ [负载 -] ───→ [电源 GND]
                  │                  │
                 VCC                GND
                  │                  │
                 SDA ←── 4.7kΩ ──→ MCU SDA (GPIO 8)
                 SCL ←── 4.7kΩ ──→ MCU SCL (GPIO 9)
                  │
                ALRT ──── 10kΩ 上拉 ──→ MCU GPIO 2 (中断)
                  │
                 A0 ──┐  地址选择（与 INA226 相同，16 地址）
                 A1 ──┘
```

**重要特性：**
- **可配置 ADC 量程：** ±163.84mV（默认，高量程）或 ±40.96mV（高精度模式）
- **内建芯片温度传感器：** 精度 ±1°C，可通过 `readDieTemp()` 读取
- **能量 / 电荷累积：** 40 位寄存器，可通过 `readEnergy()` / `readCharge()` 读取
- **多阈值同时告警：** 总线过压 (BOVL)、总线欠压 (BUVL)、分流过压 (SOVL)、温度上限 (TEMP_LIMIT)、功率上限 (PWR_LIMIT) —— 可同时启用

---

### 2.5 INA229（SPI）

**电气参数：** 与 INA228 相同的测量能力，但使用 SPI 接口

```
 [电源 +] ───→ [INP] ──┤Rshunt├── [INM] ───→ [负载 +] ───→ [负载 -] ───→ [电源 GND]
                  │                  │
                  VS                GND
                  │                  │
                  CS  ←─────────── MCU GPIO 7
                 SCK  ←─────────── MCU GPIO 4
                MISO (SDO) ──────→ MCU GPIO 5
                MOSI (SDI) ←────── MCU GPIO 6
                  │
                ALERT ── 10kΩ 上拉 ──→ MCU GPIO 2 (中断)
```

**SPI 参数：**
- **SPI 模式 1**（CPOL=0, CPHA=1）
- 时钟速率：最高 10 MHz
- ESP32-C3 默认引脚：CS=GPIO7, SCK=GPIO4, MISO=GPIO5, MOSI=GPIO6

> INA229 与 INA228 具有相同的温度传感器和能量/电荷累积功能。

---

### 2.6 INA3221（I²C，3 通道）

**电气参数：** VCC 2.7–5.5V，总线电压最大 26V，3 个独立通道

```
 [电源1 +] ───→ [IN1+] ──┤R1├── [IN1-] ───→ [负载1]     通道 1
 [电源2 +] ───→ [IN2+] ──┤R2├── [IN2-] ───→ [负载2]     通道 2
 [电源3 +] ───→ [IN3+] ──┤R3├── [IN3-] ───→ [负载3]     通道 3
                  │                │
                 VCC              GND
                  │                │
                 SDA ←─ 4.7kΩ ──→ MCU SDA
                 SCL ←─ 4.7kΩ ──→ MCU SCL
                  │
                 CRI ──── 10kΩ 上拉 ──→ MCU GPIO 2  (临界告警, 开漏)
                 WRN ──── 10kΩ 上拉 ──→ MCU GPIO 3  (警告告警, 开漏)
                  PV ──── VPU 接 3.3V ──→ MCU GPIO 4  (电源有效, 推挽)
                  TC ──── (可选) ────────→ MCU GPIO 5  (时序控制)
                  │
                 A0 ── 地址选择
```

**地址引脚 (A0)：** 仅 1 个引脚

| A0 | I²C 地址 |
|----|----------|
| GND | 0x40 |
| VS  | 0x41 |

**特殊引脚说明：**
- **CRI（Critical）：** 任一通道电流超过临界限值时拉低，开漏输出，需外接上拉
- **WRN（Warning）：** 平均测量超过警告限值时拉低，开漏输出，需外接上拉
- **PV（Power Valid）：** 所有使能通道的总线电压在设定窗口内时输出高电平（推挽输出，需将 VPU 接到逻辑电平）
- **TC（Timing Control）：** 电源上电时序异常时拉低，开漏输出

---

### 2.7 商用模块快速接线

大多数市面模块已内建分流电阻和 I²C 上拉电阻，接线非常简单：

```
 [模块 VCC] ──── 3.3V 或 5V
 [模块 GND] ──── GND
 [模块 SDA] ──── MCU SDA
 [模块 SCL] ──── MCU SCL
 [模块 IN+ / V+] ──── 电源正极
 [模块 IN- / V-] ──── 负载正极

 电源 (+) ──→ 模块 IN+ ──┤内建 Rshunt├── 模块 IN- ──→ 负载 (+)
                                                         │
 电源 GND ◄──────────────────────────────── 负载 (-) ◄───┘
```

> **重要：** 不要同时把 IN+ 接到高侧又接到低侧 —— 只能选择一种接法。高侧接法（IN+ 接电源正极侧）是最常见的。

---

## 3. 使用 NiusRobotLab_INA_monitor 工具

[NiusRobotLab_INA_monitor](https://github.com/dunknowcoding/NiusRobotLab_INA_monitor) 是配套的桌面实时监控工具，可以图形化显示电压、电流、功率曲线。

### 3.1 使用步骤

**第 1 步：烧录示例程序**

将任意 `*_basic` 示例烧录到你的开发板。例如 `ina228_basic`：

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

**第 2 步：下载 INA_monitor**

从 GitHub 下载：https://github.com/dunknowcoding/NiusRobotLab_INA_monitor

**第 3 步：连接**

1. 打开 INA_monitor 桌面应用
2. 选择开发板对应的串口（COM 端口）
3. 确保波特率为 **115200**

**第 4 步：开始监控**

点击 **Start** 按钮，即可看到实时曲线图：
- 总线电压 (bus_V)
- 电流 (current_A)
- 功率 (power_W)

**第 5 步：调节参数**

INA_monitor 可以发送串口命令来控制采样：
- `SR <Hz>` —— 设置采样率（1–400 Hz）
- `STOP` —— 暂停采样
- `RSHUNT <ohm>` —— 设置分流电阻值
- `IMAX <A>` —— 设置最大预期电流

### 3.2 JSONL 协议说明

开发板以 **JSON Lines** 格式输出每一个测量样本，每行一个 JSON 对象：

```json
{"v":1,"chip":"INA228","bus_V":3.3,"current_A":0.15,"power_W":0.495}
```

**INA_monitor 识别的字段：**

| 字段 | 类型 | 说明 |
|------|------|------|
| `v` | int | 协议版本（固定为 1） |
| `chip` | string | 芯片名称 |
| `bus_V` | float | 总线电压（V） |
| `current_A` | float | 电流（A） |
| `power_W` | float | 功率（W） |

**INA_monitor 忽略的字段：**
- `type`（如 `"ALERT_CFG"`、`"ALERT_EVENT"`、`"ENERGY"` 等）
- `temp_C`（芯片温度）
- `shunt_uV`（分流电压）
- `_note`（注释）
- 以及任何 `setExtraFieldsPrinter()` 追加的自定义字段

这意味着你可以在 JSONL 中添加额外字段而不影响 INA_monitor 的正常显示。

### 3.3 串口命令列表

通过 INA_monitor 或串口监视器发送以下命令：

| 命令 | 说明 |
|------|------|
| `START` | 开始 JSONL 流式输出 |
| `STOP` | 停止流式输出 |
| `SR <Hz>` | 设置采样率（I²C: 1–400 Hz，SPI: 1–2000 Hz） |
| `RSHUNT <ohm>` | 设置分流电阻值（如 `RSHUNT 0.1`） |
| `IMAX <A>` | 设置最大预期电流（如 `IMAX 10`） |
| `PING` | 连接测试 |
| `ALERT BOV <V>` | 总线过压阈值（INA226/228 系列） |
| `ALERT BUV <V>` | 总线欠压阈值 |
| `ALERT SOV <uV>` | 分流过压阈值 |
| `ALERT CNVR` | 转换完成中断（INA228/229） |
| `ALERT OFF` | 关闭告警 |
| `DIAG` | 读取诊断寄存器 |

---

## 4. 独立使用（不使用 Monitor 工具）

本库支持两种使用模式，可以单独使用或同时使用：

- **模式 A：JSONL 流式** —— 调用 `tick()`，由主机发送 `START` / `STOP` 控制
- **模式 B：直接读数** —— 调用 `readBusVoltage()` 等 API，不产生 Serial 输出

### 4.1 仅模式 A：JSONL 流式

这是所有 `*_basic` 示例的默认模式。`loop()` 中只需调用 `tick()`。

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

你也可以在代码中主动控制流式输出：

```cpp
void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.begin(8, 9);
  sensor.setSampleRate(50);      // 50 Hz 采样
  sensor.startStreaming();        // 无需等待 START 命令
}
```

### 4.2 仅模式 B：直接读数

不调用 `tick()`，直接使用测量 API：

```cpp
#include <INA_Series_Sensor.h>

static InaBridge228 sensor("INA228", 0x40);

void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.begin(8, 9);
  sensor.setRshunt(0.1);    // 分流电阻 100 mΩ
  sensor.setImax(10.0);     // 最大预期电流 10 A
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

### 4.3 两种模式同时使用

`tick()` 处理 JSONL 流式输出，同时你的代码也可以调用 `readBusVoltage()` 等：

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
  sensor.tick();    // 模式 A：INA_monitor 可以发送 START/STOP

  // 模式 B：自己的逻辑
  if (sensor.dataReady()) {
    float v = sensor.readBusVoltage();
    float i = sensor.readCurrent();
    if (i > 5.0) {
      digitalWrite(LED_BUILTIN, HIGH);   // 过流指示
    }
  }
}
```

> **注意：** 同时使用两种模式时，`dataReady()` 可能在流式采样间隙返回 `false`，因为流式代码也会读取并清除转换就绪标志。

### 4.4 代码示例：INA228 温度与能量监控

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
  g_drv.resetAccumulators();           // 清零能量/电荷累积器
}

void loop() {
  sensor.tick();

  if (millis() - lastReport >= 5000) {
    lastReport = millis();

    float temp;
    g_drv.readDieTemp_C(temp);
    Serial.print("芯片温度: "); Serial.print(temp, 1); Serial.println(" °C");

    float clsb = sensor.currentLsb();
    if (clsb > 0) {
      float energy_J = 0, charge_C = 0;
      g_drv.readEnergy_J(energy_J, clsb);
      g_drv.readCharge_C(charge_C, clsb);
      Serial.print("能量: "); Serial.print(energy_J, 4); Serial.println(" J");
      Serial.print("电荷: "); Serial.print(charge_C, 4); Serial.println(" C");
    }
  }
}
```

### 4.5 代码示例：INA3221 多通道独立分流电阻

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

### 4.6 代码示例：告警中断（INA228 总线过压）

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
      Serial.println("过压告警触发！");
    }
  }
}
```

**中断告警模式说明：**
- Bridge 对象（`InaBridge228`）负责 JSONL 输出和 `tick()`
- Driver 对象（`Ina::Ina228Driver`）负责告警配置和状态读取
- 两者使用相同的 I²C 地址，可以共存
- ALERT 引脚为开漏输出，需要外接上拉电阻（10kΩ）

### 4.7 代码示例：INA3221 临界过流中断

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
        Serial.println(" 临界过流！");
      }
    }
  }
}
```

### 4.8 代码示例：单次测量（One-Shot）

适用于低功耗场景，设置采样后读取单个值：

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

  delay(1000);   // 每秒读取一次
}
```

---

## 5. 市面常见 INA 模块兼容性

### 5.1 模块对照表

| 模块名称 | 芯片 | 内建 Rshunt | 默认 I²C 地址 | 内建上拉 | 供电 |
|----------|------|-------------|---------------|---------|------|
| GY-INA219 / CJMCU-219 | INA219 | 0.1 Ω | 0x40 | ✅ | 3–5V |
| Adafruit INA219 (#904) | INA219 | 0.1 Ω | 0x40 | ✅ | 3–5V |
| GY-INA226 / CJMCU-226 | INA226 | 0.1 Ω 或 0.01 Ω | 0x40 | ✅ | 3–5V |
| INA3221 分线板 (CJMCU-3221) | INA3221 | 3× 0.1 Ω | 0x40 | ✅ | 3–5V |
| Adafruit INA3221 | INA3221 | 3× 0.1 Ω | 0x40 | ✅ | 3–5V |
| Adafruit INA228 (#5832) | INA228 | 0.015 Ω | 0x40 | ✅ | 3–5V |

### 5.2 各模块配置建议

#### GY-INA219 / CJMCU-219 / Adafruit INA219

```cpp
static InaBridge219 sensor("INA219", 0x40);

void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.begin(8, 9);
  sensor.setRshunt(0.1);    // 模块内建 0.1Ω 分流电阻
  sensor.setImax(3.2);      // 最大 3.2A（0.1Ω × 3.2A = 320mV，在量程内）
}
```

- **setRshunt：** `0.1`（模块标配 R100 贴片电阻）
- **setImax：** 推荐 `3.2`（320mV 分流电压在量程内且精度最佳）
- **I²C 地址：** 默认 0x40（A0=GND, A1=GND）
- **内建上拉：** 有，无需额外添加

#### GY-INA226 / CJMCU-226

```cpp
static InaBridge226 sensor("INA226", 0x40);

void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.begin(8, 9);
  sensor.setRshunt(0.1);    // 检查你的模块！可能是 0.1Ω 或 0.01Ω
  sensor.setImax(8.0);      // 根据实际需求设置
}
```

- **setRshunt：** `0.1` 或 `0.01`（检查模块上的分流电阻丝印，R100 = 0.1Ω，R010 = 0.01Ω）
- **setImax：** 根据应用设置。0.01Ω 分流可测更大电流
- **I²C 地址：** 默认 0x40
- **内建上拉：** 有

#### INA3221 分线板 (CJMCU-3221 / Adafruit)

```cpp
static InaBridge3221 sensor("INA3221", 0x40);

void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.begin(8, 9);
  sensor.setRshunt(0.1);    // 三通道统一设置
  // 或分别设置：
  // sensor.setRshunt(1, 0.100);
  // sensor.setRshunt(2, 0.100);
  // sensor.setRshunt(3, 0.100);
}
```

- **setRshunt：** `0.1`（三通道各有一个 0.1Ω 分流电阻）
- **I²C 地址：** 0x40（A0=GND）；可改为 0x41（A0=VS）
- **内建上拉：** 有

#### Adafruit INA228 (#5832, STEMMA QT)

```cpp
static InaBridge228 sensor("INA228", 0x40);

void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.begin(8, 9);
  sensor.setRshunt(0.015);   // 模块使用 15mΩ 分流电阻
  sensor.setImax(10.0);      // 根据实际需求
}
```

- **setRshunt：** `0.015`（15mΩ，请查阅 Adafruit 模块文档确认）
- **setImax：** 根据你的负载设置
- **I²C 地址：** 默认 0x40（模块背面有焊盘可改地址）
- **内建上拉：** 有（STEMMA QT 接口）
- **ALERT 引脚：** 已引出在排针上

---

## 6. 注意事项与常见问题

### 6.1 ESP32 GPIO 重映射

`begin(SDA_pin, SCL_pin)` 中的引脚参数 **仅在 ESP32 系列上有效**。

- **ESP32 / ESP32-S3 / ESP32-C3：** 可以自由指定 SDA 和 SCL 的 GPIO 编号
- **Arduino AVR (Uno / Mega / Nano)：** 忽略引脚参数，使用板载默认的 I²C 引脚（Uno: SDA=A4, SCL=A5）
- **RP2040 / SAMD / STM32：** 使用板载默认引脚

### 6.2 串口波特率

**必须使用 115200 波特率。** 所有示例和 INA_monitor 工具都以 115200 为标准。

```cpp
Serial.begin(115200);   // 必须
```

### 6.3 串口无输出

如果打开串口监视器后看不到任何内容：

1. **检查 USB 线缆：** 确认是数据线而非仅充电线（充电线只有 VCC 和 GND，没有数据线）
2. **检查波特率：** 串口监视器必须设为 **115200**
3. **检查 COM 端口：** 确认选择了正确的端口
4. **ESP32 USB CDC：** 某些 ESP32 开发板需要在 Arduino IDE 中启用 **USB CDC On Boot**
5. **烧录 `diag_serial_only` 示例：** 该示例不需要任何 INA 硬件，仅测试 USB 串口通信是否正常
6. **关闭 INA_monitor：** 如果 INA_monitor 已占用串口，Arduino 串口监视器将无法连接。同一时间只能有一个程序打开串口

### 6.4 ESP32 中断服务程序 (ISR)

在 ESP32 / ESP8266 上，中断服务函数**必须**使用 `IRAM_ATTR`（将函数放入 IRAM）：

```cpp
#if defined(ESP32) || defined(ESP8266)
  #define INA_ISR_ATTR IRAM_ATTR
#else
  #define INA_ISR_ATTR
#endif

static void INA_ISR_ATTR onAlert() { alertTriggered = true; }
```

所有 `*_alert_interrupt` 示例已包含此兼容代码。

### 6.5 同一 I²C 总线上多个 INA

可以将多个 INA 芯片连到同一条 I²C 总线，但每个芯片 **必须使用不同的 I²C 地址**：

```cpp
static InaBridge228 sensor1("INA228-A", 0x40);
static InaBridge228 sensor2("INA228-B", 0x41);

void setup() {
  Serial.begin(115200);
  delay(500);
  sensor1.begin(8, 9);
  sensor2.begin(8, 9);   // 共用相同的 SDA/SCL
}

void loop() {
  sensor1.tick();
  sensor2.tick();
}
```

通过 A0 / A1 引脚设置不同地址。

### 6.6 `dataReady()` 始终返回 false

**可能原因：**
- 如果同时使用流式模式（`tick()` + `START`），流式代码会读取 DIAG_ALRT 寄存器并清除转换就绪标志。`dataReady()` 在下一次转换完成前会返回 `false`
- **解决方案：** 如果仅需直接读数，不要调用 `tick()` 或不要发送 `START` 命令

### 6.7 电流读数为 0

**排查步骤：**
1. **检查校准：** 确认已调用 `setRshunt()` 和 `setImax()`，且值正确
2. **检查接线：** 分流电阻必须**串联**在电流路径中，而不是并联
3. **确认负载有电流：** 如果负载未通电或电流极小，读数接近 0 是正常的
4. **检查分流电阻值：** `setRshunt()` 的参数必须与实际硬件上的分流电阻一致

### 6.8 AVR 平台 Flash 占用较大

本库在编译时会包含所有 Bridge 类的代码。在 AVR 平台（如 Arduino Uno，32KB Flash）上：

- **Flash 占用约 50–62%** 是正常的
- 如果你的项目代码较大，可能会超出 Flash 容量
- **建议：** 对于 Flash 紧张的项目，考虑使用 ESP32 或 RP2040 等更大容量的平台

### 6.9 InaBridgeCh1 的限制

`InaBridgeCh1`（用于 INA2227 / INA4230 / INA4235）有两个特殊限制：

- **无 `dataReady()` 方法：** 这些芯片没有可靠的转换就绪标志
- **无 `setImax()` 方法：** 无校准寄存器，电流 = 分流电压 / Rshunt（自动计算）

---

## 7. 示例一览表

本库包含 **69 个**可直接烧录的示例。

> **命名规则：**
> - `*_basic` —— 基础 JSONL 桥接，最简代码
> - `*_advanced` —— 高级功能（温度/能量/自定义字段 + 多阈值告警）
> - `*_alert_interrupt` —— 硬件中断告警
> - `*_q1_*` —— 汽车级 (Q1) 变体，代码结构相同

### 诊断与占位

| # | 示例名称 | 芯片 | 类型 | Bridge 类 | 功能说明 |
|---|---------|------|------|-----------|---------|
| 1 | `diag_serial_only` | — | 诊断 | — | USB 串口测试（无 I²C），排查通信问题 |
| 2 | `unknown_basic` | — | 占位 | `InaBridgeUnknown` | 无传感器占位，输出全 0，验证 JSONL 协议 |

### INA219（2 个）

| # | 示例名称 | 芯片 | 类型 | Bridge 类 | 功能说明 |
|---|---------|------|------|-----------|---------|
| 3 | `ina219_basic` | INA219 | basic | `InaBridge219` | 基础 JSONL 桥接 |
| 4 | `ina219_alert_interrupt` | INA219 | alert_interrupt | `InaBridge219` | 总线过压中断 + ALERT_EVENT |

### INA220（4 个）

| # | 示例名称 | 芯片 | 类型 | Bridge 类 | 功能说明 |
|---|---------|------|------|-----------|---------|
| 5 | `ina220_basic` | INA220 | basic | `InaBridge219` | 基础 JSONL 桥接 |
| 6 | `ina220_alert_interrupt` | INA220 | alert_interrupt | `InaBridge219` | 总线过压中断 |
| 7 | `ina220_q1_basic` | INA220-Q1 | basic | `InaBridge219` | Q1 汽车级变体 |
| 8 | `ina220_q1_alert_interrupt` | INA220-Q1 | alert_interrupt | `InaBridge219` | Q1 总线过压中断 |

### INA226（4 个）

| # | 示例名称 | 芯片 | 类型 | Bridge 类 | 功能说明 |
|---|---------|------|------|-----------|---------|
| 9 | `ina226_basic` | INA226 | basic | `InaBridge226` | 基础 JSONL 桥接 |
| 10 | `ina226_alert_interrupt` | INA226 | alert_interrupt | `InaBridge226` | 总线过压中断 |
| 11 | `ina226_q1_basic` | INA226-Q1 | basic | `InaBridge226` | Q1 汽车级变体 |
| 12 | `ina226_q1_alert_interrupt` | INA226-Q1 | alert_interrupt | `InaBridge226` | Q1 总线过压中断 |

### INA228（6 个）

| # | 示例名称 | 芯片 | 类型 | Bridge 类 | 功能说明 |
|---|---------|------|------|-----------|---------|
| 13 | `ina228_basic` | INA228 | basic | `InaBridge228` | 基础 JSONL 桥接 |
| 14 | `ina228_advanced` | INA228 | advanced | `InaBridge228` | 温度 + 能量/电荷 + 多阈值告警 |
| 15 | `ina228_alert_interrupt` | INA228 | alert_interrupt | `InaBridge228` | 总线过压中断 |
| 16 | `ina228_q1_basic` | INA228-Q1 | basic | `InaBridge228` | Q1 汽车级变体 |
| 17 | `ina228_q1_advanced` | INA228-Q1 | advanced | `InaBridge228` | Q1 温度 + 能量/电荷 + 告警 |
| 18 | `ina228_q1_alert_interrupt` | INA228-Q1 | alert_interrupt | `InaBridge228` | Q1 总线过压中断 |

### INA229 — SPI（6 个）

| # | 示例名称 | 芯片 | 类型 | Bridge 类 | 功能说明 |
|---|---------|------|------|-----------|---------|
| 19 | `ina229_basic` | INA229 | basic | `InaBridge229Spi` | SPI JSONL 桥接 |
| 20 | `ina229_advanced` | INA229 | advanced | `InaBridge229Spi` | SPI 温度 + 能量/电荷 + CNVR 告警 |
| 21 | `ina229_alert_interrupt` | INA229 | alert_interrupt | `InaBridge229Spi` | SPI 转换完成 (CNVR) 中断 |
| 22 | `ina229_q1_basic` | INA229-Q1 | basic | `InaBridge229Spi` | Q1 SPI 桥接 |
| 23 | `ina229_q1_advanced` | INA229-Q1 | advanced | `InaBridge229Spi` | Q1 SPI 温度 + 能量 + 告警 |
| 24 | `ina229_q1_alert_interrupt` | INA229-Q1 | alert_interrupt | `InaBridge229Spi` | Q1 SPI CNVR 中断 |

### INA230–234、INA236（12 个）

| # | 示例名称 | 芯片 | 类型 | Bridge 类 | 功能说明 |
|---|---------|------|------|-----------|---------|
| 25 | `ina230_basic` | INA230 | basic | `InaBridge226` | 基础 JSONL 桥接 |
| 26 | `ina230_alert_interrupt` | INA230 | alert_interrupt | `InaBridge226` | 总线过压中断 |
| 27 | `ina231_basic` | INA231 | basic | `InaBridge226` | 基础 JSONL 桥接 |
| 28 | `ina231_alert_interrupt` | INA231 | alert_interrupt | `InaBridge226` | 总线过压中断 |
| 29 | `ina232_basic` | INA232 | basic | `InaBridge226` | 基础 JSONL 桥接 |
| 30 | `ina232_alert_interrupt` | INA232 | alert_interrupt | `InaBridge226` | 总线过压中断 |
| 31 | `ina233_basic` | INA233 | basic | `InaBridge226` | 基础 JSONL 桥接 |
| 32 | `ina233_alert_interrupt` | INA233 | alert_interrupt | `InaBridge226` | 总线过压中断 |
| 33 | `ina234_basic` | INA234 | basic | `InaBridge226` | 基础 JSONL 桥接 |
| 34 | `ina234_alert_interrupt` | INA234 | alert_interrupt | `InaBridge226` | 总线过压中断 |
| 35 | `ina236_basic` | INA236 | basic | `InaBridge226` | 基础 JSONL 桥接 |
| 36 | `ina236_alert_interrupt` | INA236 | alert_interrupt | `InaBridge226` | 总线过压中断 |

### INA237（6 个）

| # | 示例名称 | 芯片 | 类型 | Bridge 类 | 功能说明 |
|---|---------|------|------|-----------|---------|
| 37 | `ina237_basic` | INA237 | basic | `InaBridge228` | 基础 JSONL 桥接 |
| 38 | `ina237_advanced` | INA237 | advanced | `InaBridge228` | 温度 + 能量/电荷 + 多阈值告警 |
| 39 | `ina237_alert_interrupt` | INA237 | alert_interrupt | `InaBridge228` | 总线过压中断 |
| 40 | `ina237_q1_basic` | INA237-Q1 | basic | `InaBridge228` | Q1 汽车级变体 |
| 41 | `ina237_q1_advanced` | INA237-Q1 | advanced | `InaBridge228` | Q1 温度 + 能量 + 告警 |
| 42 | `ina237_q1_alert_interrupt` | INA237-Q1 | alert_interrupt | `InaBridge228` | Q1 总线过压中断 |

### INA238（6 个）

| # | 示例名称 | 芯片 | 类型 | Bridge 类 | 功能说明 |
|---|---------|------|------|-----------|---------|
| 43 | `ina238_basic` | INA238 | basic | `InaBridge228` | 基础 JSONL 桥接 |
| 44 | `ina238_advanced` | INA238 | advanced | `InaBridge228` | 温度 + 能量/电荷 + 多阈值告警 |
| 45 | `ina238_alert_interrupt` | INA238 | alert_interrupt | `InaBridge228` | 总线过压中断 |
| 46 | `ina238_q1_basic` | INA238-Q1 | basic | `InaBridge228` | Q1 汽车级变体 |
| 47 | `ina238_q1_advanced` | INA238-Q1 | advanced | `InaBridge228` | Q1 温度 + 能量 + 告警 |
| 48 | `ina238_q1_alert_interrupt` | INA238-Q1 | alert_interrupt | `InaBridge228` | Q1 总线过压中断 |

### INA239（6 个）

| # | 示例名称 | 芯片 | 类型 | Bridge 类 | 功能说明 |
|---|---------|------|------|-----------|---------|
| 49 | `ina239_basic` | INA239 | basic | `InaBridge228` | 基础 JSONL 桥接 |
| 50 | `ina239_advanced` | INA239 | advanced | `InaBridge228` | 温度 + 能量/电荷 + 多阈值告警 |
| 51 | `ina239_alert_interrupt` | INA239 | alert_interrupt | `InaBridge228` | 总线过压中断 |
| 52 | `ina239_q1_basic` | INA239-Q1 | basic | `InaBridge228` | Q1 汽车级变体 |
| 53 | `ina239_q1_advanced` | INA239-Q1 | advanced | `InaBridge228` | Q1 温度 + 能量 + 告警 |
| 54 | `ina239_q1_alert_interrupt` | INA239-Q1 | alert_interrupt | `InaBridge228` | Q1 总线过压中断 |

### INA3221（6 个）

| # | 示例名称 | 芯片 | 类型 | Bridge 类 | 功能说明 |
|---|---------|------|------|-----------|---------|
| 55 | `ina3221_basic` | INA3221 | basic | `InaBridge3221` | 3 通道 JSONL 桥接 |
| 56 | `ina3221_advanced` | INA3221 | advanced | `InaBridge3221` | CRI + WAR + PV + 分流电压求和 |
| 57 | `ina3221_alert_interrupt` | INA3221 | alert_interrupt | `InaBridge3221` | CRI 临界过流中断 |
| 58 | `ina3221_q1_basic` | INA3221-Q1 | basic | `InaBridge3221` | Q1 汽车级变体 |
| 59 | `ina3221_q1_advanced` | INA3221-Q1 | advanced | `InaBridge3221` | Q1 CRI + WAR + PV + 求和 |
| 60 | `ina3221_q1_alert_interrupt` | INA3221-Q1 | alert_interrupt | `InaBridge3221` | Q1 CRI 中断 |

### INA2227（2 个）

| # | 示例名称 | 芯片 | 类型 | Bridge 类 | 功能说明 |
|---|---------|------|------|-----------|---------|
| 61 | `ina2227_basic` | INA2227 | basic | `InaBridgeCh1` | CH1 JSONL 桥接 |
| 62 | `ina2227_alert_interrupt` | INA2227 | alert_interrupt | `InaBridgeCh1` | CH1 告警中断 |

### INA4230 / INA4235（4 个）

| # | 示例名称 | 芯片 | 类型 | Bridge 类 | 功能说明 |
|---|---------|------|------|-----------|---------|
| 63 | `ina4230_basic` | INA4230 | basic | `InaBridgeCh1` | CH1 JSONL 桥接 |
| 64 | `ina4230_alert_interrupt` | INA4230 | alert_interrupt | `InaBridgeCh1` | CH1 告警中断 |
| 65 | `ina4235_basic` | INA4235 | basic | `InaBridgeCh1` | CH1 JSONL 桥接 |
| 66 | `ina4235_alert_interrupt` | INA4235 | alert_interrupt | `InaBridgeCh1` | CH1 告警中断 |

### INA740X（3 个）

| # | 示例名称 | 芯片 | 类型 | Bridge 类 | 功能说明 |
|---|---------|------|------|-----------|---------|
| 67 | `ina740x_basic` | INA740X | basic | `InaBridge228` | 基础 JSONL 桥接 |
| 68 | `ina740x_advanced` | INA740X | advanced | `InaBridge228` | 温度 + 能量/电荷 + 多阈值告警 |
| 69 | `ina740x_alert_interrupt` | INA740X | alert_interrupt | `InaBridge228` | 总线过压中断 |

### Bridge 类速查

| Bridge 类 | 适用芯片 | 接口 | 通道 | 高级功能 |
|-----------|---------|------|------|---------|
| `InaBridge219` | INA219, INA220, INA220-Q1 | I²C | 1 | — |
| `InaBridge226` | INA226, INA226-Q1, INA230–234, INA236 | I²C | 1 | — |
| `InaBridge228` | INA228, INA228-Q1, INA237–239 (+Q1), INA740X | I²C | 1 | 温度、能量、电荷 |
| `InaBridge229Spi` | INA229, INA229-Q1 | SPI | 1 | 温度、能量、电荷 |
| `InaBridge3221` | INA3221, INA3221-Q1 | I²C | 3 | 逐通道控制 |
| `InaBridgeCh1` | INA2227, INA4230, INA4235 | I²C | 1 (CH1) | — |
| `InaBridgeUnknown` | — | — | — | 占位（无传感器） |

---

> **更多信息：** 完整 API 参考请查阅 [API 参考手册 (中文)](API_CHN.md) | [API Reference (EN)](API_EN.md)  
> **INA Monitor 工具：** [NiusRobotLab_INA_monitor](https://github.com/dunknowcoding/NiusRobotLab_INA_monitor)  
> **问题反馈：** [GitHub Issues](https://github.com/dunknowcoding/INA_series_sensor/issues)
