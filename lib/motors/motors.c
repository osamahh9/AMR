#include "driver/ledc.h"
#include "driver/gpio.h"
#include "motors.h"

// --- Pin Definitions ---
#define ENA   14
#define IN1   15
#define IN2   21
#define ENB   16
#define IN3   17
#define IN4   18

void motors_init(void) {
    // Setup PWM timer
    ledc_timer_config_t timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_1,
        .duty_resolution  = LEDC_TIMER_10_BIT,
        .freq_hz          = 1000,
        .clk_cfg          = LEDC_USE_XTAL_CLK
    };
    ledc_timer_config(&timer);

    // Setup PWM channel for left motor
    ledc_channel_config_t ch_left = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_0,
        .timer_sel  = LEDC_TIMER_1,
        .gpio_num   = ENA,
        .duty       = 0
    };
    ledc_channel_config(&ch_left);

    // Setup PWM channel for right motor
    ledc_channel_config_t ch_right = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_1,
        .timer_sel  = LEDC_TIMER_1,
        .gpio_num   = ENB,
        .duty       = 0
    };
    ledc_channel_config(&ch_right);

    // Set direction pins as output
    gpio_set_direction(IN1, GPIO_MODE_OUTPUT);
    gpio_set_direction(IN2, GPIO_MODE_OUTPUT);
    gpio_set_direction(IN3, GPIO_MODE_OUTPUT);
    gpio_set_direction(IN4, GPIO_MODE_OUTPUT);
}

// speed: -100% (full reverse) to +100% (full forward), 0 = stop
void motors_set(int S_left, int S_right) {
    // Clamp inputs to [-100, 100] to prevent hardware register overflow (1023 max)
    if (S_left > 100) S_left = 100;
    if (S_left < -100) S_left = -100;
    if (S_right > 100) S_right = 100;
    if (S_right < -100) S_right = -100;

    int left = 1023 * S_left / 100;
    int right = 1023 * S_right / 100;

    // Left motor: Active Braking if 0, else Direction Control
    if (S_left == 0) {
        gpio_set_level(IN1, 0);
        gpio_set_level(IN2, 0);
    } else {
        gpio_set_level(IN1, left > 0 ? 1 : 0);
        gpio_set_level(IN2, left < 0 ? 1 : 0);
    }

    // Right motor: Active Braking if 0, else Direction Control
    if (S_right == 0) {
        gpio_set_level(IN3, 0);
        gpio_set_level(IN4, 0);
    } else {
        gpio_set_level(IN3, right > 0 ? 1 : 0);
        gpio_set_level(IN4, right < 0 ? 1 : 0);
    }

    // Apply Duty Cycles
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, left < 0 ? -left : left);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, right < 0 ? -right : right);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
}
