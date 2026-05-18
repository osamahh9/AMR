#include "nvs_flash.h"
#include "motors.h"
#include "wifi.h"
#include "server.h"
#include "encoders.h"

void app_main(void) {
    nvs_flash_init();
    motors_init();
    wifi_init();
    server_init();
    encoders_init();
}
