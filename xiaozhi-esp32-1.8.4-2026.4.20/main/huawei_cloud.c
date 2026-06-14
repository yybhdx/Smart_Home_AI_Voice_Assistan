#include "huawei_cloud.h"
#include "esp_event.h"
#include "esp_log.h"
#include "mqtt_client.h"

// --- 华为云连接凭证 ---
// 这些是在华为云设备详情页获取的，相当于该设备在互联网上的“身份证”
#define HUAWEI_MQTT_URL     "mqtt://52e4e17470.st1.iotda-device.cn-east-3.myhuaweicloud.com"
#define HUAWEI_CLIENT_ID    "69ce6bd8e094d615922d9e08_Smart_Home_0_0_2026042700"
#define HUAWEI_USERNAME     "69ce6bd8e094d615922d9e08_Smart_Home"
#define HUAWEI_PASSWORD     "f9c112cc3e69e14e2b9d6dcca1f14735406fd6dc6746b6dcaa1cf75d8736e9e6"

static const char *TAG = "HUAWEI_CLOUD";
static esp_mqtt_client_handle_t client = NULL;
static bool connected = false;

/**
 * 事件回调函数：当连接成功或断开时，系统会自动调用这个函数
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            connected = true;
            ESP_LOGI(TAG, "✅ 华为云连接成功！");
            break;
        case MQTT_EVENT_DISCONNECTED:
            connected = false;
            ESP_LOGW(TAG, "❌ 与华为云断开连接");
            break;
        default:
            break;
    }
}

/**
 * 启动 MQTT 客户端
 */
void huawei_cloud_start(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = HUAWEI_MQTT_URL,
        .broker.address.port = 1883, // MQTT 默认不加密端口
        .credentials.client_id = HUAWEI_CLIENT_ID,
        .credentials.username = HUAWEI_USERNAME,
        .credentials.authentication.password = HUAWEI_PASSWORD,
    };
    client = esp_mqtt_client_init(&mqtt_cfg);
    // 注册事件处理程序
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    // 开始运行后台连接任务
    esp_mqtt_client_start(client);
}

bool huawei_cloud_is_connected(void) {
    return connected;
}

/**
 * 发布数据（上报数据）
 * @param topic 发送到哪个频道
 * @param data  发送的内容（JSON 字符串）
 */
int huawei_cloud_publish(const char *topic, const char *data) {
    if (connected && client) {
        // QoS 为 1，确保云端至少收到一次
        return esp_mqtt_client_publish(client, topic, data, 0, 1, 0);
    }
    return -1;
}
