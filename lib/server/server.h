#pragma once

// Define the possible directions
typedef enum { 
    CMD_STOP, 
    CMD_FWD, 
    CMD_REV, 
    CMD_LEFT, 
    CMD_RIGHT 
} robot_cmd_t;

// Expose the state variables to other files
extern volatile robot_cmd_t current_cmd;
extern volatile int desired_rpm;

void server_init(void);