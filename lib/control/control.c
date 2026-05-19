#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "motors.h"
#include "server.h"
#include "encoders.h"
#include "esp_log.h"
#include "control.h"

#define MAX_PWM 100
#define MIN_PWM 0

// PID Gains - These might need tuning!
float Kp = 0.15;
float Ki = 0.05;
float integral = 0;

volatile float control_signal = 0;
volatile int Power = 0;

static const char *TAG = "Control";

void control_task(void *arg) {
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
        
        float error = (float)desired_rpm - current_measured_rpm;
        
        // Accumulate Integral (only if motor isnt saturated - Anti-Windup)
        if (Power < MAX_PWM && Power > MIN_PWM) {
            integral += error * 0.1; // 0.1 is the 100ms sample time
        }

        control_signal = (Kp * error) + (Ki * integral);
        
        // Final PWM = Feed-forward + PID
        // Basic feed-forward: if desired_rpm is 300, start at ~30% power
        float feed_forward = ((float)desired_rpm / 600.0) * 50.0; 
        
        Power = (int)(feed_forward + control_signal);

        // Clamp values
        if (Power > MAX_PWM) Power = MAX_PWM;
        if (Power < MIN_PWM) Power = MIN_PWM;

        motors_set(Power, 0);

        ESP_LOGI(TAG, "Target: %d | Actual: %.1f | PWM: %d | Error: %.1f", 
                 desired_rpm, current_measured_rpm, Power, error);
    }
}
