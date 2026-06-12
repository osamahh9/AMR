#ifndef CONTROL_H
#define CONTROL_H

#include <stdbool.h>

// Current Pose (Odometry)
extern volatile float current_x;
extern volatile float current_y;
extern volatile float current_theta;

// Navigation targets
extern volatile float target_x;
extern volatile float target_y;
extern volatile float target_theta;
extern volatile bool nav_active;

// Manual desired speeds (used by server and MQTT override)
extern volatile int desired_rpm_left;
extern volatile int desired_rpm_right;

// Tuning Parameters
extern volatile float Kp, Ki;
extern volatile float accel_limit;
extern volatile float dist_tolerance;
extern volatile float angle_tolerance;

// Control Modes
extern volatile bool manual_pid_enabled;
extern volatile bool emergency_halt_active;
extern volatile int manual_power_left;
extern volatile int manual_power_right;

void control_reset_state(void);
void control_task(void *arg);

#endif
