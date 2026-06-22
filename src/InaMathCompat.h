/**
 * @file InaMathCompat.h
 * @brief Safe <math.h> include on cores that define min/max macros (e.g. ArduinoNRF).
 */
#pragma once

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <math.h>
