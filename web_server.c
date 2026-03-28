#include "web_server.h"
#include <esp_http_server.h>
#include <esp_log.h>
#include <cJSON.h>
#include "hid_service.h"

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");

static const char *TAG = "WEB_SERVER";

static esp_err_t ws_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "Handshake done, the new connection was opened");
        return ESP_OK;
    }
    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    /* Set max_len = 0 to get the frame len */
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
        return ret;
    }
    ESP_LOGI(TAG, "frame len is %d", ws_pkt.len);
    if (ws_pkt.len) {
        /* ws_pkt.len + 1 is for NULL termination as we are expecting a string */
        buf = calloc(1, ws_pkt.len + 1);
        if (buf == NULL) {
            ESP_LOGE(TAG, "Failed to calloc memory for buf");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        /* Set max_len = ws_pkt.len to get the frame payload */
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
            free(buf);
            return ret;
        }
        ESP_LOGI(TAG, "Got packet with message: %s", ws_pkt.payload);
        
        // Parse JSON
        cJSON *root = cJSON_Parse((const char *)ws_pkt.payload);
        if (root) {
            cJSON *type = cJSON_GetObjectItem(root, "type");
            if (cJSON_IsString(type)) {
                if (strcmp(type->valuestring, "keyboard") == 0) {
                    cJSON *action = cJSON_GetObjectItem(root, "action");
                    cJSON *key = cJSON_GetObjectItem(root, "key");
                    if (cJSON_IsNumber(key)) {
                        uint8_t keycode = (uint8_t)key->valueint;
                        if (strcmp(action->valuestring, "tap") == 0) {
                            key_press(keycode);
                            vTaskDelay(pdMS_TO_TICKS(10)); // Tiny delay
                            key_release();
                        } else if (strcmp(action->valuestring, "press") == 0) {
                            key_press(keycode);
                        } else if (strcmp(action->valuestring, "release") == 0) {
                            key_release();
                        }
                    }
                } else if (strcmp(type->valuestring, "mouse") == 0) {
                    cJSON *x = cJSON_GetObjectItem(root, "x");
                    cJSON *y = cJSON_GetObjectItem(root, "y");
                    cJSON *btn = cJSON_GetObjectItem(root, "btn");
                    send_mouse_report(
                        cJSON_IsNumber(btn) ? (uint8_t)btn->valueint : 0,
                        cJSON_IsNumber(x) ? (int8_t)x->valueint : 0,
                        cJSON_IsNumber(y) ? (int8_t)y->valueint : 0,
                        0
                    );
                }
            }
            cJSON_Delete(root);
        }
    }
    free(buf);
    return ESP_OK;
}

static esp_err_t get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start);
}

static const httpd_uri_t get_root = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = get_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t ws = {
    .uri        = "/ws",
    .method     = HTTP_GET,
    .handler    = ws_handler,
    .user_ctx   = NULL,
    .is_websocket = true
};

void start_webserver(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &get_root);
        httpd_register_uri_handler(server, &ws);
    }
}
