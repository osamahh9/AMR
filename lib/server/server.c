#include "esp_http_server.h"
#include "server.h"
#include "encoders.h"
#include "object.h"
#include "control.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "esp_log.h"

static const char *TAG = "Server";

// Global state variables
volatile int desired_rpm_left = 0;
volatile int desired_rpm_right = 0;
volatile float desired_servo_angle_1 = 90.0f;
volatile float desired_servo_angle_2 = 90.0f;

// Navigation targets
volatile float target_x = 0;
volatile float target_y = 0;
volatile float target_theta = 0;
volatile bool nav_active = false;

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");

esp_err_t handle_root(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    const size_t index_html_size = (index_html_end - index_html_start);
    return httpd_resp_send(req, (const char *)index_html_start, index_html_size);
}

// Handler for /status returns JSON including pose from control module
esp_err_t handle_status(httpd_req_t *req) {
    char json_response[256];
    snprintf(json_response, sizeof(json_response), 
             "{\"left\":%.1f,\"right\":%.1f,\"dist\":%ld,\"obs\":%d,\"x\":%.1f,\"y\":%.1f,\"th\":%.1f}", 
             current_measured_rpm_left, current_measured_rpm_right, 
             measured_distance, obstacle_detected ? 1 : 0,
             current_x, current_y, current_theta);
    
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
}

// Handler for /drive?left=XXX&right=YYY (Manual Override)
esp_err_t handle_drive(httpd_req_t *req) {
    nav_active = false; // Manual drive overrides autonomous nav
    size_t buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        char *buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            char param[10];
            if (httpd_query_key_value(buf, "left", param, sizeof(param)) == ESP_OK) desired_rpm_left = atoi(param);
            if (httpd_query_key_value(buf, "right", param, sizeof(param)) == ESP_OK) desired_rpm_right = atoi(param);
        }
        free(buf);
    }
    httpd_resp_send(req, "OK", 2);
    return ESP_OK;
}

// Handler for /nav?x=X&y=Y&th=T
esp_err_t handle_nav(httpd_req_t *req) {
    size_t buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        char *buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            char val[10];
            if (httpd_query_key_value(buf, "x", val, sizeof(val)) == ESP_OK) target_x = atof(val);
            if (httpd_query_key_value(buf, "y", val, sizeof(val)) == ESP_OK) target_y = atof(val);
            if (httpd_query_key_value(buf, "th", val, sizeof(val)) == ESP_OK) {
                // Degrees to Radians
                target_theta = atof(val) * M_PI / 180.0f;
            }
            nav_active = true;
            ESP_LOGI(TAG, "New Nav Target: X=%.1f, Y=%.1f, Th(rad)=%.2f", target_x, target_y, target_theta);
        }
        free(buf);
    }
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
            int ch = 0; float angle = 0;
            if (httpd_query_key_value(buf, "ch", ch_str, sizeof(ch_str)) == ESP_OK) ch = atoi(ch_str);
            if (httpd_query_key_value(buf, "angle", angle_str, sizeof(angle_str)) == ESP_OK) angle = atof(angle_str);
            if (ch == 0) desired_servo_angle_1 = angle;
            else if (ch == 1) desired_servo_angle_2 = angle;
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
        httpd_register_uri_handler(server, &(httpd_uri_t){ .uri="/nav",   .method=HTTP_GET, .handler=handle_nav   });
    } else {
        ESP_LOGE(TAG, "Failed to start HTTP server");
    }
}
