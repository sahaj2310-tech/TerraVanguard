# TerraVanguard

![License: Source-Available (No-Modify)](https://img.shields.io/badge/License-Source--Available%20(No--Modify)-blue)
![Platform: ESP32-S3](https://img.shields.io/badge/Platform-ESP32--S3-orange)
![Framework: Arduino](https://img.shields.io/badge/Framework-Arduino-00979D?logo=arduino&logoColor=white)
![Language: C++](https://img.shields.io/badge/Language-C%2B%2B-00599C?logo=cplusplus&logoColor=white)
![Top language](https://img.shields.io/github/languages/top/sahaj2310-tech/TerraVanguard)
![Status: Complete](https://img.shields.io/badge/Status-Complete-brightgreen)
![PRs: not accepted](https://img.shields.io/badge/PRs-not%20accepted-red)

![Stars](https://img.shields.io/github/stars/sahaj2310-tech/TerraVanguard?style=social)
![Forks](https://img.shields.io/github/forks/sahaj2310-tech/TerraVanguard?style=social)
![Last commit](https://img.shields.io/github/last-commit/sahaj2310-tech/TerraVanguard)
![Repo size](https://img.shields.io/github/repo-size/sahaj2310-tech/TerraVanguard)
![Code size](https://img.shields.io/github/languages/code-size/sahaj2310-tech/TerraVanguard)

![Sensors: MPU9250 | DHT11 | HC-SR04](https://img.shields.io/badge/Sensors-MPU9250%20%7C%20DHT11%20%7C%20HC--SR04-informational)
![Motor: L298N](https://img.shields.io/badge/Motor-L298N-informational)
![Camera: OV2640](https://img.shields.io/badge/Camera-OV2640-informational)

An autonomous terrain-exploration rover built on the **ESP32-S3**. TerraVanguard
combines differential drive, multi-sensor obstacle avoidance, a gyro-stabilized
camera gimbal, environmental sensing, and a live Wi-Fi video stream — all running
on a single microcontroller.

> Originally developed as an academic robotics project. The firmware is published
> here for reference and demonstration. See [LICENSE](LICENSE) for usage terms.

---

## Features

- **Differential drive** — two DC gear motors driven by an L298N H-bridge, with
  forward / reverse / turn primitives.
- **Obstacle avoidance** — three HC-SR04 ultrasonic sensors (left / center / right)
  feeding a simple reactive navigation loop.
- **Gyro-stabilized camera gimbal** — an MPU9250 9-axis IMU drives two servos
  (pan / tilt) through a low-pass filter to keep the camera level.
- **Environmental sensing** — a DHT11 reports temperature and humidity.
- **Live video** — the on-board camera streams MJPEG over HTTP to any browser on
  the same network.
- **Resilience** — Wi-Fi auto-reconnect, watchdog feeding, and `millis()` rollover
  handling for long-running operation.

## Repository layout

```
terravanguard/
├── firmware/
│   ├── terravanguard/            # Main build: full system + camera web stream
│   └── terravanguard_nocam/      # Lightweight build: drive + sensors, no camera
├── tests/                        # Standalone sketches to validate each subsystem
│   ├── test_dht/                 # DHT11 temperature / humidity
│   ├── test_servo_dual/          # Two-servo sweep
│   ├── test_mpu9250/             # IMU read-out
│   ├── test_mpu_servo/           # IMU → servo gimbal stabilization
│   ├── test_motor_l298n/         # L298N motor driver
│   ├── test_ultrasonic/          # Triple ultrasonic ranging
│   └── test_camera/              # Camera MJPEG web server
├── docs/
│   ├── HARDWARE.md               # Wiring, pin map, and bill of materials
│   └── images/                   # Pinout and wiring reference images
├── LICENSE
└── README.md
```

Each `.ino` lives in a folder of the same name, as required by the Arduino IDE.

## Quick start

1. **Install the toolchain**
   - [Arduino IDE](https://www.arduino.cc/en/software) (or arduino-cli)
   - ESP32 board support — add the Espressif board package and select an
     **ESP32-S3** board with PSRAM enabled.

2. **Install libraries** (Library Manager)
   - `MPU9250_asukiaaa`
   - `ESP32Servo`
   - `DHT sensor library` (Adafruit)
   - The ESP32 camera driver ships with the ESP32 board package.

3. **Wire the hardware** — follow [docs/HARDWARE.md](docs/HARDWARE.md).

4. **Set your Wi-Fi** — open `firmware/terravanguard/terravanguard.ino` and replace
   the placeholders:
   ```cpp
   const char* ssid     = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   ```

5. **Flash** — open the sketch, select your port, and upload.

6. **View the stream** — open the Serial Monitor at `115200` baud to read the
   device IP, then browse to `http://<device-ip>/`.

## Builds

| Build | Folder | Use |
| --- | --- | --- |
| Full | `firmware/terravanguard` | Drive + all sensors + live camera stream |
| No-camera | `firmware/terravanguard_nocam` | Lower resource use; drive + sensors only |

## License

This project is released under a custom **view-only, no-modification** license.
You may read and study the source, but you may not modify, redistribute, or use it
commercially without written permission. See [LICENSE](LICENSE) for the full terms.
