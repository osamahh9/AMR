#ifndef ENCODERS_H
#define ENCODERS_H

// Expose the calculated RPM so other files can read it
extern volatile float current_measured_rpm;

void encoders_init(void);

#endif