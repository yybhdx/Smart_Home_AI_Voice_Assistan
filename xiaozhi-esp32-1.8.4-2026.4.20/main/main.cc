#include <esp_log.h>
#include <esp_err.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <driver/gpio.h>
#include <esp_event.h>

#include "application.h"
#include "system_info.h"
#include "stm32_uart.h"
#include "freertos/task.h"

#define TAG "main"

extern "C" void app_main(void)
{
    // 1. 初始化事件循环：这是 ESP-IDF 的核心机制，用于处理 WiFi、MQTT 等各种系统事件
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 2. 初始化 NVS (Non-volatile Storage)：非易失性存储，用于保存 WiFi 名称和密码等配置
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // 如果存储空间坏了或版本不对，就擦除重来
        ESP_LOGW(TAG, "正在擦除 NVS 闪存以修复损坏...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 3. 核心桥梁线程：创建一个名为 "stm32_task" 的后台线程
    // 该线程负责通过串口(UART)接收 STM32 发来的数据，并上传到华为云
    // 参数说明：执行函数名、线程名、堆栈大小(4096字节)、参数、优先级(5)、返回句柄
    xTaskCreate(stm32_uart_task, "stm32_task", 4096, NULL, 5, NULL);

    // 4. 启动“小智”语音助手应用程序：
    // 这个函数会进入一个死循环，负责处理语音唤醒、对话和音频播放
    Application::GetInstance().Start();
}
