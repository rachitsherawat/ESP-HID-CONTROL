#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <esp_err.h>

void wifi_init_sta(const char* ssid, const char* pass);

#endif // WIFI_MANAGER_H
