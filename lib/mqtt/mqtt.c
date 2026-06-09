#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "cJSON.h"

#include "mqtt.h"
#include "control.h"
#include <math.h>

static const char *TAG = "MQTT";

static esp_mqtt_client_handle_t client;
static bool connected = false;

// Command handling state
static char current_job_id[64] = "";
static char next_completion_event[64] = "";

typedef struct {
    const char *name;
    float x;
    float y;
    float th;
    const char *arrival_event;
} location_t;

static const location_t locations[] = {
    {"CONVEYOR_RED_BOX_SLOT",   300.0f,  150.0f,  90.0f, "Done_Picking_RedBox"},
    {"CONVEYOR_GREEN_BOX_SLOT", 300.0f,    0.0f,  90.0f, "Done_Picking_GreenBox"},
    {"CONVEYOR_BLUE_BOX_SLOT",  300.0f, -150.0f,  90.0f, "Done_Picking_BlueBox"},
    {"CONVEYOR_END",            400.0f,    0.0f,   0.0f, "AMR_at_conveyor_end"},
    {"ARM_STATION",             800.0f,    0.0f, 180.0f, "AMR_arrived_to_arm_station"},
    {"HOME",                      0.0f,    0.0f,   0.0f, "AMR_at_home"}
};

static const location_t* find_location(const char *name) {
    for (int i = 0; i < sizeof(locations) / sizeof(location_t); i++) {
        if (strcmp(locations[i].name, name) == 0) {
            return &locations[i];
        }
    }
    return NULL;
}

void mqtt_publish_event(const char *event, const char *jobId) {
    if (!connected) return;

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "schema", "robot_cell.v1");
    cJSON_AddStringToObject(root, "source", "amr");
    cJSON_AddStringToObject(root, "event", event);
    cJSON_AddStringToObject(root, "jobId", jobId);
    cJSON_AddStringToObject(root, "robot", "amr");
    cJSON_AddNumberToObject(root, "timeMs", (double)xTaskGetTickCount() * portTICK_PERIOD_MS);

    char *json_str = cJSON_PrintUnformatted(root);
    esp_mqtt_client_publish(client, "cell/amr/event", json_str, 0, 1, 0);
    
    ESP_LOGI(TAG, "Sent event: %s", json_str);
    cJSON_free(json_str);
    cJSON_Delete(root);
}

void mqtt_publish_status(void) {
    if (!connected) return;

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "robot", "amr");
    cJSON_AddStringToObject(root, "state", nav_active ? "MOVING" : "IDLE");
    cJSON_AddNumberToObject(root, "x", current_x);
    cJSON_AddNumberToObject(root, "y", current_y);
    cJSON_AddNumberToObject(root, "th", current_theta * 180.0f / M_PI);
    cJSON_AddBoolToObject(root, "mqtt", true);

    char *json_str = cJSON_PrintUnformatted(root);
    esp_mqtt_client_publish(client, "cell/amr/status", json_str, 0, 1, 0);
    cJSON_free(json_str);
    cJSON_Delete(root);
}

void mqtt_report_completion(void) {
    if (strlen(next_completion_event) > 0) {
        mqtt_publish_event(next_completion_event, current_job_id);
        next_completion_event[0] = '\0';
    }
}

static void handle_amr_command(const char *data, int len) {
    cJSON *root = cJSON_Parse(data);
    if (!root) return;

    cJSON *cmd_item = cJSON_GetObjectItem(root, "cmd");
    cJSON *job_item = cJSON_GetObjectItem(root, "jobId");
    
    if (cmd_item && job_item) {
        const char *cmd = cmd_item->valuestring;
        strncpy(current_job_id, job_item->valuestring, sizeof(current_job_id) - 1);

        if (strcmp(cmd, "PICK_FILLED_BOX") == 0 || strcmp(cmd, "DELIVER_TO_ARM") == 0) {
            cJSON *dest_item = cJSON_GetObjectItem(root, "destination");
            if (!dest_item) dest_item = cJSON_GetObjectItem(root, "dropoff");
            if (!dest_item) dest_item = cJSON_GetObjectItem(root, "pickup");
            
            const char *dest_name = dest_item ? dest_item->valuestring : "ARM_STATION";
            const location_t *loc = find_location(dest_name);

            if (loc) {
                // Store the completion event
                strncpy(next_completion_event, loc->arrival_event, sizeof(next_completion_event) - 1);

                // Acknowledge first
                mqtt_publish_event("AMR_PICK_COMMAND_ACCEPTED", current_job_id);

                // Set nav target
                target_x = loc->x;
                target_y = loc->y;
                target_theta = loc->th * M_PI / 180.0f;
                nav_active = true;
                
                ESP_LOGI(TAG, "Moving to %s (%.1f, %.1f) -> Expecting event: %s", 
                         loc->name, loc->x, loc->y, next_completion_event);
            }
        } else if (strcmp(cmd, "STOP") == 0) {
            nav_active = false;
            mqtt_publish_event("AMR_STOPPED", current_job_id);
        }
    }
    cJSON_Delete(root);
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT Connected");
            connected = true;
            esp_mqtt_client_subscribe(client, "cell/amr/cmd", 1);
            mqtt_publish_status();
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT Disconnected");
            connected = false;
            break;
        case MQTT_EVENT_DATA:
            if (strncmp(event->topic, "cell/amr/cmd", event->topic_len) == 0) {
                handle_amr_command(event->data, event->data_len);
            }
            break;
        default:
            break;
    }
}

static void mqtt_status_task(void *pvParameters) {
    while (1) {
        if (connected) {
            mqtt_publish_status();
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void mqtt_app_start(void) {
    const char *broker_uri = "mqtt://192.168.137.1:1883";
    ESP_LOGI(TAG, "Starting MQTT client targeting %s", broker_uri);
    
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = broker_uri,
        .credentials.client_id = "AMR_ESP32_S3",
        .network.reconnect_timeout_ms = 5000,
        .network.timeout_ms = 10000,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

    xTaskCreate(mqtt_status_task, "mqtt_status_task", 4096, NULL, 5, NULL);
}

bool mqtt_is_connected(void) {
    return connected;
}
