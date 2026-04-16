# Changelog

All notable changes to the **INA Series Sensor** library are documented here.

## [0.5.0] — 2026-04-16

### Added

- **Standalone direct-read API** for all Bridge classes — `readBusVoltage()`, `readShuntVoltage()`, `readCurrent()`, `readPower()` return measurement values without any Serial output, enabling usage without the INA_monitor tool.
- **`dataReady()`** polling method on all supported Bridge classes (228, 219, 226, 229, 3221) to check for new conversion results.
- **Programmatic control methods**: `startStreaming()`, `stopStreaming()`, `setSampleRate()`, `setRshunt()`, `setImax()`, `isStreaming()`, `rshunt()`, `imax()`, `address()`.
- **INA228/229 advanced measurements**: `readDieTemp()` (°C), `readEnergy()` (J), `readCharge()` (C), `resetAccumulators()`.
- **INA3221 per-channel shunt resistor**: `setRshunt(ch, ohm)` allows each channel to use a different shunt value.
- **INA3221 channel enable/disable**: `enableChannel(ch, enable)`, `isChannelEnabled(ch)` for controlling which channels participate in conversion.
- **`begin()` method** as the primary initialization entry point (replaces `beginI2c()` which is kept as a legacy alias).
- **40 new example sketches**: `*_advanced` (temperature, energy, charge) and `*_alert_interrupt` (hardware interrupt with ISR) for all chip families.
- **Complete API reference** (`docs/API.md`, 1500+ lines) — every public method documented with parameters, return values, code examples, and notes.
- **Comprehensive user guide** (`docs/USAGE.md`, 900+ lines) — wiring diagrams, commercial module compatibility, INA_monitor setup, standalone usage, FAQ.
- `InaSerialLineReader.h` — extracted non-blocking serial reader as a standalone header.
- `src/ina/` driver layer — low-level register access for alerts, temperature, energy, and charge.
- Cross-platform compile verification script (`scripts/compile-matrix.ps1`).

### Changed

- **Renamed examples**: `*_bridge` → `*_basic` for clarity (28 examples).
- **Renamed variables in examples**: `g_bridge` → `sensor`, `g_alertFlag` → `alertTriggered` for better readability.
- **Renamed internal method**: `sampleOnce()` → `emitJsonSample()` to clarify that it produces Serial output.
- **`library.properties`**: `architectures` changed from platform-specific list to `*` (all platforms).
- Improved Doxygen comments on all public methods in all Bridge classes.
- `INA_Series_Sensor.h` main header now includes Quick Start documentation.

### Fixed

- **`InaSerialLineReader` overflow bug**: when an input line exceeded the buffer capacity, the tail of the overflowed line was incorrectly returned as a new line. Now the entire overflowed line is discarded.
- Added `#include <math.h>` to all driver `.cpp` files that use `isfinite()` / `fabsf()` for cross-platform portability.
- `Wire.requestFrom()` argument types cast to `(int)` for broader platform compatibility.

## [0.2.5] — 2026-04-14

- Initial public release with JSONL bridge support for 26 INA chip variants.
- 28 basic example sketches.
- Compatible with NiusRobotLab_INA_monitor desktop tool.
