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

void control_task(void *arg);

#endif
