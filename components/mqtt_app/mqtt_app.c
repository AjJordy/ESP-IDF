#include "esp_log.h"
#include "mqtt_client.h"
#include "freertos/event_groups.h"

#include "mqtt_app.h"

static const char *TAG = "MQTT";


static esp_mqtt_client_handle_t mqtt_client; // handler for mqtt client
static EventGroupHandle_t status_mqtt_event_group;
#define MQTT_CONNECTED BIT0


static void mqtt_event_handler(void *event_handler_arg, esp_event_base_t event_base, 
                               int32_t event_id, void *event_data) 
{
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t) event_id)
    {
    case MQTT_EVENT_CONNECTED:
        xEventGroupSetBits(status_mqtt_event_group, MQTT_CONNECTED);
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        break;
    case MQTT_EVENT_DISCONNECTED:
        xEventGroupClearBits(status_mqtt_event_group, MQTT_CONNECTED);
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED");
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED");
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED");
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("topic: %.*s\n", event->topic_len, event->topic);
        printf("message: %.*s\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "ERROR: %s", strerror(event->error_handle->esp_transport_sock_errno));
        break;
    default:
        break;
    }
}


void mqtt_app_start(){
    // MQTT connection
    status_mqtt_event_group = xEventGroupCreate();
    esp_mqtt_client_config_t esp_mqtt_client_config = {
        .network.disable_auto_reconnect = false,
        .session.keepalive = 30,
        .broker.address.uri = "mqtt://mqtt.eclipseprojects.io",
        .broker.address.port = 1883,
        .session.last_will = {
            .topic = "asdfg/status",
            .msg = "Offline",
            .msg_len = strlen("Offline"),
            .qos = 1,
            .retain = 0,
        }
    };
    mqtt_client = esp_mqtt_client_init(&esp_mqtt_client_config);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
    ESP_LOGI(TAG, "MQTT start");
    mqtt_app_publish("asdfg/status", "Online", 1, 0);
}


void mqtt_app_subscribe(char *topic, int qos){
    int msg_id = esp_mqtt_client_subscribe(mqtt_client, topic, qos);
    ESP_LOGI(TAG, "Subscribed, msg_id=%d", msg_id);
}


void mqtt_app_unsubscribe(char *topic){
    int msg_id = esp_mqtt_client_unsubscribe(mqtt_client, topic);
    ESP_LOGI(TAG, "Unsubscribed, msg_id=%d", msg_id);
}

void mqtt_app_publish(char *topic, char* payload, int qos, bool retain) {
    int msg_id = esp_mqtt_client_publish(mqtt_client, topic, payload, strlen(payload), qos, retain);
    ESP_LOGI(TAG, "Sent publish successful, msg_id=%d", msg_id);
}

int mqtt_is_connected() {
    EventBits_t bits = xEventGroupGetBits(status_mqtt_event_group);
    if (bits & MQTT_CONNECTED) {
        return 1;
    }
    return 0;
}


void mqtt_app_stop(void) {
    esp_mqtt_client_stop(mqtt_client);
    ESP_LOGI(TAG, "MQTT stoped");
}