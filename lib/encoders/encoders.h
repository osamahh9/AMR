#ifndef ENCODERS_H
#define ENCODERS_H

// Expose the calculated RPMs for both motors
extern volatile float current_measured_rpm_left;
extern volatile float current_measured_rpm_right;

void encoders_init(void);

#endif
