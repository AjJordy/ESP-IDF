#include "wifi.h"

#include <stdio.h>
#include <string.h>

// ---- FreeRTOS includes ----
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"

// ---- ESP32 includes ----
#include "esp_system.h"
#include "esp_log.h"
#include "esp_wifi.h" // WiFi
#include "esp_event.h"
#include "esp_http_client.h"
#include "nvs_flash.h"


#include "lwip/err.h"
#include "lwip/sys.h"


#define EXAMPLE_ESP_WIFI_SSID       CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS       CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUN_RETRY   CONFIG_ESP_MAXIMUM_RETRY

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1


static EventGroupHandle_t s_wifi_event_group;
SemaphoreHandle_t wifi_semaphore; 
static int s_retry_num = 0;


static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    }

    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED){
        if (s_retry_num < EXAMPLE_ESP_MAXIMUN_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI("WIFI", "Retry to connect to the AP");
        }
        else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGI("WIFI", "Retry to connect to the AP");
        }
        ESP_LOGE("WIFI", "Connect to the AP fail");
    }

    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP){
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI("WIFI", "got ip: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}


void wifi_init_sta(){
    wifi_semaphore = xSemaphoreCreateBinary();
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false,
            },
        }
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI("WIFI", "wifi_init_sta finished");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, 
                    WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, 
                    pdFALSE, 
                    pdFALSE, 
                    portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI("WIFI", "Connected to app SSID: %s password:%s", EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
        xSemaphoreGive(wifi_semaphore);
    }

    else if (bits & WIFI_FAIL_BIT){
        ESP_LOGI("WIFI", "Failed to connected to app SSID: %s password:%s", EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    }

    else {
        ESP_LOGI("WIFI", "UNEXPECTED EVENT");
    }

}