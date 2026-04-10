/**
 * Rewrites each examples subfolder ino to use INA_Series_Sensor library classes.
 * Run: node scripts/wire-examples-library.mjs
 */
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const EXAMPLES = path.join(__dirname, "..", "examples");

const SPECS = [
  { dir: "ina219_bridge", file: "ina219_bridge.ino", cls: "InaBridge219", args: '"INA219", 0x40', hdr: "INA219" },
  { dir: "ina220_bridge", file: "ina220_bridge.ino", cls: "InaBridge219", args: '"INA220", 0x40', hdr: "INA220" },
  { dir: "ina220_q1_bridge", file: "ina220_q1_bridge.ino", cls: "InaBridge219", args: '"INA220-Q1", 0x40', hdr: "INA220-Q1" },
  { dir: "ina226_bridge", file: "ina226_bridge.ino", cls: "InaBridge226", args: '"INA226", 0x40', hdr: "INA226" },
  { dir: "ina226_q1_bridge", file: "ina226_q1_bridge.ino", cls: "InaBridge226", args: '"INA226-Q1", 0x40', hdr: "INA226-Q1" },
  { dir: "ina230_bridge", file: "ina230_bridge.ino", cls: "InaBridge226", args: '"INA230", 0x40, "TI SLYSF02 (INA230)"', hdr: "INA230" },
  { dir: "ina231_bridge", file: "ina231_bridge.ino", cls: "InaBridge226", args: '"INA231", 0x40, "TI SLYSF02 (INA231)"', hdr: "INA231" },
  { dir: "ina232_bridge", file: "ina232_bridge.ino", cls: "InaBridge226", args: '"INA232", 0x40, "TI SLYSF02 (INA232)"', hdr: "INA232" },
  { dir: "ina233_bridge", file: "ina233_bridge.ino", cls: "InaBridge226", args: '"INA233", 0x40, "TI SLYSF02 (INA233)"', hdr: "INA233" },
  { dir: "ina234_bridge", file: "ina234_bridge.ino", cls: "InaBridge226", args: '"INA234", 0x40, "TI SLYSF02 (INA234)"', hdr: "INA234" },
  { dir: "ina236_bridge", file: "ina236_bridge.ino", cls: "InaBridge226", args: '"INA236", 0x40, "TI SLYSF02 (INA236)"', hdr: "INA236" },
  { dir: "ina228_bridge", file: "ina228_bridge.ino", cls: "InaBridge228", args: '"INA228", 0x40', hdr: "INA228" },
  { dir: "ina228_q1_bridge", file: "ina228_q1_bridge.ino", cls: "InaBridge228", args: '"INA228-Q1", 0x40, "TI INA228-Q1"', hdr: "INA228-Q1" },
  { dir: "ina237_bridge", file: "ina237_bridge.ino", cls: "InaBridge228", args: '"INA237", 0x40, "TI INA237"', hdr: "INA237" },
  { dir: "ina237_q1_bridge", file: "ina237_q1_bridge.ino", cls: "InaBridge228", args: '"INA237-Q1", 0x40, "TI INA237-Q1"', hdr: "INA237-Q1" },
  { dir: "ina238_bridge", file: "ina238_bridge.ino", cls: "InaBridge228", args: '"INA238", 0x40, "TI INA238"', hdr: "INA238" },
  { dir: "ina238_q1_bridge", file: "ina238_q1_bridge.ino", cls: "InaBridge228", args: '"INA238-Q1", 0x40, "TI INA238-Q1"', hdr: "INA238-Q1" },
  { dir: "ina239_bridge", file: "ina239_bridge.ino", cls: "InaBridge228", args: '"INA239", 0x40, "TI INA239"', hdr: "INA239" },
  { dir: "ina239_q1_bridge", file: "ina239_q1_bridge.ino", cls: "InaBridge228", args: '"INA239-Q1", 0x40, "TI INA239-Q1"', hdr: "INA239-Q1" },
  { dir: "ina740x_bridge", file: "ina740x_bridge.ino", cls: "InaBridge228", args: '"INA740X", 0x40, "TI INA740X"', hdr: "INA740X" },
  { dir: "ina3221_bridge", file: "ina3221_bridge.ino", cls: "InaBridge3221", args: '"INA3221", 0x40', hdr: "INA3221" },
  { dir: "ina3221_q1_bridge", file: "ina3221_q1_bridge.ino", cls: "InaBridge3221", args: '"INA3221-Q1", 0x40', hdr: "INA3221-Q1" },
  { dir: "ina229_bridge", file: "ina229_bridge.ino", cls: "SPI229", args: "", hdr: "INA229" },
  { dir: "ina229_q1_bridge", file: "ina229_q1_bridge.ino", cls: "SPI229Q1", args: "", hdr: "INA229-Q1" },
  { dir: "ina2227_bridge", file: "ina2227_bridge.ino", cls: "InaBridgeCh1", args: '"INA2227", "INA2227 bridge ready (CH1)", 0x40', hdr: "INA2227" },
  { dir: "ina4230_bridge", file: "ina4230_bridge.ino", cls: "InaBridgeCh1", args: '"INA4230", "INA4230 bridge ready (CH1)", 0x40', hdr: "INA4230" },
  { dir: "ina4235_bridge", file: "ina4235_bridge.ino", cls: "InaBridgeCh1", args: '"INA4235", "INA4235 bridge ready (CH1)", 0x40', hdr: "INA4235" },
  { dir: "unknown_bridge", file: "unknown_bridge.ino", cls: "InaBridgeUnknown", args: "", hdr: "UNKNOWN" }
];

function inoI2c(cls, varName, args, title, fileName) {
  return `/**
 * @file ${fileName}
 * @brief ${title} — JSONL bridge using library class (see INA_Series_Sensor).
 * ESP32-C3 SuperMini: GPIO8=SDA, GPIO9=SCL, 3V3/GND. USB 115200.
 */
#include <INA_Series_Sensor.h>

static ${cls} ${varName}(${args});

void setup() {
  Serial.begin(115200);
  delay(100);
  ${varName}.beginI2c(8, 9);
}

void loop() {
  ${varName}.tick();
}
`;
}

function inoSpi229(title, chip, ref) {
  return `/**
 * @file ${title}_bridge.ino
 * @brief ${chip} — SPI JSONL bridge (library InaBridge229Spi).
 * ESP32-C3: GPIO4=SCK, 5=MISO, 6=MOSI, 7=CS. USB 115200.
 */
#include <INA_Series_Sensor.h>

static InaBridge229Spi g_bridge("${chip}", "${ref}");

void setup() {
  Serial.begin(115200);
  delay(100);
  g_bridge.beginSpi();
}

void loop() {
  g_bridge.tick();
}
`;
}

function inoUnknown() {
  return `/**
 * @file unknown_bridge.ino
 * @brief UNKNOWN chip placeholder (InaBridgeUnknown).
 */
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
`;
}

for (const s of SPECS) {
  const p = path.join(EXAMPLES, s.dir, s.file);
  let body;
  if (s.cls === "SPI229") body = inoSpi229("ina229", "INA229", "TI INA229");
  else if (s.cls === "SPI229Q1") body = inoSpi229("ina229_q1", "INA229-Q1", "TI INA229-Q1");
  else if (s.cls === "InaBridgeUnknown") body = inoUnknown();
  else body = inoI2c(s.cls, "g_bridge", s.args, s.hdr, s.file);
  fs.writeFileSync(p, body, "utf8");
  console.log("wrote", p);
}
