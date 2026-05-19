#include "esp_http_server.h"
#include "server.h"
#include <stdlib.h> // Required for atoi()

// Define the global state variables
volatile robot_cmd_t current_cmd = CMD_STOP;
volatile int desired_rpm = 0;

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");

esp_err_t handle_root(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    const size_t index_html_size = (index_html_end - index_html_start);
    return httpd_resp_send(req, (const char *)index_html_start, index_html_size);
}

// Helper function to extract ?rpm=XXX and clamp it to 0-600
int get_rpm(httpd_req_t *req) {
    int rpm = 300; // Default speed
    size_t buf_len = httpd_req_get_url_query_len(req) + 1;
    
    if (buf_len > 1) {
        char *buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            char param[10];
            if (httpd_query_key_value(buf, "rpm", param, sizeof(param)) == ESP_OK) {
                rpm = atoi(param);
            }
        }
        free(buf);
    }
    
    // Safety clamp: ensure the value is strictly between 0 and 600
    if (rpm > 600) rpm = 600;
    if (rpm < 0) rpm = 0;
    
    return rpm;
}

// Handlers now ONLY update the state variables
esp_err_t handle_fwd(httpd_req_t *req)   { current_cmd = CMD_FWD;   desired_rpm = get_rpm(req); httpd_resp_send(req, "OK", 2); return ESP_OK; }
esp_err_t handle_rev(httpd_req_t *req)   { current_cmd = CMD_REV;   desired_rpm = get_rpm(req); httpd_resp_send(req, "OK", 2); return ESP_OK; }
esp_err_t handle_left(httpd_req_t *req)  { current_cmd = CMD_LEFT;  desired_rpm = get_rpm(req); httpd_resp_send(req, "OK", 2); return ESP_OK; }
esp_err_t handle_right(httpd_req_t *req) { current_cmd = CMD_RIGHT; desired_rpm = get_rpm(req); httpd_resp_send(req, "OK", 2); return ESP_OK; }
esp_err_t handle_stop(httpd_req_t *req)  { current_cmd = CMD_STOP;  desired_rpm = 0;            httpd_resp_send(req, "OK", 2); return ESP_OK; }

void server_init(void) {
    httpd_handle_t server;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_start(&server, &config);

    httpd_register_uri_handler(server, &(httpd_uri_t){ .uri="/",      .method=HTTP_GET, .handler=handle_root  });
    httpd_register_uri_handler(server, &(httpd_uri_t){ .uri="/fwd",   .method=HTTP_GET, .handler=handle_fwd   });
    httpd_register_uri_handler(server, &(httpd_uri_t){ .uri="/rev",   .method=HTTP_GET, .handler=handle_rev   });
    httpd_register_uri_handler(server, &(httpd_uri_t){ .uri="/left",  .method=HTTP_GET, .handler=handle_left  });
    httpd_register_uri_handler(server, &(httpd_uri_t){ .uri="/right", .method=HTTP_GET, .handler=handle_right });
    httpd_register_uri_handler(server, &(httpd_uri_t){ .uri="/stop",  .method=HTTP_GET, .handler=handle_stop  });
}