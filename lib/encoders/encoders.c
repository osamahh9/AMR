#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/pulse_cnt.h"
#include "esp_log.h"
#include "encoders.h"

#define LEFT_SENSOR_GPIO      4
#define RIGHT_SENSOR_GPIO     5
#define SLOTS_ON_DISK         20
#define SAMPLE_PERIOD_MS      100 
#define WINDOW_SIZE           10  // 1 second rolling window

volatile float current_measured_rpm_left = 0.0;
volatile float current_measured_rpm_right = 0.0;

// Rolling window history
static int history_left[WINDOW_SIZE] = {0};
static int history_right[WINDOW_SIZE] = {0};
static int history_idx = 0;

static void encoders_task(void *arg) {
    pcnt_unit_handle_t* units = (pcnt_unit_handle_t*)arg;
    pcnt_unit_handle_t unit_left = units[0];
    pcnt_unit_handle_t unit_right = units[1];
    int pulse_count_left = 0;
    int pulse_count_right = 0;

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_PERIOD_MS));
        
        // Read current pulse counts
        ESP_ERROR_CHECK(pcnt_unit_get_count(unit_left, &pulse_count_left));
        ESP_ERROR_CHECK(pcnt_unit_clear_count(unit_left));
        
        ESP_ERROR_CHECK(pcnt_unit_get_count(unit_right, &pulse_count_right));
        ESP_ERROR_CHECK(pcnt_unit_clear_count(unit_right));

        // Update rolling history
        history_left[history_idx] = pulse_count_left;
        history_right[history_idx] = pulse_count_right;
        history_idx = (history_idx + 1) % WINDOW_SIZE;

        // Calculate total pulses in the 1s window
        int total_left = 0;
        int total_right = 0;
        for (int i = 0; i < WINDOW_SIZE; i++) {
            total_left += history_left[i];
            total_right += history_right[i];
        }

        // RPM = (total_pulses_per_sec * 60) / slots
        // With WINDOW_SIZE=10 and SAMPLE_PERIOD_MS=100, the window is exactly 1 second.
        current_measured_rpm_left = (float)(total_left * 60) / SLOTS_ON_DISK;
        current_measured_rpm_right = (float)(total_right * 60) / SLOTS_ON_DISK;
    }
}

static pcnt_unit_handle_t setup_pcnt_unit(int gpio_num) {
    pcnt_unit_config_t unit_config = { .high_limit = 20000, .low_limit = -1 };
    pcnt_unit_handle_t pcnt_unit = NULL;
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

    pcnt_glitch_filter_config_t filter_config = { .max_glitch_ns = 1000 };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));

    pcnt_chan_config_t chan_config = { .edge_gpio_num = gpio_num, .level_gpio_num = -1 };
    pcnt_channel_handle_t pcnt_chan = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_config, &pcnt_chan));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_HOLD));
    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));

    return pcnt_unit;
}

void encoders_init(void) {
    static pcnt_unit_handle_t units[2];
    units[0] = setup_pcnt_unit(LEFT_SENSOR_GPIO);
    units[1] = setup_pcnt_unit(RIGHT_SENSOR_GPIO);

    xTaskCreate(encoders_task, "encoders_task", 4096, units, 5, NULL);
}
