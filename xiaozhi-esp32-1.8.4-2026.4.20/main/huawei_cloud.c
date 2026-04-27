#include "huawei_cloud.h"
#include "esp_event.h"
#include "esp_log.h"
#include "mqtt_client.h"

#define HUAWEI_MQTT_URL     "mqtt://52e4e17470.st1.iotda-device.cn-east-3.myhuaweicloud.com"
#define HUAWEI_CLIENT_ID    "69ce6bd8e094d615922d9e08_Smart_Home_0_0_2026042700"
#define HUAWEI_USERNAME     "69ce6bd8e094d615922d9e08_Smart_Home"
#define HUAWEI_PASSWORD     "f9c112cc3e69e14e2b9d6dcca1f14735406fd6dc6746b6dcaa1cf75d8736e9e6"

static const char *TAG = "HUAWEI_CLOUD";
static esp_mqtt_client_handle_t client = NULL;
static bool connected = false;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            connected = true;
            ESP_LOGI(TAG, "Huawei Cloud MQTT Connected");
            break;
        case MQTT_EVENT_DISCONNECTED:
            connected = false;
            ESP_LOGW(TAG, "Huawei Cloud MQTT Disconnected");
            break;
        default:
            break;
    }
}

void huawei_cloud_start(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = HUAWEI_MQTT_URL,
        .broker.address.port = 1883,
        .credentials.client_id = HUAWEI_CLIENT_ID,
        .credentials.username = HUAWEI_USERNAME,
        .credentials.authentication.password = HUAWEI_PASSWORD,
    };
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

bool huawei_cloud_is_connected(void) {
    return connected;
}

int huawei_cloud_publish(const char *topic, const char *data) {
    if (connected && client) {
        return esp_mqtt_client_publish(client, topic, data, 0, 1, 0);
    }
    return -1;
}
