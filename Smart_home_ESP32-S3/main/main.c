#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "mqtt_client.h"

// --- 配置参数 ---
#define WIFI_SSID      "jf"
#define WIFI_PASS      "a8pswys108"

// 华为云 MQTT 参数
#define HUAWEI_MQTT_URL       "mqtt://52e4e17470.st1.iotda-device.cn-east-3.myhuaweicloud.com"
#define HUAWEI_MQTT_PORT      1883
#define HUAWEI_CLIENT_ID      "69ce6bd8e094d615922d9e08_Smart_Home_0_0_2026040213"
#define HUAWEI_USERNAME       "69ce6bd8e094d615922d9e08_Smart_Home"
#define HUAWEI_PASSWORD       "b859e0be5c2f5ed05ec764914e485d1204b37bb341afe10c91d4a9c8dae43a19"
#define HUAWEI_PUB_TOPIC      "$oc/devices/69ce6bd8e094d615922d9e08_Smart_Home/sys/properties/report"

#define STM32_UART_PORT   UART_NUM_2
#define STM32_TX_PIN      (17)
#define STM32_RX_PIN      (18)
#define UART_BUF_SIZE     (1024)

static const char *TAG = "SMART_HOME_S3";
esp_mqtt_client_handle_t mqtt_client = NULL;
bool is_mqtt_connected = false;

// --- MQTT 事件处理 ---
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Huawei Cloud MQTT Connected!");
            is_mqtt_connected = true;
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "Huawei Cloud MQTT Disconnected!");
            is_mqtt_connected = false;
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            break;
    }
}

// --- WiFi 事件处理 (保持不变) ---
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Connected! IP:" IPSTR, IP2STR(&event->ip_info.ip));
    }
}

// --- 初始化 WiFi (保持不变) ---
void wifi_init_sta(void) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL);
    wifi_config_t wifi_config = { .sta = { .ssid = WIFI_SSID, .password = WIFI_PASS } };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
}

// --- 初始化 MQTT ---
void mqtt_app_start(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = HUAWEI_MQTT_URL,
        .broker.address.port = HUAWEI_MQTT_PORT,
        .credentials.client_id = HUAWEI_CLIENT_ID,
        .credentials.username = HUAWEI_USERNAME,
        .credentials.authentication.password = HUAWEI_PASSWORD,
    };
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

// --- UART 接收并转发任务 ---
void stm32_rx_task(void *arg) {
    uint8_t *data = (uint8_t *) malloc(UART_BUF_SIZE);
    while (1) {
        int len = uart_read_bytes(STM32_UART_PORT, data, UART_BUF_SIZE - 1, 20 / portTICK_PERIOD_MS);
        if (len > 0) {
            data[len] = '\0';
            char *payload = (char *)data;
            ESP_LOGI(TAG, "Recv from STM32: %s", payload);
            
            // 如果 MQTT 已连接，且收到的是 JSON 数据，则转发
            if (is_mqtt_connected && payload[0] == '{') {
                int msg_id = esp_mqtt_client_publish(mqtt_client, HUAWEI_PUB_TOPIC, payload, 0, 1, 0);
                ESP_LOGI(TAG, "Sent to Huawei Cloud, msg_id=%d", msg_id);
            } else if (!is_mqtt_connected) {
                ESP_LOGW(TAG, "MQTT not connected, data dropped.");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    free(data);
}

// --- 初始化 UART (保持不变) ---
void uart_init(void) {
    uart_config_t uart_config = {
        .baud_rate = 115200, .data_bits = UART_DATA_8_BITS, .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1, .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, .source_clk = UART_SCLK_DEFAULT,
    };
    uart_driver_install(STM32_UART_PORT, UART_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(STM32_UART_PORT, &uart_config);
    uart_set_pin(STM32_UART_PORT, STM32_TX_PIN, STM32_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    uart_init();
    wifi_init_sta();
    
    // 等待 WiFi 连接成功后（简单延时或根据事件）启动 MQTT
    vTaskDelay(pdMS_TO_TICKS(5000)); 
    mqtt_app_start();

    xTaskCreate(stm32_rx_task, "stm32_rx_task", 4096, NULL, 10, NULL);
}