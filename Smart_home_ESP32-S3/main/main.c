#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "audio_manager.h"
#include "wifi_mqtt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define STM32_UART_PORT UART_NUM_2
#define STM32_TX_PIN (4)
#define STM32_RX_PIN (5)
#define UART_BUF_SIZE (1024)
#define HUAWEI_PUB_TOPIC "$oc/devices/69ce6bd8e094d615922d9e08_Smart_Home/sys/properties/report"

static const char *TAG = "MAIN_APP";

void uart_init(void) {
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
    uart_set_pin(STM32_UART_PORT, STM32_TX_PIN, STM32_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void stm32_task(void *arg) {
    uint8_t *temp_data = malloc(UART_BUF_SIZE);
    char latest_valid_json[UART_BUF_SIZE] = {0};
    bool has_new_data = false;
    
    // 记录上次上传的时间
    TickType_t last_upload_tick = xTaskGetTickCount();

    while (1) {
        // 1. 尝试读取串口数据（50ms超时）
        int len = uart_read_bytes(STM32_UART_PORT, temp_data, UART_BUF_SIZE - 1, pdMS_TO_TICKS(50));
        
        if (len > 0) {
            temp_data[len] = '\0';
            // 简单的校验：如果是以 { 开头，认为是有效的 JSON
            if (temp_data[0] == '{') {
                // 将最新数据保存到缓冲区
                strncpy(latest_valid_json, (char *)temp_data, UART_BUF_SIZE - 1);
                has_new_data = true;
                ESP_LOGD(TAG, "Received latest data: %s", latest_valid_json);
            }
        }

        // 2. 检查是否到了 1000ms 的上传周期
        TickType_t current_tick = xTaskGetTickCount();
        if (has_new_data && (current_tick - last_upload_tick >= pdMS_TO_TICKS(1000))) {
            
            if (wifi_mqtt_is_connected()) {
                // 执行上传
                wifi_mqtt_publish(HUAWEI_PUB_TOPIC, latest_valid_json);
                ESP_LOGI(TAG, "Uploaded to Huawei Cloud (Interval 1s): %s", latest_valid_json);
                
                // 更新时间戳并复位标志
                last_upload_tick = current_tick;
                has_new_data = false; 
            } else {
                ESP_LOGW(TAG, "Waiting for MQTT connection...");
            }
        }

        // 3. 适当的小延迟，防止死循环占用过多CPU
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void) {
    // 1. NVS 初始化
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. 模块初始化
    audio_manager_init();
    uart_init();
    wifi_mqtt_init();

    // 3. 启动任务
    audio_manager_start_loopback();
    // 调高 stm32_task 优先级或根据需要调整
    xTaskCreate(stm32_task, "stm32_task", 4096, NULL, 10, NULL);

    // 4. 启动 MQTT（内部会有连接逻辑）
    wifi_mqtt_start();
}