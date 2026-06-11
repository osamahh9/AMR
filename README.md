# AMR (Autonomous Mobile Robot) - ESP32-S3

A comprehensive, FreeRTOS-based firmware for an Autonomous Mobile Robot built on the ESP32-S3 platform. This project features differential drive kinematics, real-time odometry, autonomous coordinate-based navigation, manual drive control, live system tuning, and full MQTT integration.

---

## 🚀 Features

*   **Dual-Mode Control**
    *   **Autonomous Navigation:** Coordinate-based "Go-to-Goal" with a 3-stage state machine (Rotate -> Drive -> Rotate).
    *   **Manual Drive:** Real-time D-pad control via the web dashboard with adjustable speed scaling.
    *   **Mission Flags:** One-touch navigation to predefined locations (HOME, ARM STATION, BOX SLOTS).
*   **Live System Tuning (No Re-flashing Needed)**
    *   **PID Tuning:** Adjust `Kp` and `Ki` gains at runtime for different surfaces or loads.
    *   **Safety Tuning:** Adjustable obstacle detection threshold (5cm - 100cm).
    *   **Nav Precision:** Tune distance (mm) and angle (rad) tolerances live for faster or more precise missions.
*   **Precision Odometry & Telemetry**
    *   50Hz control loop tracking `X`, `Y`, and `Heading` via hardware pulse counters (PCNT).
    *   **Expanded MQTT Telemetry:** Real-time status updates including Pose, RPMs, Distance, and last received Command.
*   **Advanced Monitoring Dashboard**
    *   Embedded HTML/JS UI with a toggleable **ADVANCED** mode.
    *   Live telemetry stream: Position, Heading, Obstacle Status, and Motor RPMs.
    *   MQTT Command Tracker: Monitor raw commands received from the broker.

---

## 🛠 Hardware Architecture

*   **Microcontroller**: ESP32-S3 DevKitC-1 (16MB Flash)
*   **Motors**: 2x DC Motors with encoders (connected via motor driver, e.g., L298N)
*   **Sensors**: 1x HC-SR04 Ultrasonic Sensor
*   **Actuators**: Up to 3x standard 5V Servos (e.g., SG90, MG996R)

### Pin Configuration

| Component | Pin (GPIO) | Subsystem / Function |
| :--- | :--- | :--- |
| **Left Motor (PWM)** | 14 (ENA) | `LEDC_TIMER_1`, `CHANNEL_0` |
| **Left Motor (Dir)** | 15 (IN1), 21 (IN2) | Standard GPIO Output |
| **Right Motor (PWM)** | 16 (ENB) | `LEDC_TIMER_1`, `CHANNEL_1` |
| **Right Motor (Dir)** | 17 (IN3), 18 (IN4) | Standard GPIO Output |
| **Left Encoder** | 9 | `PCNT` Unit 0 |
| **Right Encoder** | 13 | `PCNT` Unit 1 |
| **Ultrasonic Trig** | 7 | Standard GPIO Output |
| **Ultrasonic Echo** | 8 | Standard GPIO Input |
| **Servo 1** | 10 | `LEDC_TIMER_0`, `CHANNEL_2` |
| **Servo 2** | 11 | `LEDC_TIMER_0`, `CHANNEL_3` |
| **Servo 3 (Inverse)**| 12 | `LEDC_TIMER_0`, `CHANNEL_4` |

---

## ⚙️ Usage & Controls

Once the ESP32-S3 boots and connects to WiFi, open its IP address in a web browser (e.g., `http://192.168.1.100`).

### 🎮 Manual Drive
Use the **D-pad** to move the robot.
*   **Forward/Backward/Turn**: Press and hold to move, release to stop.
*   **Stop Button**: Immediately cuts power to motors.

### 🚩 Mission Flags
Click any location button (e.g., **RED BOX**, **ARM STATION**) to trigger a predefined autonomous mission.

### 🛠 Advanced Mode (Toggle)
Enable the **ADVANCED** switch in the header to reveal deep telemetry and tuning settings:
*   **Manual Speed Multiplier:** Scale the D-pad speed from 0.2x to 2.0x.
*   **System Tuning:**
    *   **PID Gains:** Tweak `Kp` and `Ki` to stop oscillations or improve responsiveness.
    *   **Obstacle Stop:** Change how close the robot gets to objects before stopping.
    *   **Nav Precision:** Adjust tolerances to prevent the robot from "jiggling" at the end of a mission.
*   **MQTT Monitor:** View the last raw JSON command received from the MQTT broker.

---

## 📡 MQTT Integration

The robot communicates with a central broker (default: `192.168.137.1`) for remote monitoring and fleet control.

### Status Telemetry (`cell/amr/status`)
Publishes a JSON payload every second:
```json
{
  "robot": "amr",
  "state": "IDLE/MOVING",
  "x": 120.5, "y": 45.2, "th": 90.0,
  "dist": 25,
  "rpm_l": 150.2, "rpm_r": 149.8,
  "mqtt": true
}
```

### Remote Commands (`cell/amr/cmd`)
Accepts JSON commands such as:
*   `{"cmd": "PICK_FILLED_BOX", "jobId": "123", "destination": "ARM_STATION"}`
*   `{"cmd": "STOP", "jobId": "123"}`

---

## License
MIT License. Feel free to use, modify, and distribute for your own robotic projects!
