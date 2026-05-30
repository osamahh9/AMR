#ifndef CONTROL_H
#define CONTROL_H

// Current Pose (Odometry)
extern volatile float current_x;
extern volatile float current_y;
extern volatile float current_theta;

void control_task(void *arg);

#endif
