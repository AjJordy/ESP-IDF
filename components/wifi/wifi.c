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
#include "esp_mac.h"
#include "esp_log.h"
#include "esp_wifi.h" // WiFi
#include "esp_event.h"

#include "lwip/err.h"
#include "lwip/sys.h"

static const char* TAG = "WIFI";


#define EXAMPLE_ESP_WIFI_SSID       CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS       CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUN_RETRY   CONFIG_ESP_MAXIMUM_RETRY

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1


static bool attempt_reconnect = false;
static esp_netif_t *esp_netif;
static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;
SemaphoreHandle_t wifi_semaphore; 


//get wifi disconnection string
char *get_wifi_disconnection_string(wifi_err_reason_t wifi_err_reason)
{
    switch (wifi_err_reason)
    {
    case WIFI_REASON_UNSPECIFIED:
        return "WIFI_REASON_UNSPECIFIED";
    case WIFI_REASON_AUTH_EXPIRE:
        return "WIFI_REASON_AUTH_EXPIRE";
    case WIFI_REASON_AUTH_LEAVE:
        return "WIFI_REASON_AUTH_LEAVE";
    case WIFI_REASON_ASSOC_EXPIRE:
        return "WIFI_REASON_ASSOC_EXPIRE";
    case WIFI_REASON_ASSOC_TOOMANY:
        return "WIFI_REASON_ASSOC_TOOMANY";
    case WIFI_REASON_NOT_AUTHED:
        return "WIFI_REASON_NOT_AUTHED";
    case WIFI_REASON_NOT_ASSOCED:
        return "WIFI_REASON_NOT_ASSOCED";
    case WIFI_REASON_ASSOC_LEAVE:
        return "WIFI_REASON_ASSOC_LEAVE";
    case WIFI_REASON_ASSOC_NOT_AUTHED:
        return "WIFI_REASON_ASSOC_NOT_AUTHED";
    case WIFI_REASON_DISASSOC_PWRCAP_BAD:
        return "WIFI_REASON_DISASSOC_PWRCAP_BAD";
    case WIFI_REASON_DISASSOC_SUPCHAN_BAD:
        return "WIFI_REASON_DISASSOC_SUPCHAN_BAD";
    case WIFI_REASON_BSS_TRANSITION_DISASSOC:
        return "WIFI_REASON_BSS_TRANSITION_DISASSOC";
    case WIFI_REASON_IE_INVALID:
        return "WIFI_REASON_IE_INVALID";
    case WIFI_REASON_MIC_FAILURE:
        return "WIFI_REASON_MIC_FAILURE";
    case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
        return "WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT";
    case WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT:
        return "WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT";
    case WIFI_REASON_IE_IN_4WAY_DIFFERS:
        return "WIFI_REASON_IE_IN_4WAY_DIFFERS";
    case WIFI_REASON_GROUP_CIPHER_INVALID:
        return "WIFI_REASON_GROUP_CIPHER_INVALID";
    case WIFI_REASON_PAIRWISE_CIPHER_INVALID:
        return "WIFI_REASON_PAIRWISE_CIPHER_INVALID";
    case WIFI_REASON_AKMP_INVALID:
        return "WIFI_REASON_AKMP_INVALID";
    case WIFI_REASON_UNSUPP_RSN_IE_VERSION:
        return "WIFI_REASON_UNSUPP_RSN_IE_VERSION";
    case WIFI_REASON_INVALID_RSN_IE_CAP:
        return "WIFI_REASON_INVALID_RSN_IE_CAP";
    case WIFI_REASON_802_1X_AUTH_FAILED:
        return "WIFI_REASON_802_1X_AUTH_FAILED";
    case WIFI_REASON_CIPHER_SUITE_REJECTED:
        return "WIFI_REASON_CIPHER_SUITE_REJECTED";
    case WIFI_REASON_TDLS_PEER_UNREACHABLE:
        return "WIFI_REASON_TDLS_PEER_UNREACHABLE";
    case WIFI_REASON_TDLS_UNSPECIFIED:
        return "WIFI_REASON_TDLS_UNSPECIFIED";
    case WIFI_REASON_SSP_REQUESTED_DISASSOC:
        return "WIFI_REASON_SSP_REQUESTED_DISASSOC";
    case WIFI_REASON_NO_SSP_ROAMING_AGREEMENT:
        return "WIFI_REASON_NO_SSP_ROAMING_AGREEMENT";
    case WIFI_REASON_BAD_CIPHER_OR_AKM:
        return "WIFI_REASON_BAD_CIPHER_OR_AKM";
    case WIFI_REASON_NOT_AUTHORIZED_THIS_LOCATION:
        return "WIFI_REASON_NOT_AUTHORIZED_THIS_LOCATION";
    case WIFI_REASON_SERVICE_CHANGE_PERCLUDES_TS:
        return "WIFI_REASON_SERVICE_CHANGE_PERCLUDES_TS";
    case WIFI_REASON_UNSPECIFIED_QOS:
        return "WIFI_REASON_UNSPECIFIED_QOS";
    case WIFI_REASON_NOT_ENOUGH_BANDWIDTH:
        return "WIFI_REASON_NOT_ENOUGH_BANDWIDTH";
    case WIFI_REASON_MISSING_ACKS:
        return "WIFI_REASON_MISSING_ACKS";
    case WIFI_REASON_EXCEEDED_TXOP:
        return "WIFI_REASON_EXCEEDED_TXOP";
    case WIFI_REASON_STA_LEAVING:
        return "WIFI_REASON_STA_LEAVING";
    case WIFI_REASON_END_BA:
        return "WIFI_REASON_END_BA";
    case WIFI_REASON_UNKNOWN_BA:
        return "WIFI_REASON_UNKNOWN_BA";
    case WIFI_REASON_TIMEOUT:
        return "WIFI_REASON_TIMEOUT";
    case WIFI_REASON_PEER_INITIATED:
        return "WIFI_REASON_PEER_INITIATED";
    case WIFI_REASON_AP_INITIATED:
        return "WIFI_REASON_AP_INITIATED";
    case WIFI_REASON_INVALID_FT_ACTION_FRAME_COUNT:
        return "WIFI_REASON_INVALID_FT_ACTION_FRAME_COUNT";
    case WIFI_REASON_INVALID_PMKID:
        return "WIFI_REASON_INVALID_PMKID";
    case WIFI_REASON_INVALID_MDE:
        return "WIFI_REASON_INVALID_MDE";
    case WIFI_REASON_INVALID_FTE:
        return "WIFI_REASON_INVALID_FTE";
    case WIFI_REASON_TRANSMISSION_LINK_ESTABLISH_FAILED:
        return "WIFI_REASON_TRANSMISSION_LINK_ESTABLISH_FAILED";
    case WIFI_REASON_ALTERATIVE_CHANNEL_OCCUPIED:
        return "WIFI_REASON_ALTERATIVE_CHANNEL_OCCUPIED";
    case WIFI_REASON_BEACON_TIMEOUT:
        return "WIFI_REASON_BEACON_TIMEOUT";
    case WIFI_REASON_NO_AP_FOUND:
        return "WIFI_REASON_NO_AP_FOUND";
    case WIFI_REASON_AUTH_FAIL:
        return "WIFI_REASON_AUTH_FAIL";
    case WIFI_REASON_ASSOC_FAIL:
        return "WIFI_REASON_ASSOC_FAIL";
    case WIFI_REASON_HANDSHAKE_TIMEOUT:
        return "WIFI_REASON_HANDSHAKE_TIMEOUT";
    case WIFI_REASON_CONNECTION_FAIL:
        return "WIFI_REASON_CONNECTION_FAIL";
    case WIFI_REASON_AP_TSF_RESET:
        return "WIFI_REASON_AP_TSF_RESET";
    case WIFI_REASON_ROAMING:
        return "WIFI_REASON_ROAMING";
    case WIFI_REASON_ASSOC_COMEBACK_TIME_TOO_LONG:
        return "WIFI_REASON_ASSOC_COMEBACK_TIME_TOO_LONG";
    case WIFI_REASON_SA_QUERY_TIMEOUT:
        return "WIFI_REASON_SA_QUERY_TIMEOUT";
    case WIFI_REASON_NO_AP_FOUND_W_COMPATIBLE_SECURITY:
        return "WIFI_REASON_NO_AP_FOUND_W_COMPATIBLE_SECURITY";
    case WIFI_REASON_NO_AP_FOUND_IN_AUTHMODE_THRESHOLD:
        return "WIFI_REASON_NO_AP_FOUND_IN_AUTHMODE_THRESHOLD";
    case WIFI_REASON_NO_AP_FOUND_IN_RSSI_THRESHOLD:
        return "WIFI_REASON_NO_AP_FOUND_IN_RSSI_THRESHOLD";
    }
    return "UNKNOWN";
}


static void sta_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    }

    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED){
        wifi_event_sta_disconnected_t *wifi_event_sta_disconnected = event_data;
        ESP_LOGW("WIFI", "DISCONNECTED %d: %s", wifi_event_sta_disconnected->reason,
                                                get_wifi_disconnection_string(wifi_event_sta_disconnected->reason));
        if (attempt_reconnect) {
             if (wifi_event_sta_disconnected->reason == WIFI_REASON_NO_AP_FOUND ||
                wifi_event_sta_disconnected->reason == WIFI_REASON_ASSOC_LEAVE ||
                wifi_event_sta_disconnected->reason == WIFI_REASON_AUTH_EXPIRE)
            {
                if (s_retry_num < EXAMPLE_ESP_MAXIMUN_RETRY) {
                    esp_wifi_connect();
                    s_retry_num++;
                    ESP_LOGI("WIFI", "Retry to connect to the AP");
                } else {
                    xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
                    ESP_LOGE("WIFI", "Connect to the AP fail");                    
                }
            }       
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP){
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI("WIFI", "got ip: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}


static void ap_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI("WIFI", "station "MACSTR" join, AID=%d", 
                 MAC2STR(event->mac), event->aid);
    } else if (event_id ==  WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI("WIFI", "station "MACSTR" leave, AID=%d", 
                 MAC2STR(event->mac), event->aid);
    }
}


char *getAuthModeName(wifi_auth_mode_t wifi_auth_mode){
    switch (wifi_auth_mode){
        case WIFI_AUTH_OPEN:
            return "WIFI_AUTH_OPEN";
        case WIFI_AUTH_WEP:
            return "WIFI_AUTH_WEP";
        case WIFI_AUTH_WPA_PSK:
            return "WIFI_AUTH_WPA_PSK";
        case WIFI_AUTH_WPA2_PSK:
            return "WIFI_AUTH_WPA2_PSK";
        case WIFI_AUTH_WPA_WPA2_PSK:
            return "WIFI_AUTH_WPA_WPA2_PSK";
        case WIFI_AUTH_WPA2_ENTERPRISE:
            return "WIFI_AUTH_WPA2_ENTERPRISE";
        case WIFI_AUTH_WPA3_PSK:
            return "WIFI_AUTH_WPA3_PSK";
        case WIFI_AUTH_WPA2_WPA3_PSK:
            return "WIFI_AUTH_WPA2_WPA3_PSK";
        case WIFI_AUTH_WAPI_PSK:
            return "WIFI_AUTH_WAPI_PSK";
        case WIFI_AUTH_OWE:
            return "WIFI_AUTH_OWE";
        case WIFI_AUTH_MAX:
            return "WIFI_AUTH_MAX";
        default:
            return "NOT FOUND";
    }
}



void wifi_scan() {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));

    wifi_scan_config_t wifi_scan_config = {
        // .show_hidden = true, 
        // .channel = 2,
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_scan_start(&wifi_scan_config, true));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));

    ESP_LOGI(TAG, "Total APS scanned = %u", ap_count);

    for (int i=0; (i<DEFAULT_SCAN_LIST_SIZE) && (i<ap_count); i++){
        ESP_LOGI(TAG, "SSID \t\t%s", ap_info[i].ssid);
        ESP_LOGI(TAG, "RSSI \t\t%d", ap_info[i].rssi);
        ESP_LOGI(TAG, "Channel \t%d", ap_info[i].primary);
        ESP_LOGI(TAG, "Authmode \t%s\n", getAuthModeName(ap_info[i].authmode));
    }

    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());
    ESP_ERROR_CHECK(esp_event_loop_delete_default());
    esp_netif_destroy(sta_netif);
}


void wifi_init(void){
    ESP_ERROR_CHECK(esp_netif_init());                   // initialize tcp/ip adapter
    ESP_ERROR_CHECK(esp_event_loop_create_default());    // create default event loop

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); // initialize wifi configuration
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));                // initialize wifi with configuration
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
}


void wifi_init_sta(){
   
    wifi_init();
    attempt_reconnect = true;
    s_wifi_event_group = xEventGroupCreate();
    esp_netif = esp_netif_create_default_wifi_sta();

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &sta_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &sta_event_handler, NULL));
    

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

    wifi_semaphore = xSemaphoreCreateBinary();
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI("WIFI", "Connected to app SSID: %s password:%s", 
                EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
        xSemaphoreGive(wifi_semaphore);
    } else if (bits & WIFI_FAIL_BIT){
        ESP_LOGI("WIFI", "Failed to connected to app SSID: %s password:%s", 
                EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGI("WIFI", "UNEXPECTED EVENT");
    }

}


void wifi_connect_ap(const char *ssid, const char *pass){
    wifi_init();
    ESP_LOGI("WIFI", "ESP_WIFI_MODE_AP");
    esp_netif = esp_netif_create_default_wifi_ap();

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,         // register event handler instance
                                                        ESP_EVENT_ANY_ID,   // event id
                                                        &ap_event_handler,  // event handler
                                                        NULL,               // no event handler arg
                                                        NULL));             // no event handler instance

    wifi_config_t wifi_config = {};
    strncpy((char *) wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid) - 1);
    strncpy((char *) wifi_config.ap.password, pass, sizeof(wifi_config.ap.password) - 1);
    wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    wifi_config.ap.max_connection = 4;
    wifi_config.ap.beacon_interval = 100;
    wifi_config.ap.channel = 1;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI("WIFI", "wifi_init_softap finished. SSID: %s password: %s channel: %d",
             wifi_config.ap.ssid, wifi_config.ap.password, wifi_config.ap.channel);
}


void wifi_disconnect(void) {
    attempt_reconnect = false;
    esp_wifi_stop();
    esp_netif_destroy(esp_netif);
}