#include "nvs_flash.h"
#include "motors.h"
#include "wifi.h"
#include "server.h"
#include "encoders.h"
#include "control.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "object.h"

void init_system(void) {
    nvs_flash_init();
    motors_init();
    wifi_init();
    server_init();
    encoders_init();
    object_detection_init();
}

void app_main(void) {
    init_system();

    xTaskCreate(control_task, "control_task", 4096, NULL, 5, NULL);
    xTaskCreate(object_detection_task, "object_detection_task", 4096, NULL, 5, NULL);
}
