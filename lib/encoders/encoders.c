#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/pulse_cnt.h"
#include "esp_log.h"
#include "encoders.h"

#define SENSOR_GPIO      4
#define SLOTS_ON_DISK    20
#define SAMPLE_PERIOD_MS 100 

volatile float current_measured_rpm = 0.0;

static void encoders_task(void *arg) {
    pcnt_unit_handle_t pcnt_unit = (pcnt_unit_handle_t)arg;
    int pulse_count = 0;

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_PERIOD_MS));
        ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit, &pulse_count));
        ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));

        // Corrected Math: (pulses * (1000/100) * 60) / 20 = pulse_count * 30
        current_measured_rpm = (float)(pulse_count * 300) / SLOTS_ON_DISK;
    }
}

void encoders_init(void) {
    pcnt_unit_config_t unit_config = { .high_limit = 20000, .low_limit = -1 };
    pcnt_unit_handle_t pcnt_unit = NULL;
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

    pcnt_glitch_filter_config_t filter_config = { .max_glitch_ns = 1000 };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));

    pcnt_chan_config_t chan_config = { .edge_gpio_num = SENSOR_GPIO, .level_gpio_num = -1 };
    pcnt_channel_handle_t pcnt_chan = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_config, &pcnt_chan));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_HOLD));
    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));

    xTaskCreate(encoders_task, "encoders_task", 2048, pcnt_unit, 5, NULL);
}
