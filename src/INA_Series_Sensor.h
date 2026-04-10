/**
 * @file INA_Series_Sensor.h
 * @brief Texas Instruments INA current/power monitors — JSON Lines serial bridge for the INA Monitor
 *        desktop application (and compatible tools). Target: ESP32-class boards with USB serial.
 *
 * Install: Sketch → Include Library → Add .ZIP Library, or copy this folder under
 * `Documents/Arduino/libraries/`, then use `#include <INA_Series_Sensor.h>`.
 */

#pragma once

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
