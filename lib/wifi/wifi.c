#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "wifi.h"
#include <string.h>

#define SSID "ESP32_Robot"
#define PASS "12345678"

void wifi_init(void) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t ap_config = {
        .ap = {
            .ssid            = SSID,
            .ssid_len        = strlen(SSID),
            .password        = PASS,
            .max_connection  = 2,
            .authmode        = WIFI_AUTH_WPA2_PSK
        }
    };

    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    esp_wifi_start();
}
