#include "esp_http_server.h"
#include "server.h"
#include "encoders.h"
#include "object.h"
#include <stdlib.h>
#include <stdio.h>
#include "esp_log.h"

static const char *TAG = "Server";

// Global state variables for dual motor control and servos
volatile int desired_rpm_left = 0;
volatile int desired_rpm_right = 0;
volatile float desired_servo_angle_1 = 90.0f;
volatile float desired_servo_angle_2 = 90.0f;

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");

esp_err_t handle_root(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    const size_t index_html_size = (index_html_end - index_html_start);
    return httpd_resp_send(req, (const char *)index_html_start, index_html_size);
}

// Handler for /status returns JSON including obstacle flag
esp_err_t handle_status(httpd_req_t *req) {
    char json_response[128];
    snprintf(json_response, sizeof(json_response), 
             "{\"left\":%.1f,\"right\":%.1f,\"dist\":%ld,\"obs\":%d}", 
             current_measured_rpm_left, current_measured_rpm_right, 
             measured_distance, obstacle_detected ? 1 : 0);
    
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
}

// Handler for /drive?left=XXX&right=YYY
esp_err_t handle_drive(httpd_req_t *req) {
    size_t buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        char *buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            char param[10];
            // Parse left RPM
            if (httpd_query_key_value(buf, "left", param, sizeof(param)) == ESP_OK) {
                desired_rpm_left = atoi(param);
            }
            // Parse right RPM
            if (httpd_query_key_value(buf, "right", param, sizeof(param)) == ESP_OK) {
                desired_rpm_right = atoi(param);
            }
        }
        free(buf);
    }
    
    // Safety clamp: ensure values are within -600 to 600
    if (desired_rpm_left > 600)  desired_rpm_left = 600;
    if (desired_rpm_left < -600) desired_rpm_left = -600;
    if (desired_rpm_right > 600)  desired_rpm_right = 600;
    if (desired_rpm_right < -600) desired_rpm_right = -600;

    httpd_resp_send(req, "OK", 2);
    return ESP_OK;
}

// Handler for /servo?ch=X&angle=Y
esp_err_t handle_servo(httpd_req_t *req) {
    size_t buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        char *buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            char ch_str[10], angle_str[10];
            int ch = 0;
            float angle = 0;
            
            if (httpd_query_key_value(buf, "ch", ch_str, sizeof(ch_str)) == ESP_OK) {
                ch = atoi(ch_str);
            }
            if (httpd_query_key_value(buf, "angle", angle_str, sizeof(angle_str)) == ESP_OK) {
                angle = atof(angle_str);
            }
            
            if (ch == 0) {
                desired_servo_angle_1 = angle;
            } else if (ch == 1) {
                desired_servo_angle_2 = angle;
            }
            ESP_LOGI(TAG, "Desired Servo CH %d Angle set to %.1f", ch, angle);
        }
        free(buf);
    }
    httpd_resp_send(req, "OK", 2);
    return ESP_OK;
}

void server_init(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &(httpd_uri_t){ .uri="/",      .method=HTTP_GET, .handler=handle_root  });
        httpd_register_uri_handler(server, &(httpd_uri_t){ .uri="/drive", .method=HTTP_GET, .handler=handle_drive });
        httpd_register_uri_handler(server, &(httpd_uri_t){ .uri="/status",.method=HTTP_GET, .handler=handle_status });
        httpd_register_uri_handler(server, &(httpd_uri_t){ .uri="/servo", .method=HTTP_GET, .handler=handle_servo });
    } else {
        ESP_LOGE(TAG, "Failed to start HTTP server");
    }
}
