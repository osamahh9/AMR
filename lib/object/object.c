#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "object.h"
#include "server.h"

static const char *TAG = "OBJECT_DETECTION";

volatile uint32_t measured_distance = 0;
volatile bool obstacle_detected = false;

// --- Configuration ---
#define ULTRASONIC_TRIGGER_PIN 1
#define ULTRASONIC_ECHO_PIN    2
#define MAX_DISTANCE_CM        400
#define DETECTION_THRESHOLD_CM 20

// NOTE: Check if GPIO 6/7 are used for Flash on your specific board!
// If they are, servos will not work.
#define SERVO_PIN_1            6
#define SERVO_PIN_2            7
#define SERVO_PIN_3            8
#define SERVO_CH_1             LEDC_CHANNEL_2
#define SERVO_CH_2             LEDC_CHANNEL_3
#define SERVO_CH_3             LEDC_CHANNEL_4

// --- Global variables ---
static ultrasonic_sensor_t sensor = {
    .trigger_pin = ULTRASONIC_TRIGGER_PIN,
    .echo_pin = ULTRASONIC_ECHO_PIN
};

static servo_config_t servo_cfg = {
    .max_angle = 180,
    .min_width_us = 500,
    .max_width_us = 2500,
    .freq = 50,
    .timer_number = LEDC_TIMER_0, // Using TIMER 1 to avoid conflict with motors on TIMER 0
    .channels = {
        .servo_pin = {SERVO_PIN_1, SERVO_PIN_2, SERVO_PIN_3},
        .ch = {SERVO_CH_1, SERVO_CH_2, SERVO_CH_3}
    },
    .channel_number = 3
};

esp_err_t object_detection_init(void) {
    ESP_LOGI(TAG, "Initializing ultrasonic sensor (Trig:%d, Echo:%d)...", ULTRASONIC_TRIGGER_PIN, ULTRASONIC_ECHO_PIN);
    ultrasonic_init(&sensor);

    ESP_LOGI(TAG, "Initializing servos on GPIO %d, %d, and %d...", SERVO_PIN_1, SERVO_PIN_2, SERVO_PIN_3);
    esp_err_t ret = iot_servo_init(LEDC_LOW_SPEED_MODE, &servo_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize servos: %d", ret);
        return ret;
    }

    // TEST: Force a tiny movement to see signal on oscilloscope
    iot_servo_write_angle(LEDC_LOW_SPEED_MODE, SERVO_CH_1, 1.0f);
    iot_servo_write_angle(LEDC_LOW_SPEED_MODE, SERVO_CH_2, 1.0f);
    iot_servo_write_angle(LEDC_LOW_SPEED_MODE, SERVO_CH_3, 1.0f);
    vTaskDelay(pdMS_TO_TICKS(100));
    iot_servo_write_angle(LEDC_LOW_SPEED_MODE, SERVO_CH_1, 0.0f);
    iot_servo_write_angle(LEDC_LOW_SPEED_MODE, SERVO_CH_2, 0.0f);
    iot_servo_write_angle(LEDC_LOW_SPEED_MODE, SERVO_CH_3, 0.0f);

    return ESP_OK;
}

void object_set_servo(uint8_t channel, float angle) {
    if (channel == 0) {
        iot_servo_write_angle(LEDC_LOW_SPEED_MODE, SERVO_CH_1, angle);
    } else if (channel == 1) {
        iot_servo_write_angle(LEDC_LOW_SPEED_MODE, SERVO_CH_2, angle);
    } else if (channel == 2) {
        iot_servo_write_angle(LEDC_LOW_SPEED_MODE, SERVO_CH_3, angle);
    }
}

void object_detection_task(void *pvParameters) {
    uint32_t distance;
    esp_err_t res;

    while (1) {
        res = ultrasonic_measure_cm(&sensor, MAX_DISTANCE_CM, &distance);
        if (res != ESP_OK) {
            ESP_LOGW(TAG, "Ultrasonic Error: %d (Check wiring!)", res);
            measured_distance = 999;
            obstacle_detected = false;
        } else {
            measured_distance = distance;
            obstacle_detected = (distance < DETECTION_THRESHOLD_CM);
            ESP_LOGI(TAG, "Distance: %ld cm | Obstacle: %s", distance, obstacle_detected ? "YES" : "NO");

            // Servo 1 & 2 follow web angles
            iot_servo_write_angle(LEDC_LOW_SPEED_MODE, SERVO_CH_1, desired_servo_angle_1);
            iot_servo_write_angle(LEDC_LOW_SPEED_MODE, SERVO_CH_2, desired_servo_angle_2);
            
            // Servo 3 follows inverse of Servo 1
            float inverse_angle = 180.0f - desired_servo_angle_1;
            iot_servo_write_angle(LEDC_LOW_SPEED_MODE, SERVO_CH_3, inverse_angle);
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
