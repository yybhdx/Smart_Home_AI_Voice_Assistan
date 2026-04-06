/**
 * @file main.c
 * @brief ESP32-S3 智能家居网关 - 串口转华为云 MQTT 转发程序
 * 
 * 设计思路：
 * 1. 使用 UART2 接收来自 STM32 的传感器 JSON 数据
 * 2. 建立 Wi-Fi 连接并接入华为云 IoT DA 平台
 * 3. 通过异步事件回调处理网络状态，通过 FreeRTOS 任务处理串口数据
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"  // ESP32 原生运行 FreeRTOS
#include "freertos/task.h"      // 任务管理
#include "esp_system.h"
#include "esp_wifi.h"           // Wi-Fi 驱动
#include "esp_event.h"          // 事件循环库（核心：处理异步通知）
#include "esp_log.h"            // 日志系统（类似 printf 但带颜色和等级）
#include "nvs_flash.h"          // 非易失性存储（Wi-Fi 配置必需）
#include "driver/uart.h"        // UART 驱动
#include "mqtt_client.h"        // MQTT 客户端库

// =================== 配置参数 ===================
#define WIFI_SSID      "jf"
#define WIFI_PASS      "a8pswys108"

/* 华为云 MQTT 接入地址 (注意前缀是 mqtt://) */
#define HUAWEI_MQTT_URL       "mqtt://52e4e17470.st1.iotda-device.cn-east-3.myhuaweicloud.com"
#define HUAWEI_MQTT_PORT      1883
#define HUAWEI_CLIENT_ID      "69ce6bd8e094d615922d9e08_Smart_Home_0_0_2026040213"
#define HUAWEI_USERNAME       "69ce6bd8e094d615922d9e08_Smart_Home"
#define HUAWEI_PASSWORD       "b859e0be5c2f5ed05ec764914e485d1204b37bb341afe10c91d4a9c8dae43a19"
/* 属性上报 Topic */
#define HUAWEI_PUB_TOPIC      "$oc/devices/69ce6bd8e094d615922d9e08_Smart_Home/sys/properties/report"

/* 串口引脚定义 (ESP32 引脚可以映射，不同于 STM32 的固定引脚) */
#define STM32_UART_PORT   UART_NUM_2  // 使用硬件串口 2
#define STM32_TX_PIN      (17)        // ESP32 TX
#define STM32_RX_PIN      (18)        // ESP32 RX
#define UART_BUF_SIZE     (1024)      // 接收缓冲区大小

static const char *TAG = "SMART_HOME_S3";  // 日志标签，方便过滤
esp_mqtt_client_handle_t mqtt_client = NULL; // MQTT 句柄，类似 STM32 的 Handle
bool is_mqtt_connected = false;            // MQTT 连接状态标志

/**
 * @brief MQTT 事件回调函数
 * @note  ESP-IDF 采用异步通知，当连接成功、断开或收到消息时，此函数被系统调用
 * @param event_data 事件数据结构体，包含事件 ID、收到的数据等
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Huawei Cloud MQTT Connected!");
            is_mqtt_connected = true;
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "Huawei Cloud MQTT Disconnected!");
            is_mqtt_connected = false;
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            break;
    }
}

/**
 * @brief 系统事件回调函数 (处理 Wi-Fi 和 IP 状态)
 * @note  类似于 STM32 的中断服务函数，但运行在特定的事件任务中
 */
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        // Wi-Fi 驱动启动成功后，开始尝试连接热点
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        // 断开连接后自动重连
        ESP_LOGW(TAG, "WiFi lost, retrying...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        // 获取到 IP 地址，说明 Wi-Fi 真正连通
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Connected! IP:" IPSTR, IP2STR(&event->ip_info.ip));
    }
}

/**
 * @brief 初始化 Wi-Fi (STA 模式)
 * @note  ESP32 的网络初始化较多，包括 TCP/IP 堆栈初始化、事件循环创建等
 */
void wifi_init_sta(void) {
    // 1. 初始化底层网络接口 (LwIP)
    esp_netif_init();
    // 2. 创建系统默认事件循环 (用于处理回调)
    esp_event_loop_create_default();
    // 3. 创建 STA 模式的默认网口实例
    esp_netif_create_default_wifi_sta();

    // 4. 初始化 Wi-Fi 驱动
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    // 5. 注册事件处理函数 (类似于 STM32 注册中断回调)
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL);

    // 6. 配置 Wi-Fi 参数
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    
    // 7. 启动 Wi-Fi
    esp_wifi_start();
}

/**
 * @brief 初始化并启动 MQTT 客户端
 */
void mqtt_app_start(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = HUAWEI_MQTT_URL,
        .broker.address.port = HUAWEI_MQTT_PORT,
        .credentials.client_id = HUAWEI_CLIENT_ID,
        .credentials.username = HUAWEI_USERNAME,
        .credentials.authentication.password = HUAWEI_PASSWORD,
    };
    // 根据配置初始化句柄
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    // 注册 MQTT 事件回调
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    // 启动 MQTT 客户端任务
    esp_mqtt_client_start(mqtt_client);
}

/**
 * @brief FreeRTOS 任务：接收 STM32 串口数据并转发至 MQTT
 * @param arg 任务参数 (本例未使用)
 * @note  这是一个死循环任务，类似于 STM32 的 while(1)，但它会在 uart_read_bytes 处阻塞，交出 CPU 使用权
 */
void stm32_rx_task(void *arg) {
    uint8_t *data = (uint8_t *) malloc(UART_BUF_SIZE); // 动态分配内存存放串口数据
    while (1) {
        // 关键：读取串口数据。参数 20/portTICK_PERIOD_MS 意味着如果在 20ms 内没收到数据则继续，起到“分帧”作用
        int len = uart_read_bytes(STM32_UART_PORT, data, UART_BUF_SIZE - 1, 20 / portTICK_PERIOD_MS);
        if (len > 0) {
            data[len] = '\0'; // 补上字符串结束符
            char *payload = (char *)data;
            ESP_LOGI(TAG, "Recv from STM32: %s", payload);
            
            // 数据校验与发布：仅在 MQTT 连接且收到的是 JSON ({ 开头) 时发布
            if (is_mqtt_connected && payload[0] == '{') {
                // 发布消息。参数：Topic, 数据, 长度(0代表自动算), QoS, 是否保留
                int msg_id = esp_mqtt_client_publish(mqtt_client, HUAWEI_PUB_TOPIC, payload, 0, 1, 0);
                ESP_LOGI(TAG, "Sent to Huawei Cloud, msg_id=%d", msg_id);
            } else if (!is_mqtt_connected) {
                ESP_LOGW(TAG, "MQTT not connected, data dropped.");
            }
        }
        // 释放 CPU 10ms，让其他低优先级任务运行
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    free(data); // 理论上不会运行到这里
}

/**
 * @brief 初始化 UART 硬件驱动
 * @note  ESP32 的 UART 引脚可以自由选择 GPIO，这是与 STM32 的最大区别
 */
void uart_init(void) {
    uart_config_t uart_config = {
        .baud_rate = 115200,                // 波特率
        .data_bits = UART_DATA_8_BITS,      // 8位数据位
        .parity    = UART_PARITY_DISABLE,   // 无校验
        .stop_bits = UART_STOP_BITS_1,      // 1个停止位
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    // 1. 安装驱动：串口号，接收缓存，发送缓存，队列长度，中断标志
    uart_driver_install(STM32_UART_PORT, UART_BUF_SIZE * 2, 0, 0, NULL, 0);
    // 2. 配置参数
    uart_param_config(STM32_UART_PORT, &uart_config);
    // 3. 设置引脚：串口号，TX，RX，RTS，CTS
    uart_set_pin(STM32_UART_PORT, STM32_TX_PIN, STM32_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

/**
 * @brief ESP32 程序入口 (相当于 STM32 的 main)
 * @note  app_main 本身也是一个 FreeRTOS 任务
 */
void app_main(void) {
    // 1. 初始化 NVS（存储 Wi-Fi 配置等信息的基础）
    // ESP32 很多底层操作需要用到这块 Flash
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. 初始化串口
    uart_init();
    ESP_LOGI(TAG, "UART Init Completed.");

    // 3. 初始化 Wi-Fi
    wifi_init_sta();
    
    // 4. 等待一会儿让 Wi-Fi 连上后再启动 MQTT
    // STM32 习惯用 HAL_Delay，ESP32 用 vTaskDelay
    vTaskDelay(pdMS_TO_TICKS(5000)); 
    
    // 5. 启动 MQTT 客户端
    mqtt_app_start();

    // 6. 创建专门的 FreeRTOS 任务用于串口监控
    // 参数：任务函数，任务名，栈深度(字节)，参数，优先级，任务句柄
    xTaskCreate(stm32_rx_task, "stm32_rx_task", 4096, NULL, 10, NULL);
    
    // app_main 函数结束后，后台任务（Wi-Fi, MQTT, stm32_rx_task）会继续运行
}