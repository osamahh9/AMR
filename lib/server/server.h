#pragma once

// Expose the state variables for the dual motors
extern volatile int desired_rpm_left;
extern volatile int desired_rpm_right;

void server_init(void);
