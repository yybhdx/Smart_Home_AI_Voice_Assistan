#include "stm32_uart.h"
#include "huawei_cloud.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

// UART1 on GPIO 8/9 to avoid conflict with xiaozhi mic (GPIO 4/5/6)
#define STM32_UART_PORT  UART_NUM_1
#define STM32_TX_PIN     8
#define STM32_RX_PIN     9
#define UART_BUF_SIZE    1024
#define HUAWEI_PUB_TOPIC "$oc/devices/69ce6bd8e094d615922d9e08_Smart_Home/sys/properties/report"

static const char *TAG = "STM32_UART";

static void uart_init(void) {
    uart_config_t uart_config = {
        .baud_rate = 115200,
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

void stm32_uart_task(void *arg) {
    // Wait for xiaozhi's WiFi to connect
    ESP_LOGI(TAG, "Waiting for WiFi connection...");
    wifi_ap_record_t ap_info;
    while (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    ESP_LOGI(TAG, "WiFi connected, starting Huawei Cloud MQTT");

    huawei_cloud_start();
    uart_init();
    ESP_LOGI(TAG, "UART1 initialized on GPIO%d/GPIO%d @ 115200", STM32_TX_PIN, STM32_RX_PIN);

    uint8_t *temp_data = malloc(UART_BUF_SIZE);
    char latest_valid_json[UART_BUF_SIZE] = {0};
    bool has_new_data = false;
    TickType_t last_upload_tick = xTaskGetTickCount();

    while (1) {
        int len = uart_read_bytes(STM32_UART_PORT, temp_data, UART_BUF_SIZE - 1, pdMS_TO_TICKS(50));

        if (len > 0) {
            temp_data[len] = '\0';
            if (temp_data[0] == '{') {
                strncpy(latest_valid_json, (char *)temp_data, UART_BUF_SIZE - 1);
                has_new_data = true;
                ESP_LOGD(TAG, "Received: %s", latest_valid_json);
            }
        }

        TickType_t current_tick = xTaskGetTickCount();
        if (has_new_data && (current_tick - last_upload_tick >= pdMS_TO_TICKS(1000))) {
            if (huawei_cloud_is_connected()) {
                huawei_cloud_publish(HUAWEI_PUB_TOPIC, latest_valid_json);
                ESP_LOGI(TAG, "Uploaded: %s", latest_valid_json);
                last_upload_tick = current_tick;
                has_new_data = false;
            } else {
                ESP_LOGW(TAG, "Waiting for MQTT connection...");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
