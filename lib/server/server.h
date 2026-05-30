#pragma once

// Expose the state variables for the dual motors and servos
extern volatile int desired_rpm_left;
extern volatile int desired_rpm_right;
extern volatile float desired_servo_angle_1;
extern volatile float desired_servo_angle_2;

void server_init(void);
