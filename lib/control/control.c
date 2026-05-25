#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "motors.h"
#include "server.h"
#include "encoders.h"
#include "esp_log.h"
#include "control.h"
#include <math.h>

#define MAX_PWM 100
#define MIN_PWM 0

// PID Gains
static float Kp = 0.15;
static float Ki = 0.05;

// PID States for Left Motor
static float integral_left = 0;
static int power_left_last = 0;

// PID States for Right Motor
static float integral_right = 0;
static int power_right_last = 0;

static const char *TAG = "Control";

static int run_pid(float target_rpm, float current_rpm, float *integral, int *last_power) {
    if (target_rpm == 0) {
        *integral = 0;
        return 0;
    }

    float abs_target = fabsf(target_rpm);
    float error = abs_target - current_rpm;

    // Accumulate Integral (Anti-Windup)
    if (*last_power < MAX_PWM && *last_power > MIN_PWM) {
        *integral += error * 0.1; // 100ms sample time
    }

    float control_signal = (Kp * error) + (Ki * (*integral));
    
    // Feed-forward: base power for target RPM
    float feed_forward = (abs_target / 600.0) * 50.0; 
    
    int power = (int)(feed_forward + control_signal);

    // Clamp values
    if (power > MAX_PWM) power = MAX_PWM;
    if (power < MIN_PWM) power = MIN_PWM;

    *last_power = power;
    
    // Apply sign for direction
    return (target_rpm < 0) ? -power : power;
}

void control_task(void *arg) {
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));

        int final_power_left = run_pid((float)desired_rpm_left, current_measured_rpm_left, &integral_left, &power_left_last);
        int final_power_right = run_pid((float)desired_rpm_right, current_measured_rpm_right, &integral_right, &power_right_last);

        motors_set(final_power_left, final_power_right);

        ESP_LOGI(TAG, "L: Tgt %d, Act %.1f, Pwr %d | R: Tgt %d, Act %.1f, Pwr %d", 
                 desired_rpm_left, current_measured_rpm_left, final_power_left,
                 desired_rpm_right, current_measured_rpm_right, final_power_right);
    }
}
