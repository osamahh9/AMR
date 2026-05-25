#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_random.h"
#include "object.h"

static const char *TAG = "OBJECT_DETECTION";

volatile uint32_t measured_distance = 0;

// --- Configuration ---
#define ULTRASONIC_TRIGGER_PIN 1
#define ULTRASONIC_ECHO_PIN    2
#define MAX_DISTANCE_CM        400
#define DETECTION_THRESHOLD_CM 20

// NOTE: Check if GPIO 6/7 are used for Flash on your specific board!
// If they are, servos will not work.
#define SERVO_PIN_1            10
#define SERVO_PIN_2            11
#define SERVO_CH_1             LEDC_CHANNEL_2
#define SERVO_CH_2             LEDC_CHANNEL_3

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
    .timer_number = LEDC_TIMER_1,
    .channels = {
        .servo_pin = {SERVO_PIN_1, SERVO_PIN_2},
        .ch = {SERVO_CH_1, SERVO_CH_2}
    },
    .channel_number = 2
};

esp_err_t object_detection_init(void) {
    ESP_LOGI(TAG, "Initializing ultrasonic sensor (Trig:%d, Echo:%d)...", ULTRASONIC_TRIGGER_PIN, ULTRASONIC_ECHO_PIN);
    ultrasonic_init(&sensor);

    ESP_LOGI(TAG, "Initializing servos on GPIO %d and %d...", SERVO_PIN_1, SERVO_PIN_2);
    esp_err_t ret = iot_servo_init(LEDC_LOW_SPEED_MODE, &servo_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize servos: %d", ret);
        return ret;
    }

    // TEST: Force a tiny movement to see signal on oscilloscope
    iot_servo_write_angle(LEDC_LOW_SPEED_MODE, SERVO_CH_1, 1.0f);
    iot_servo_write_angle(LEDC_LOW_SPEED_MODE, SERVO_CH_2, 1.0f);
    vTaskDelay(pdMS_TO_TICKS(100));
    iot_servo_write_angle(LEDC_LOW_SPEED_MODE, SERVO_CH_1, 0.0f);
    iot_servo_write_angle(LEDC_LOW_SPEED_MODE, SERVO_CH_2, 0.0f);

    return ESP_OK;
}

void object_set_servo(uint8_t channel, float angle) {
    if (channel == 0) {
        iot_servo_write_angle(LEDC_LOW_SPEED_MODE, SERVO_CH_1, angle);
    } else if (channel == 1) {
        iot_servo_write_angle(LEDC_LOW_SPEED_MODE, SERVO_CH_2, angle);
    }
}

void object_detection_task(void *pvParameters) {
    uint32_t distance;
    esp_err_t res;

    while (1) {
        res = ultrasonic_measure_cm(&sensor, MAX_DISTANCE_CM, &distance);
        if (res != ESP_OK) {
            // Error 513 = ESP_ERR_TIMEOUT. Check your wiring!
            ESP_LOGW(TAG, "Ultrasonic Error: %d (Check wiring!)", res);
            measured_distance = 999;
        } else {
            measured_distance = distance;
            ESP_LOGI(TAG, "Distance: %ld cm", distance);

            if (distance < DETECTION_THRESHOLD_CM) {
                ESP_LOGI(TAG, "Object detected! Randomizing servos...");
                float a1 = (float)(esp_random() % 181);
                float a2 = (float)(esp_random() % 181);
                iot_servo_write_angle(LEDC_LOW_SPEED_MODE, SERVO_CH_1, a1);
                iot_servo_write_angle(LEDC_LOW_SPEED_MODE, SERVO_CH_2, a2);
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
