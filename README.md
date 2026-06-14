# AMR PRO OS (Autonomous Mobile Robot) - ESP32-S3

A professional-grade, FreeRTOS-based firmware for an Autonomous Mobile Robot built on the ESP32-S3 platform. This project features high-performance differential drive kinematics, real-time odometry, an advanced HMI dashboard with a "Digital Twin" visualizer, and robust system-wide stability optimizations.

---

## 🚀 Key Features

*   **Virtual Joystick Control**
    *   **Fluid Motion:** Simultaneous speed and steering control with an auto-reset "dead-man's switch."
    *   **Dual-Mode Backend:** Toggle between **PID Mode** (closed-loop speed control in RPM) and **RAW Mode** (open-loop PWM thrust percentage).
    *   **Ear Meters:** Symmetrical, center-fill horizontal meters flanking the joystick for intuitive thrust visualization.

*   **End-Effector HMI (Digital Twin)**
    *   **Kinematic Visualizer:** Real-time 2D canvas simulating the robotic claw's **Wrist Roll** and **Claw Aperture**.
    *   **Industrial Design:** Realistic pivot-and-hook jaw animation mirroring professional automation tools.
    *   **Motion Fluidity:** Quadratic **LERP (Linear Interpolation)** motion engine for buttery-smooth, cinematic servo travel.
    *   **Precision Center:** Recalibrated so 45° on the slider points the gripper perfectly upright.

*   **Advanced Data Visualization**
    *   **Adaptive Viewport Map:** Auto-zooming and auto-panning 2D spatial grid that keeps the robot and its path history perfectly framed.
    *   **Autoscaling Oscilloscopes:** Real-time line charts for Wheel RPMs and Ultrasonic Distance that automatically adjust their Y-axis based on data peaks.

*   **System & Network Stability**
    *   **Priority Optimization:** Motor control (`10`) and safety sensors (`7`) run at elevated priorities to ensure real-time responsiveness.
    *   **Active Hardware Braking:** Fixed "Stuck Motor" bugs by forcing H-bridge direction pins LOW at zero-speed and implementing PWM clamping.
    *   **Network Guarding:** Throttled joystick updates (1% change threshold) and sequential polling to prevent socket exhaustion (Error 23).
    *   **Emergency HALT:** Dedicated hardware-level lockdown that bypasses all control loops to stop the robot instantly.

---

## 🛠 Hardware Architecture

*   **Microcontroller**: ESP32-S3 DevKitC-1 (16MB Flash, 8MB PSRAM)
*   **Motors**: 2x DC Motors with encoders (L298N or equivalent H-Bridge)
*   **Sensors**: 1x HC-SR04 Ultrasonic Sensor
*   **Actuators**: 3x Standard 5V Servos (Servo 3 is the hardware-inverse of Servo 1)

### Pin Configuration

| Component | Pin (GPIO) | Subsystem / Function |
| :--- | :--- | :--- |
| **Left Motor (PWM)** | 14 (ENA) | `LEDC_TIMER_1`, `CHANNEL_0` |
| **Left Motor (Dir)** | 15 (IN1), 21 (IN2) | Hardware Braking Supported |
| **Right Motor (PWM)** | 16 (ENB) | `LEDC_TIMER_1`, `CHANNEL_1` |
| **Right Motor (Dir)** | 17 (IN3), 18 (IN4) | Hardware Braking Supported |
| **Left Encoder** | 9 | `PCNT` Unit 0 |
| **Right Encoder** | 13 | `PCNT` Unit 1 |
| **Ultrasonic Trig** | 7 | Trigger Output |
| **Ultrasonic Echo** | 8 | Echo Input |
| **Servo 1 (Wrist)** | 10 | `LEDC_TIMER_0`, `CHANNEL_2` |
| **Servo 2 (Claw)** | 11 | `LEDC_TIMER_0`, `CHANNEL_3` |
| **Servo 3 (Inverse)**| 12 | Slave to Servo 1 |

---

## ⚙️ Usage & Controls

Access the dashboard via the ESP32's IP address (e.g., `http://192.168.137.99`).

### 🎮 Manual Drive
Drag the joystick to move. The background will glow **Deep Blue** during motion. Release the stick to trigger active braking.

### 🦾 End-Effector HMI (Advanced Section)
*   **Wrist Roll:** Tilt the gripper (45° is vertical).
*   **Claw Aperture:** Adjust the pincer opening (0° is Open, 130° is fully Clamped).
*   **Motion Fluidity:** Tweak the % slider to choose between slow cinematic glides or fast mechanical snaps.

### 🛠 System Tuning
Open the **ADVANCED** toggle to access:
*   **PID Gains:** Live-adjust `Kp` and `Ki` to match your floor surface.
*   **Speed Multiplier:** Set a global "governor" (0.1x to 2.0x) to cap the maximum joystick output.
*   **Obstacle Stop:** Change the distance threshold (5cm - 100cm) for the auto-safety brake.

---

## 📡 MQTT Telemetry (`cell/amr/status`)

Publishes a rich JSON payload every second for remote fleet monitoring:
```json
{
  "robot": "amr",
  "state": "IDLE/MOVING",
  "x": 120.5, "y": 45.2, "th": 1.57,
  "dist": 25,
  "rpm_l": 150.2, "rpm_r": 149.8,
  "mqtt": true,
  "cmd": "LAST_MQTT_COMMAND"
}
```

---

## License
MIT License. Optimized for high-precision robotics and academic demonstrations.
