# INA Series Sensor — Documentation

English documentation for the **INA Series Sensor** Arduino library (JSON Lines bridge for TI INA current/power monitors and the **INA Monitor** desktop application).

| Document | Contents |
|----------|----------|
| **[USAGE.md](./USAGE.md)** | Installation, API by bridge class, serial protocol, host commands, INA Monitor workflow, calibration, tips, troubleshooting |
| **[WIRING.md](./WIRING.md)** | Power and logic levels, I²C/SPI connections, address selection, wiring tables for ESP32, Raspberry Pi Pico, Arduino Uno/Nano, and more |

---

**Library root:** `README.md` in the repository root (overview, badges, quick start).

**Version:** see `library.properties` (`version=`).

### Arduino Library Manager (third-party registration)

This layout follows the [Arduino Library Specification](https://arduino.github.io/arduino-cli/latest/library-specification/). To **submit** the library to the [Library Manager index](https://github.com/arduino/library-registry), you must:

1. Host the repo on **GitHub** (or GitLab / Bitbucket).
2. Set **`url`** in `library.properties` to the **repository home page** (update it if your GitHub user or repo name differs from the default `https://github.com/NiusRobotLab/INA_series_sensor`).
3. Create a **Git tag** (or GitHub Release) on a commit where `version` in `library.properties` matches the release (e.g. tag **`v0.2.0`** for version **`0.2.0`**).
4. Open a **pull request** against [arduino/library-registry](https://github.com/arduino/library-registry) adding your repo URL to `repositories.txt` (see their [README](https://github.com/arduino/library-registry?tab=readme-ov-file#adding-a-library-to-library-manager)).

Optional: run **[arduino-lint](https://github.com/arduino/arduino-lint)** locally before submitting.

### Push this folder to a new public GitHub repository

If Git is already initialized here (with commit and tag **`v0.2.0`**), add the remote and push:

```bash
git remote add origin https://github.com/YOUR_USER/INA_series_sensor.git
git push -u origin main
git push origin v0.2.0
```

Replace **`YOUR_USER`** with your GitHub username and adjust the repo name if needed; then update **`url=`** in **`library.properties`** to match the final repository URL.
