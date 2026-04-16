/**
 * Rewrites each examples subfolder .ino to use INA_Series_Sensor bridge classes.
 * Run: node scripts/wire-examples-library.mjs
 *
 * Only folders listed in SPECS below are overwritten. Folders not in SPECS — e.g.
 * driver_smoke_*, diag_serial_only, *_advanced — must be maintained by hand.
 */
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const EXAMPLES = path.join(__dirname, "..", "examples");

const SPECS = [
  { dir: "ina219_basic", file: "ina219_basic.ino", cls: "InaBridge219", args: '"INA219", 0x40', hdr: "INA219" },
  { dir: "ina220_basic", file: "ina220_basic.ino", cls: "InaBridge219", args: '"INA220", 0x40', hdr: "INA220" },
  { dir: "ina220_q1_basic", file: "ina220_q1_basic.ino", cls: "InaBridge219", args: '"INA220-Q1", 0x40', hdr: "INA220-Q1" },
  { dir: "ina226_basic", file: "ina226_basic.ino", cls: "InaBridge226", args: '"INA226", 0x40', hdr: "INA226" },
  { dir: "ina226_q1_basic", file: "ina226_q1_basic.ino", cls: "InaBridge226", args: '"INA226-Q1", 0x40', hdr: "INA226-Q1" },
  { dir: "ina230_basic", file: "ina230_basic.ino", cls: "InaBridge226", args: '"INA230", 0x40, "TI SLYSF02 (INA230)"', hdr: "INA230" },
  { dir: "ina231_basic", file: "ina231_basic.ino", cls: "InaBridge226", args: '"INA231", 0x40, "TI SLYSF02 (INA231)"', hdr: "INA231" },
  { dir: "ina232_basic", file: "ina232_basic.ino", cls: "InaBridge226", args: '"INA232", 0x40, "TI SLYSF02 (INA232)"', hdr: "INA232" },
  { dir: "ina233_basic", file: "ina233_basic.ino", cls: "InaBridge226", args: '"INA233", 0x40, "TI SLYSF02 (INA233)"', hdr: "INA233" },
  { dir: "ina234_basic", file: "ina234_basic.ino", cls: "InaBridge226", args: '"INA234", 0x40, "TI SLYSF02 (INA234)"', hdr: "INA234" },
  { dir: "ina236_basic", file: "ina236_basic.ino", cls: "InaBridge226", args: '"INA236", 0x40, "TI SLYSF02 (INA236)"', hdr: "INA236" },
  { dir: "ina228_basic", file: "ina228_basic.ino", cls: "InaBridge228", args: '"INA228", 0x40', hdr: "INA228" },
  { dir: "ina228_q1_basic", file: "ina228_q1_basic.ino", cls: "InaBridge228", args: '"INA228-Q1", 0x40, "TI INA228-Q1"', hdr: "INA228-Q1" },
  { dir: "ina237_basic", file: "ina237_basic.ino", cls: "InaBridge228", args: '"INA237", 0x40, "TI INA237"', hdr: "INA237" },
  { dir: "ina237_q1_basic", file: "ina237_q1_basic.ino", cls: "InaBridge228", args: '"INA237-Q1", 0x40, "TI INA237-Q1"', hdr: "INA237-Q1" },
  { dir: "ina238_basic", file: "ina238_basic.ino", cls: "InaBridge228", args: '"INA238", 0x40, "TI INA238"', hdr: "INA238" },
  { dir: "ina238_q1_basic", file: "ina238_q1_basic.ino", cls: "InaBridge228", args: '"INA238-Q1", 0x40, "TI INA238-Q1"', hdr: "INA238-Q1" },
  { dir: "ina239_basic", file: "ina239_basic.ino", cls: "InaBridge228", args: '"INA239", 0x40, "TI INA239"', hdr: "INA239" },
  { dir: "ina239_q1_basic", file: "ina239_q1_basic.ino", cls: "InaBridge228", args: '"INA239-Q1", 0x40, "TI INA239-Q1"', hdr: "INA239-Q1" },
  { dir: "ina740x_basic", file: "ina740x_basic.ino", cls: "InaBridge228", args: '"INA740X", 0x40, "TI INA740X"', hdr: "INA740X" },
  { dir: "ina3221_basic", file: "ina3221_basic.ino", cls: "InaBridge3221", args: '"INA3221", 0x40', hdr: "INA3221" },
  { dir: "ina3221_q1_basic", file: "ina3221_q1_basic.ino", cls: "InaBridge3221", args: '"INA3221-Q1", 0x40', hdr: "INA3221-Q1" },
  { dir: "ina229_basic", file: "ina229_basic.ino", cls: "SPI229", args: "", hdr: "INA229" },
  { dir: "ina229_q1_basic", file: "ina229_q1_basic.ino", cls: "SPI229Q1", args: "", hdr: "INA229-Q1" },
  { dir: "ina2227_basic", file: "ina2227_basic.ino", cls: "InaBridgeCh1", args: '"INA2227", "INA2227 bridge ready (CH1)", 0x40', hdr: "INA2227" },
  { dir: "ina4230_basic", file: "ina4230_basic.ino", cls: "InaBridgeCh1", args: '"INA4230", "INA4230 bridge ready (CH1)", 0x40', hdr: "INA4230" },
  { dir: "ina4235_basic", file: "ina4235_basic.ino", cls: "InaBridgeCh1", args: '"INA4235", "INA4235 bridge ready (CH1)", 0x40', hdr: "INA4235" },
  { dir: "unknown_basic", file: "unknown_basic.ino", cls: "InaBridgeUnknown", args: "", hdr: "UNKNOWN" }
];

function inoI2c(cls, varName, args, title) {
  return `/**
 * ${title} basic example — JSONL bridge (I²C).
 * Wiring (ESP32-C3): SDA=GPIO8, SCL=GPIO9, 115200 baud.
 *
 * Usage with NiusRobotLab_INA_monitor:
 *   Send START over Serial to begin streaming; STOP to pause.
 *   Optional: SR <Hz> to set sample rate before START.
 *
 * Standalone (no INA_monitor):
 *   You can also call ${varName}.readBusVoltage(), .readCurrent(), etc.
 *   directly in loop() to obtain measurements without JSONL output.
 */
#include <INA_Series_Sensor.h>

static ${cls} ${varName}(${args});

void setup() {
  Serial.begin(115200);
  delay(500);
  ${varName}.begin(8, 9);
}

void loop() {
  ${varName}.tick();
}
`;
}

function inoSpi229(chip, ref) {
  const varName = "sensor";
  return `/**
 * ${chip} basic example — JSONL bridge (SPI).
 * Wiring (ESP32-C3): SCK=GPIO4, MISO=GPIO5, MOSI=GPIO6, CS=GPIO7, 115200 baud.
 *
 * Usage with NiusRobotLab_INA_monitor:
 *   Send START over Serial to begin streaming; STOP to pause.
 *   Optional: SR <Hz> to set sample rate before START.
 *
 * Standalone (no INA_monitor):
 *   You can also call ${varName}.readBusVoltage(), .readCurrent(), etc.
 *   directly in loop() to obtain measurements without JSONL output.
 */
#include <INA_Series_Sensor.h>

static InaBridge229Spi ${varName}("${chip}", "${ref}");

void setup() {
  Serial.begin(115200);
  delay(500);
  ${varName}.beginSpi();
}

void loop() {
  ${varName}.tick();
}
`;
}

function inoUnknown() {
  return `/**
 * UNKNOWN stub (no sensor). Outputs zeroes. Use to verify USB serial only.
 * Send START over Serial to begin streaming; STOP to pause.
 */
#include <INA_Series_Sensor.h>

static InaBridgeUnknown sensor;

void setup() {
  Serial.begin(115200);
  delay(500);
  sensor.begin();
}

void loop() {
  sensor.tick();
}
`;
}

for (const s of SPECS) {
  const p = path.join(EXAMPLES, s.dir, s.file);
  let body;
  if (s.cls === "SPI229") body = inoSpi229("INA229", "TI INA229");
  else if (s.cls === "SPI229Q1") body = inoSpi229("INA229-Q1", "TI INA229-Q1");
  else if (s.cls === "InaBridgeUnknown") body = inoUnknown();
  else body = inoI2c(s.cls, "sensor", s.args, s.hdr);
  fs.writeFileSync(p, body, "utf8");
  console.log("wrote", p);
}
