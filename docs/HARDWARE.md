# Hardware Guide

This document describes the components, wiring, and pin assignments for
TerraVanguard. All GPIO numbers refer to the **ESP32-S3** microcontroller.

## Bill of materials

| Qty | Component | Notes |
| --- | --- | --- |
| 1 | ESP32-S3 board with on-board camera | PSRAM strongly recommended for streaming |
| 1 | L298N dual H-bridge motor driver | Drives the two DC motors |
| 2 | DC gear motors + wheels | Differential (tank-style) drive |
| 1 | MPU9250 9-axis IMU | I²C, gyro-stabilized gimbal |
| 2 | SG90 / MG90 servos | Pan and tilt axes |
| 3 | HC-SR04 ultrasonic sensors | Left / center / right obstacle detection |
| 1 | DHT11 sensor | Temperature and humidity |
| 1 | Battery pack | Sized for motors + logic (use a separate motor supply) |

## Pin map

### Motor driver (L298N)

| Signal | GPIO | Motor |
| --- | --- | --- |
| IN1 | 0  | A |
| IN2 | 45 | A |
| IN3 | 48 | B |
| IN4 | 47 | B |

> The no-camera build uses GPIO 14 for IN1 instead of GPIO 0, since GPIO 0 is a
> strapping pin. Prefer GPIO 14 if you encounter boot issues.

### MPU9250 IMU (I²C)

| Signal | GPIO |
| --- | --- |
| SDA | 2 |
| SCL | 1 |

Bus runs at 400 kHz.

### Servos (camera gimbal)

| Axis | GPIO |
| --- | --- |
| Servo X (pan)  | 39 |
| Servo Y (tilt) | 40 |

Both servos initialize to 90°. The tilt axis is constrained to 0–95°.

### DHT11

| Signal | GPIO |
| --- | --- |
| Data | 21 |

### Ultrasonic sensors (HC-SR04)

| Sensor | TRIG | ECHO |
| --- | --- | --- |
| 1 | 3  | 19 |
| 2 | 35 | 36 |
| 3 | 37 | 38 |

### Camera

| Signal | GPIO | Signal | GPIO |
| --- | --- | --- | --- |
| PWDN  | -1 | RESET | -1 |
| XCLK  | 15 | PCLK  | 13 |
| SIOD  | 4  | SIOC  | 5  |
| VSYNC | 6  | HREF  | 7  |
| Y2    | 11 | Y3    | 9  |
| Y4    | 8  | Y5    | 10 |
| Y6    | 12 | Y7    | 18 |
| Y8    | 17 | Y9    | 16 |

## Behavior notes

- **Obstacle avoidance**: if any ultrasonic sensor reads below ~20 cm, the rover
  turns right; with a clear path (>30 cm) it drives forward; otherwise it stops.
- **Gimbal stabilization**: gyro rates are mapped to servo angles and smoothed with
  a low-pass filter (`angle = 0.9 * angle + 0.1 * target`).
- **Power**: drive the motors from a dedicated supply and share a common ground
  with the ESP32-S3. Do not power motors from the board's regulator.

## Reference images

See [images/](images/) for the ESP32-S3 pinout, the FTDI programming wiring, and
the camera pan/tilt assembly.
