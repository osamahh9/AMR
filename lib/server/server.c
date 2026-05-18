#include "esp_http_server.h"
#include "motors.h"
#include "server.h"

// Serve the HTML page
esp_err_t handle_root(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    return ESP_OK;
}

// Movement handlers — each calls motors_set() then replies "OK"
esp_err_t handle_fwd(httpd_req_t *req)   { motors_set( 100,  100); httpd_resp_send(req, "OK", 2); return ESP_OK; }
esp_err_t handle_rev(httpd_req_t *req)   { motors_set(-100, -100); httpd_resp_send(req, "OK", 2); return ESP_OK; }
esp_err_t handle_left(httpd_req_t *req)  { motors_set(-100,  100); httpd_resp_send(req, "OK", 2); return ESP_OK; }
esp_err_t handle_right(httpd_req_t *req) { motors_set( 100, -100); httpd_resp_send(req, "OK", 2); return ESP_OK; }
esp_err_t handle_stop(httpd_req_t *req)  { motors_set(0, 0);       httpd_resp_send(req, "OK", 2); return ESP_OK; }

void server_init(void) {
    httpd_handle_t server;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_start(&server, &config);

    // Register each URL route with its handler function
    httpd_register_uri_handler(server, &(httpd_uri_t){ .uri="/",      .method=HTTP_GET, .handler=handle_root  });
    httpd_register_uri_handler(server, &(httpd_uri_t){ .uri="/fwd",   .method=HTTP_GET, .handler=handle_fwd   });
    httpd_register_uri_handler(server, &(httpd_uri_t){ .uri="/rev",   .method=HTTP_GET, .handler=handle_rev   });
    httpd_register_uri_handler(server, &(httpd_uri_t){ .uri="/left",  .method=HTTP_GET, .handler=handle_left  });
    httpd_register_uri_handler(server, &(httpd_uri_t){ .uri="/right", .method=HTTP_GET, .handler=handle_right });
    httpd_register_uri_handler(server, &(httpd_uri_t){ .uri="/stop",  .method=HTTP_GET, .handler=handle_stop  });
}
