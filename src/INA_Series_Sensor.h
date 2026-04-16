/**
 * @file INA_Series_Sensor.h
 * @brief All-in-one Arduino library for Texas Instruments INA-series
 *        current/voltage/power monitors.
 *
 * Supported chips: INA219, INA220, INA226, INA228, INA229 (SPI), INA230–239,
 * INA740X, INA3221, INA2227, INA4230, INA4235.
 *
 * ## Two Usage Modes
 *
 * **Mode A – JSONL Streaming (for NiusRobotLab_INA_monitor)**
 *   Create a bridge object, call begin() in setup() and tick() in loop().
 *   The host sends START/STOP/SR commands; the bridge emits JSON Lines.
 *
 * **Mode B – Standalone Direct Reading**
 *   After begin(), call readBusVoltage(), readCurrent(), readPower(), etc.
 *   directly. No Serial output. Use dataReady() to poll for new conversions.
 *
 * Both modes can be combined in the same sketch.
 *
 * ## Quick Start
 * @code
 *   #include <INA_Series_Sensor.h>
 *
 *   static InaBridge228 sensor("INA228", 0x40);
 *
 *   void setup() {
 *     Serial.begin(115200);
 *     sensor.begin(8, 9);       // SDA=8, SCL=9
 *   }
 *
 *   void loop() {
 *     sensor.tick();             // Mode A: JSONL streaming
 *
 *     // Mode B: standalone reading
 *     if (sensor.dataReady()) {
 *       float v = sensor.readBusVoltage();
 *       float i = sensor.readCurrent();
 *       float p = sensor.readPower();
 *     }
 *   }
 * @endcode
 *
 * @author  NiusRobotLab / dunknowcoding
 * @see     https://github.com/dunknowcoding/INA_series_sensor
 */

#pragma once

// ── Bridge layer (JSONL protocol + standalone measurement API) ────
#include "InaJsonlProtocol.h"
#include "InaWireCompat.h"
#include "InaSpiCompat.h"
#include "InaBridge219.h"
#include "InaBridge226.h"
#include "InaBridge228.h"
#include "InaBridge3221.h"
#include "InaBridge229Spi.h"
#include "InaBridgeCh1.h"
#include "InaBridgeUnknown.h"

// ── Driver layer (low-level register access, alerts, temperature) ─
#include "ina/InaTypes.h"
#include "ina/InaI2cBus.h"
#include "ina/Ina219Driver.h"
#include "ina/Ina226Driver.h"
#include "ina/Ina228Driver.h"
#include "ina/Ina3221Driver.h"
#include "ina/InaCh1Driver.h"
#include "ina/InaSpiBus.h"
#include "ina/Ina229Driver.h"
