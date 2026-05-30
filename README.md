# AMR (Autonomous Mobile Robot) - ESP32-S3

A comprehensive, FreeRTOS-based firmware for an Autonomous Mobile Robot built on the ESP32-S3 platform. This project features differential drive kinematics, real-time odometry, autonomous coordinate-based navigation, obstacle detection, and a built-in web dashboard for monitoring and control.

---

## 🚀 Features

*   **Autonomous Navigation (Go-to-Goal)**
    *   Coordinate-based navigation to target `(X, Y, Theta)`.
    *   State-machine controller: Rotate to face goal -> Drive to goal -> Rotate to final heading.
    *   Smooth acceleration/deceleration ramps to prevent wheel slip and overshoot.
*   **Precision Odometry**
    *   Differential drive kinematics tracking `X`, `Y`, and `Heading`.
    *   High-frequency 50Hz (20ms) control loop.
    *   Atomic 64-bit pulse counting using ESP32 hardware pulse counters (PCNT) and FreeRTOS spinlocks.
*   **Obstacle Detection & Safety**
    *   Integrated HC-SR04 ultrasonic sensor running at 20Hz.
    *   Real-time obstacle flagging (threshold: < 20cm).
*   **Synchronized Servo Control**
    *   Independent control for two servos (e.g., for a sensor turret or robotic arm).
    *   Third inverse-coupled servo channel for synchronized opposite movement.
    *   Hardware PWM via ESP32 LEDC (Timer 1, 50Hz).
*   **Real-time Web Dashboard**
    *   Built-in asynchronous HTTP server hosting an embedded HTML/JS dashboard.
    *   Live telemetry: (X, Y) position, heading, obstacle status, and motor RPMs.
    *   Interactive controls: Target coordinate input and live servo sliders.

---

## 🛠 Hardware Architecture

*   **Microcontroller**: ESP32-S3 DevKitC-1 (16MB Flash)
*   **Motors**: 2x DC Motors with encoders (connected via motor driver, e.g., L298N)
*   **Sensors**: 1x HC-SR04 Ultrasonic Sensor
*   **Actuators**: Up to 3x standard 5V Servos (e.g., SG90, MG996R)

### Pin Configuration

| Component | Pin (GPIO) | Subsystem / Function |
| :--- | :--- | :--- |
| **Left Motor (PWM)** | 19 (ENA) | `LEDC_TIMER_0`, `CHANNEL_0` |
| **Left Motor (Dir)** | 20 (IN1), 21 (IN2) | Standard GPIO Output |
| **Right Motor (PWM)** | 16 (ENB) | `LEDC_TIMER_0`, `CHANNEL_1` |
| **Right Motor (Dir)** | 17 (IN3), 18 (IN4) | Standard GPIO Output |
| **Left Encoder** | 4 | `PCNT` Unit 0 |
| **Right Encoder** | 5 | `PCNT` Unit 1 |
| **Ultrasonic Trig** | 1 | Standard GPIO Output |
| **Ultrasonic Echo** | 2 | Standard GPIO Input |
| **Servo 1** | 6 | `LEDC_TIMER_1`, `CHANNEL_2` |
| **Servo 2** | 7 | `LEDC_TIMER_1`, `CHANNEL_3` |
| **Servo 3 (Inverse)**| 8 | `LEDC_TIMER_1`, `CHANNEL_4` |

---

## 📁 Software Structure

The project is modularized into dedicated FreeRTOS tasks to ensure non-blocking, real-time performance.

*   `src/main.c`: System entry point and FreeRTOS task initialization.
*   `lib/control`: The brain of the robot. Handles the 50Hz PID loop, odometry calculations, and the 3-stage navigation state machine.
*   `lib/encoders`: Interfaces with the ESP32 Pulse Counter (PCNT) peripheral for high-speed, interrupt-free encoder reading.
*   `lib/motors`: Low-level abstraction for setting motor direction and PWM duty cycles.
*   `lib/object`: Manages the 20Hz ultrasonic ping cycle and handles the hardware PWM generation for the servos.
*   `lib/server`: The asynchronous HTTP server that hosts the UI and handles REST API calls (`/nav`, `/servo`, `/status`, `/drive`).
*   `lib/wifi`: (Placeholder/Implicit) Handles connection to the local network or hosting an AP.

---

## ⚙️ Setup and Installation

This project is built using [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/) via CMake (or PlatformIO).

### Prerequisites
1. Install [ESP-IDF v5.x+](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html).
2. Ensure you have the `esp-idf-lib` components installed (specifically the ultrasonic and servo helpers).

### Build & Flash
Open a terminal in the project root:

```bash
# Set target to ESP32-S3
idf.py set-target esp32s3

# Build the project
idf.py build

# Flash to the device and open the serial monitor
idf.py -p /dev/ttyUSB0 flash monitor
```
*(Replace `/dev/ttyUSB0` with your actual serial port).*

---

## 🎮 Usage: The Web Dashboard

Once the ESP32-S3 boots and connects to WiFi, it will print its IP address to the serial monitor.
Open that IP address in any modern web browser (e.g., `http://192.168.1.100`).

### Navigation Control
1. **Target X (mm)**: Forward/backward distance relative to the starting point.
2. **Target Y (mm)**: Left/right distance relative to the starting point.
3. **Target Angle (deg)**: The final direction the robot should face after reaching the destination.
4. Click **GO TO GOAL**. The robot will execute the turn-drive-turn sequence automatically.

### Servo Control
Adjust the horizontal sliders for **Servo 1** and **Servo 2**.
*   *Note: Servo 3 (GPIO 8) is hardcoded to automatically mirror Servo 1 inversely (180 deg - angle).*

### Emergency Stop
Click the red **EMERGENCY STOP** button to immediately halt the navigation sequence and cut power to the motors.

---

## 🔧 Tuning Physical Constants

For the odometry and navigation to be accurate, the physical measurements of the robot must be exact. Update these constants at the top of `lib/control/control.c`:

```c
#define WHEEL_DIAMETER_MM 65.0   // Exact diameter of the wheels
#define WHEEL_BASE_MM     150.0  // Exact center-to-center distance between wheels
#define PULSES_PER_REV    20.0   // Encoder slots per revolution
```

If the robot overshoots or oscillates, tune the following gains in the same file:
*   `STEER_GAIN`: Adjusts how aggressively the robot corrects its course while driving.
*   `TURN_GAIN`: Adjusts the speed of in-place rotations.
*   `ACCEL_LIMIT`: Adjusts the acceleration/deceleration smoothness.

---

## License
MIT License. Feel free to use, modify, and distribute for your own robotic projects!
