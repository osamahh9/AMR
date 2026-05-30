#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "motors.h"
#include "server.h"
#include "encoders.h"
#include "esp_log.h"
#include "control.h"
#include <math.h>

static const char *TAG = "Control";

// --- Robot Physical Constants ---
#define WHEEL_DIAMETER_MM 65.0
#define WHEEL_BASE_MM     150.0 
#define PULSES_PER_REV    20.0
#define MM_PER_PULSE      (M_PI * WHEEL_DIAMETER_MM / PULSES_PER_REV)

// Navigation Configuration
#define NAV_MAX_RPM       200.0
#define NAV_MIN_RPM       40.0
#define DIST_TOLERANCE_MM 20.0
#define ANGLE_TOLERANCE_RAD 0.05
#define DECEL_DIST_MM     300.0
#define STEER_GAIN        25.0f
#define TURN_GAIN         80.0f
#define ACCEL_LIMIT       2.0f  // Reduced to 2.0 to maintain 100 RPM/s at 50Hz frequency

typedef enum {
    NAV_IDLE,
    NAV_ROTATE_TO_GOAL,
    NAV_DRIVE_TO_GOAL,
    NAV_ROTATE_TO_FINAL
} nav_state_t;

static nav_state_t current_nav_state = NAV_IDLE;

// PID Gains for Speed
static float Kp = 0.15, Ki = 0.05;
static float integral_left = 0, integral_right = 0;
static int power_left_last = 0, power_right_last = 0;

// Acceleration ramp state
static float commanded_speed = 0;

// Odometry state
volatile float current_x = 0, current_y = 0, current_theta = 0;
static int64_t last_pulses_left = 0;
static int64_t last_pulses_right = 0;

static int run_pid(float target_rpm, float current_rpm, float *integral, int *last_power) {
    if (target_rpm == 0) { *integral = 0; return 0; }
    float abs_target = fabsf(target_rpm);
    float error = abs_target - current_rpm;
    if (*last_power < 100 && *last_power > 0) *integral += error * 0.02; // Adjusted for 20ms
    float control_signal = (Kp * error) + (Ki * (*integral));
    float feed_forward = (abs_target / 600.0) * 50.0; 
    int power = (int)(feed_forward + control_signal);
    if (power > 100) power = 100;
    if (power < 0) power = 0;
    *last_power = power;
    return (target_rpm < 0) ? -power : power;
}

void update_odometry() {
    int64_t snap_left, snap_right;
    portENTER_CRITICAL(&pulse_mux);
    snap_left = total_pulses_left;
    snap_right = total_pulses_right;
    portEXIT_CRITICAL(&pulse_mux);

    int64_t d_left = snap_left - last_pulses_left;
    int64_t d_right = snap_right - last_pulses_right;
    last_pulses_left = snap_left;
    last_pulses_right = snap_right;

    float dist_left = (float)d_left * MM_PER_PULSE;
    float dist_right = (float)d_right * MM_PER_PULSE;
    float dist_center = (dist_left + dist_right) / 2.0f;
    float delta_theta = (dist_right - dist_left) / WHEEL_BASE_MM;

    float mid_theta = current_theta + delta_theta / 2.0f;
    current_theta += delta_theta;
    while (current_theta > M_PI) current_theta -= 2 * M_PI;
    while (current_theta < -M_PI) current_theta += 2 * M_PI;

    current_x += dist_center * cosf(mid_theta);
    current_y += dist_center * sinf(mid_theta);
}

static void clamp_rpm(volatile int *val) {
    if (*val > 600) *val = 600;
    if (*val < -600) *val = -600;
}

void control_task(void *arg) {
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(20)); // 50Hz
        update_odometry();

        if (nav_active) {
            float dx = target_x - current_x;
            float dy = target_y - current_y;
            float dist_to_goal = sqrtf(dx*dx + dy*dy);
            float angle_to_goal = atan2f(dy, dx);
            
            if (current_nav_state == NAV_IDLE) {
                current_nav_state = NAV_ROTATE_TO_GOAL;
                commanded_speed = 0;
            }

            if (current_nav_state == NAV_ROTATE_TO_GOAL) {
                float angle_error = angle_to_goal - current_theta;
                while (angle_error > M_PI) angle_error -= 2 * M_PI;
                while (angle_error < -M_PI) angle_error += 2 * M_PI;

                if (fabsf(angle_error) < ANGLE_TOLERANCE_RAD) {
                    current_nav_state = NAV_DRIVE_TO_GOAL;
                } else {
                    float turn_speed = angle_error * TURN_GAIN;
                    if (turn_speed > 120) turn_speed = 120;
                    if (turn_speed < -120) turn_speed = -120;
                    if (fabsf(turn_speed) < 50) turn_speed = (turn_speed > 0) ? 50 : -50;
                    desired_rpm_left = (int)(-turn_speed);
                    desired_rpm_right = (int)(turn_speed);
                }
            } 
            
            if (current_nav_state == NAV_DRIVE_TO_GOAL) {
                float angle_error = angle_to_goal - current_theta;
                while (angle_error > M_PI) angle_error -= 2 * M_PI;
                while (angle_error < -M_PI) angle_error += 2 * M_PI;

                if (dist_to_goal < DIST_TOLERANCE_MM) {
                    current_nav_state = NAV_ROTATE_TO_FINAL;
                    commanded_speed = 0;
                } else {
                    float target_speed = fminf(NAV_MAX_RPM, dist_to_goal * 0.8f);
                    if (target_speed < NAV_MIN_RPM) target_speed = NAV_MIN_RPM;

                    if (commanded_speed < target_speed) commanded_speed += ACCEL_LIMIT;
                    else if (commanded_speed > target_speed) commanded_speed -= ACCEL_LIMIT;
                    
                    float steer = angle_error * STEER_GAIN;
                    desired_rpm_left = (int)(commanded_speed - steer);
                    desired_rpm_right = (int)(commanded_speed + steer);
                    
                    if (fabsf(angle_error) > 1.0f) {
                        current_nav_state = NAV_ROTATE_TO_GOAL;
                        commanded_speed = 0;
                    }
                }
            }

            if (current_nav_state == NAV_ROTATE_TO_FINAL) {
                float angle_error = target_theta - current_theta;
                while (angle_error > M_PI) angle_error -= 2 * M_PI;
                while (angle_error < -M_PI) angle_error += 2 * M_PI;

                if (fabsf(angle_error) < ANGLE_TOLERANCE_RAD) {
                    ESP_LOGI(TAG, "Nav Complete!");
                    nav_active = false;
                    current_nav_state = NAV_IDLE;
                    desired_rpm_left = 0;
                    desired_rpm_right = 0;
                } else {
                    float turn_speed = angle_error * TURN_GAIN;
                    if (turn_speed > 120) turn_speed = 120;
                    if (turn_speed < -120) turn_speed = -120;
                    if (fabsf(turn_speed) < 50) turn_speed = (turn_speed > 0) ? 50 : -50;
                    desired_rpm_left = (int)(-turn_speed);
                    desired_rpm_right = (int)(turn_speed);
                }
            }
            
            clamp_rpm(&desired_rpm_left);
            clamp_rpm(&desired_rpm_right);
        } else {
            current_nav_state = NAV_IDLE;
            commanded_speed = 0;
        }

        int p_left = run_pid((float)desired_rpm_left, current_measured_rpm_left, &integral_left, &power_left_last);
        int p_right = run_pid((float)desired_rpm_right, current_measured_rpm_right, &integral_right, &power_right_last);
        motors_set(p_left, p_right);
    }
}
