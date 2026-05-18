#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/pulse_cnt.h"
#include "esp_log.h"
#include "encoders.h"

#define SENSOR_GPIO      4       // LM393 DO connected to GPIO 4 (18 is used by motors)
#define SLOTS_ON_DISK    20      // Number of slots on your encoder wheel
#define SAMPLE_PERIOD_MS 1000    // Calculate speed every 1 second

static const char *TAG = "RPM_METER";

static void encoders_task(void *arg) {
    pcnt_unit_handle_t pcnt_unit = (pcnt_unit_handle_t)arg;
    int pulse_count = 0;

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_PERIOD_MS));

        // Safely extract the calculated hardware count and clear it for the next interval
        ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit, &pulse_count));
        ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));

        // Formula: (Pulses in 1 sec * 60 sec) / encoder slot count
        float rpm = (float)(pulse_count * 60) / SLOTS_ON_DISK;

        ESP_LOGI(TAG, "Pulses/sec: %d | Motor Speed: %.2f RPM", pulse_count, rpm);
    }
}

void encoders_init(void)
{
    ESP_LOGI(TAG, "Initializing PCNT Unit...");

    // 1. Configure the PCNT unit structure
    pcnt_unit_config_t unit_config = {
        .high_limit = 20000,     // High threshold limit before looping
        .low_limit = -1,
    };
    pcnt_unit_handle_t pcnt_unit = NULL;
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

    // 2. Configure a Glitch Filter to ignore line electrical noise
    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = 1000,   // Discard pulses shorter than 1 microsecond
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));

    // 3. Configure the specific channel connection
    pcnt_chan_config_t chan_config = {
        .edge_gpio_num = SENSOR_GPIO,
        .level_gpio_num = -1,    // Not tracking direction, only pulse counts
    };
    pcnt_channel_handle_t pcnt_chan = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_config, &pcnt_chan));

    // 4. Tell the channel to increase the counter on a falling edge
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_HOLD));

    // 5. Enable and start the PCNT engine
    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));

    // Create a task to monitor RPM
    xTaskCreate(encoders_task, "encoders_task", 2048, pcnt_unit, 5, NULL);
}
