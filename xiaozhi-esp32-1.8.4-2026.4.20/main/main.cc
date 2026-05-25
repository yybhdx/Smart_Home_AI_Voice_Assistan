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
    // 初始化默认事件循环
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 初始化NVS闪存以进行WiFi配置
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "Erasing NVS flash to fix corruption");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 启动STM32UART+华为云任务
    xTaskCreate(stm32_uart_task, "stm32_task", 4096, NULL, 5, NULL);

    // 启动小智应用程序(在此处阻塞)
    Application::GetInstance().Start();
}
