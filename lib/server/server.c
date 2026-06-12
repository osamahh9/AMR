#include "esp_http_server.h"
#include "server.h"
#include "encoders.h"
#include "object.h"
#include "control.h"
#include "mqtt.h"
#include "motors.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "esp_log.h"
#include "cJSON.h"

static const char *TAG = "Server";

// Servo angles remain local to server (or can be moved later)
volatile float desired_servo_angle_1 = 90.0f;
volatile float desired_servo_angle_2 = 90.0f;

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");

esp_err_t handle_root(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    const size_t index_html_size = (index_html_end - index_html_start);
    return httpd_resp_send(req, (const char *)index_html_start, index_html_size);
}

// Handler for /status returns JSON including pose from control module
esp_err_t handle_status(httpd_req_t *req) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "left", current_measured_rpm_left);
    cJSON_AddNumberToObject(root, "right", current_measured_rpm_right);
    cJSON_AddNumberToObject(root, "dist", measured_distance);
    cJSON_AddBoolToObject(root, "obs", obstacle_detected);
    cJSON_AddNumberToObject(root, "x", current_x);
    cJSON_AddNumberToObject(root, "y", current_y);
    cJSON_AddNumberToObject(root, "th", current_theta);
    cJSON_AddBoolToObject(root, "mqtt", mqtt_is_connected());
    cJSON_AddStringToObject(root, "cmd", last_mqtt_cmd);

    char *json_str = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, HTTPD_RESP_USE_STRLEN);

    free(json_str);
    cJSON_Delete(root);
    return ESP_OK;
}

// Handler for /halt
esp_err_t handle_halt(httpd_req_t *req) {
    emergency_halt_active = true;
    nav_active = false;
    control_reset_state();
    motors_set(0, 0);
    ESP_LOGW(TAG, "EMERGENCY HALT ACTIVATED");
    httpd_resp_send(req, "HALTED", 6);
    return ESP_OK;
}

// Handler for /drive?left=XXX&right=YYY (Manual Override)
esp_err_t handle_drive(httpd_req_t *req) {
    emergency_halt_active = false; // Reset halt on new command
    if (nav_active) {
        nav_active = false;
        control_reset_state();
    }
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
    emergency_halt_active = false; // Reset halt on new command
    size_t buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        char *buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            char val[10];
            control_reset_state(); // Clear state before new mission
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

// Handler for /tune?param=XXX&val=YYY
esp_err_t handle_tune(httpd_req_t *req) {
    size_t buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        char *buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            char param[32], val[16];
            if (httpd_query_key_value(buf, "param", param, sizeof(param)) == ESP_OK &&
                httpd_query_key_value(buf, "val", val, sizeof(val)) == ESP_OK) {
                float v = atof(val);
                if (strcmp(param, "kp") == 0) Kp = v;
                else if (strcmp(param, "ki") == 0) Ki = v;
                else if (strcmp(param, "accel") == 0) accel_limit = v;
                else if (strcmp(param, "dist_tol") == 0) dist_tolerance = v;
                else if (strcmp(param, "ang_tol") == 0) angle_tolerance = v;
                else if (strcmp(param, "obs_th") == 0) obstacle_threshold = (uint32_t)v;
                ESP_LOGI(TAG, "Tuning: %s = %.3f", param, v);
            }
        }
        free(buf);
    }
    httpd_resp_send(req, "OK", 2);
    return ESP_OK;
}

// Handler for /mode?pid=0/1
esp_err_t handle_mode(httpd_req_t *req) {
    size_t buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        char *buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            char val[10];
            if (httpd_query_key_value(buf, "pid", val, sizeof(val)) == ESP_OK) {
                bool new_mode = (atoi(val) != 0);
                if (new_mode != manual_pid_enabled) {
                    control_reset_state();
                    manual_pid_enabled = new_mode;
                }
                ESP_LOGI(TAG, "Control Mode: %s", manual_pid_enabled ? "PID" : "RAW");
            }
        }
        free(buf);
    }
    httpd_resp_send(req, "OK", 2);
    return ESP_OK;
}

// Handler for /raw?left=XXX&right=YYY
esp_err_t handle_raw(httpd_req_t *req) {
    emergency_halt_active = false; // Reset halt on new command
    if (manual_pid_enabled) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Enable RAW mode first");
    }
    size_t buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        char *buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            char p_l[10], p_r[10];
            if (httpd_query_key_value(buf, "left", p_l, sizeof(p_l)) == ESP_OK) manual_power_left = atoi(p_l);
            if (httpd_query_key_value(buf, "right", p_r, sizeof(p_r)) == ESP_OK) manual_power_right = atoi(p_r);
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
    config.max_uri_handlers = 16; 
    config.max_open_sockets = 7;   // Keep at 7 but improve recycling
    config.lru_purge_enable = true;
    config.recv_wait_timeout = 1;  // Aggressive recycling
    config.send_wait_timeout = 1;
    config.keep_alive_enable = true; 

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &(httpd_uri_t){ .uri="/",           .method=HTTP_GET, .handler=handle_root  });
        httpd_register_uri_handler(server, &(httpd_uri_t){ .uri="/drive",      .method=HTTP_GET, .handler=handle_drive });
        httpd_register_uri_handler(server, &(httpd_uri_t){ .uri="/status",     .method=HTTP_GET, .handler=handle_status });
        httpd_register_uri_handler(server, &(httpd_uri_t){ .uri="/job/status", .method=HTTP_GET, .handler=handle_status });
        httpd_register_uri_handler(server, &(httpd_uri_t){ .uri="/identity",   .method=HTTP_GET, .handler=handle_status });
        httpd_register_uri_handler(server, &(httpd_uri_t){ .uri="/servo",      .method=HTTP_GET, .handler=handle_servo });
        httpd_register_uri_handler(server, &(httpd_uri_t){ .uri="/nav",        .method=HTTP_GET, .handler=handle_nav   });
        httpd_register_uri_handler(server, &(httpd_uri_t){ .uri="/tune",       .method=HTTP_GET, .handler=handle_tune  });
        httpd_register_uri_handler(server, &(httpd_uri_t){ .uri="/mode",       .method=HTTP_GET, .handler=handle_mode  });
        httpd_register_uri_handler(server, &(httpd_uri_t){ .uri="/raw",        .method=HTTP_GET, .handler=handle_raw   });
        httpd_register_uri_handler(server, &(httpd_uri_t){ .uri="/halt",       .method=HTTP_GET, .handler=handle_halt  });
    } else {
        ESP_LOGE(TAG, "Failed to start HTTP server");
    }
}
