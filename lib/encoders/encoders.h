#ifndef ENCODERS_H
#define ENCODERS_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"

// Expose the calculated RPMs for both motors
extern volatile float current_measured_rpm_left;
extern volatile float current_measured_rpm_right;

// Expose cumulative pulse counts for odometry
extern volatile int64_t total_pulses_left;
extern volatile int64_t total_pulses_right;

// Spinlock for atomic access to 64-bit counters
extern portMUX_TYPE pulse_mux;

void encoders_init(void);

#endif
