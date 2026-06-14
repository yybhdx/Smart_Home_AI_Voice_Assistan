#include "stm32_uart.h"
#include "huawei_cloud.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

// 定义串口连接的引脚 (ESP32-S3 与 STM32 的物理连接线)
// UART1 on GPIO 8/9 to avoid conflict with xiaozhi mic (GPIO 4/5/6)
#define STM32_UART_PORT  UART_NUM_1
#define STM32_TX_PIN     8  // ESP32 发送给 STM32 的引脚
#define STM32_RX_PIN     9  // ESP32 接收 STM32 数据的引脚
#define UART_BUF_SIZE    1024
// 华为云的数据上报主题（Topic），云平台靠这个地址来识别上报的数据
#define HUAWEI_PUB_TOPIC "$oc/devices/69ce6bd8e094d615922d9e08_Smart_Home/sys/properties/report"

static const char *TAG = "STM32_UART";

/**
 * 串口初始化：配置波特率 115200，并绑定引脚
 */
static void uart_init(void) {
    uart_config_t uart_config = {
        .baud_rate = 115200,      // 波特率必须与 STM32 端一致
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_driver_install(STM32_UART_PORT, UART_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(STM32_UART_PORT, &uart_config);
    uart_set_pin(STM32_UART_PORT, STM32_TX_PIN, STM32_RX_PIN,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

/**
 * 核心任务逻辑：
 * 1. 等待 WiFi 连接成功
 * 2. 启动华为云连接
 * 3. 循环监听串口数据
 * 4. 收到 JSON 数据后上报华为云
 */
void stm32_uart_task(void *arg) {
    // A. 循环检查 WiFi 状态，没联网就不往下走
    // Wait for xiaozhi's WiFi to connect
    ESP_LOGI(TAG, "Waiting for WiFi connection...");
    wifi_ap_record_t ap_info;
    while (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    ESP_LOGI(TAG, "WiFi connected, starting Huawei Cloud MQTT");

    // B. 启动华为云 MQTT 通信协议
    huawei_cloud_start();
    uart_init();
    ESP_LOGI(TAG, "UART1 initialized on GPIO%d/GPIO%d @ 115200", STM32_TX_PIN, STM32_RX_PIN);

    uint8_t *temp_data = (uint8_t *)malloc(UART_BUF_SIZE);
    char latest_valid_json[UART_BUF_SIZE] = {0};
    bool has_new_data = false;
    TickType_t last_upload_tick = xTaskGetTickCount(); // 记录上次上传的时间点

    while (1) {
        // C. 读取串口数据，等待时长为 50 毫秒
        int len = uart_read_bytes(STM32_UART_PORT, temp_data, UART_BUF_SIZE - 1, pdMS_TO_TICKS(50));

        if (len > 0) {
            temp_data[len] = '\0';
            // 简单的校验：如果是以 '{' 开头，我们认为它是一个 JSON 数据包
            if (temp_data[0] == '{') {
                strncpy(latest_valid_json, (char *)temp_data, UART_BUF_SIZE - 1);
                has_new_data = true;
                ESP_LOGD(TAG, "Received: %s", latest_valid_json);
            }
        }

        // D. 控制上传频率：每 1000 毫秒 (1秒) 上传一次，避免云端压力过大
        TickType_t current_tick = xTaskGetTickCount();
        if (has_new_data && (current_tick - last_upload_tick >= pdMS_TO_TICKS(1000))) {
            if (huawei_cloud_is_connected()) {
                // 将 JSON 数据包原封不动地发给华为云
                huawei_cloud_publish(HUAWEI_PUB_TOPIC, latest_valid_json);
                ESP_LOGI(TAG, "Uploaded: %s", latest_valid_json);
                last_upload_tick = current_tick;
                has_new_data = false;
            } else {
                ESP_LOGW(TAG, "Waiting for MQTT connection...");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // 给 CPU 一点喘息时间
    }
}
