idf_component_register(SRCS "main.c" "hid_service.c" "wifi_manager.c" "web_server.c"
                    INCLUDE_DIRS "."
                    REQUIRES bt nvs_flash esp_wifi esp_http_server esp_event json
                    EMBED_FILES "web/index.html")
