#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi_manager.h"
#include "hid_service.h"
#include "web_server.h"

static const char *TAG = "MAIN";

// WIFI Credentials - USER: UPDATE THESE
#define EXAMPLE_ESP_WIFI_SSID      "Galaxy S23"
#define EXAMPLE_ESP_WIFI_PASS      "encrypt@256"

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Starting Wireless HID System...");

    // 1. Initialize BLE HID Service
    hid_init();
    ESP_LOGI(TAG, "BLE HID Initialized.");

    // 2. Initialize WiFi
    wifi_init_sta(EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    ESP_LOGI(TAG, "WiFi Station Mode Started.");

    // 3. Start WebSocket Server
    start_webserver();
    ESP_LOGI(TAG, "WebSocket Server Started.");
}
