#pragma once

#include <stdbool.h>

// Expose the state variables for the dual motors and servos
extern volatile int desired_rpm_left;
extern volatile int desired_rpm_right;
extern volatile float desired_servo_angle_1;
extern volatile float desired_servo_angle_2;

// Navigation targets
extern volatile float target_x;
extern volatile float target_y;
extern volatile float target_theta;
extern volatile bool nav_active;

void server_init(void);
