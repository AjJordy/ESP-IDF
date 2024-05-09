#ifndef WIFI_H
#define WIFI_H

#include "esp_wifi.h"

#define DEFAULT_SCAN_LIST_SIZE 10

void wifi_init_sta();
void wifi_connect_ap(const char *ssid, const char *pass);
void wifi_disconnect();
void wifi_scan();
char *getAuthModeName(wifi_auth_mode_t wifi_auth_mode);

#endif
