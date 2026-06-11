#include "nvs_flash.h"
#include "motors.h"
#include "wifi.h"
#include "server.h"
#include "encoders.h"
#include "control.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "object.h"
#include "mqtt.h"

void init_system(void) {
    nvs_flash_init();
    object_detection_init(); // Initialize servos first on Timer 0
    motors_init();           // Initialize motors second on Timer 1
    wifi_init();
    server_init();
    encoders_init();
    mqtt_app_start();
}

void app_main(void) {
    init_system();

    // High priority for motor control (10) and safety sensors (7)
    xTaskCreate(control_task, "control_task", 4096, NULL, 10, NULL);
    xTaskCreate(object_detection_task, "object_detection_task", 4096, NULL, 7, NULL);
}
