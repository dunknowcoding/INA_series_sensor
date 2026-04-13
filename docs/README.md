# INA Series Sensor — Documentation

English documentation for the **INA Series Sensor** Arduino library: **JSON Lines** over USB serial for Texas Instruments **INA** current/power monitors, designed for the **NiusRobotLab_INA_monitor** desktop host (same wire protocol as the **INA Monitor** user interface).

| Document | Contents |
|----------|----------|
| **[USAGE.md](./USAGE.md)** | Installation, API by bridge class, serial protocol, host commands, **NiusRobotLab_INA_monitor** workflow, calibration, tips, troubleshooting |
| **[WIRING.md](./WIRING.md)** | Power and logic levels, I²C/SPI connections, address selection, wiring tables for ESP32, Raspberry Pi Pico, Arduino Uno/Nano, and more |

---

## Canonical URLs

| Item | URL / note |
|------|------------|
| **This library (source + Arduino Library Manager homepage)** | **`https://github.com/dunknowcoding/INA_series_sensor`** — set as **`url=`** in **`library.properties`** so the Arduino Library Manager index can list releases and updates. |
| **NiusRobotLab_INA_monitor (desktop application)** | *Not published on GitHub yet.* When the maintainer publishes the repository, add its link here and in the root **`README.md`** under **NiusRobotLab_INA_monitor (host UI)**. |

---

**Library overview:** `README.md` in the repository root (badges, quick start, protocol summary).

**Version:** see `library.properties` (`version=`).

### Arduino Library Manager (third-party registration)

This layout follows the [Arduino Library Specification](https://arduino.github.io/arduino-cli/latest/library-specification/). To **submit** the library to the [Library Manager index](https://github.com/arduino/library-registry), you must:

1. Host the repo on **GitHub** (or GitLab / Bitbucket).
2. Keep **`url=`** in **`library.properties`** equal to the **public repository home page** (default: **`https://github.com/dunknowcoding/INA_series_sensor`**). The indexer uses this URL to discover new **tags/releases**.
3. Create a **Git tag** (or GitHub Release) on a commit where `version` in `library.properties` matches the release (e.g. tag **`v0.2.5`** for version **`0.2.5`**).
4. Open a **pull request** against [arduino/library-registry](https://github.com/arduino/library-registry) adding your repo URL to `repositories.txt` (see their [README](https://github.com/arduino/library-registry?tab=readme-ov-file#adding-a-library-to-library-manager)).

Optional: run **[arduino-lint](https://github.com/arduino/arduino-lint)** locally before submitting.

### Push this folder to GitHub

If Git is already initialized (with commit and tag **`v0.2.5`**), add the remote and push:

```bash
git remote add origin https://github.com/dunknowcoding/INA_series_sensor.git
git push -u origin main
git push origin v0.2.5
```

If your GitHub **username** or **repository name** differs, replace the URL above and update **`url=`** in **`library.properties`** to match **exactly**.
