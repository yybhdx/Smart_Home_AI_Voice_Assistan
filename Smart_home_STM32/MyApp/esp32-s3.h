/**
 * @file    esp32-s3.h
 * @brief   ESP32-S3 WiFi 模块通信驱动头文件（华为云 IoT 数据上报）
 * @details 定义 ESP32-S3 通信所需的 WiFi 连接参数、华为云 IoT 平台 MQTT 连接参数，
 *          以及数据上报任务的函数声明。
 *
 * @note    数据上报流程：
 *          STM32 采集传感器数据 -> UART3 发送 JSON -> ESP32-S3 接收 ->
 *          MQTT 协议 -> 华为云 IoTDA 平台
 *
 * @note    MQTT 通信参数说明：
 *          - 使用 TLS 加密连接，端口 8883
 *          - 设备认证方式：设备密钥（Secret）
 *          - 上报 Topic：$oc/devices/{device_id}/sys/properties/report
 */

#ifndef ESP32_S3_H
#define ESP32_S3_H

#include <stdint.h>

/* ======================== WiFi 连接参数 ======================== */

#define WIFI_SSID "jifei"      ///< WiFi 热点名称（SSID）
#define WIFI_PWD "12345678"    ///< WiFi 密码

/* ======================== 华为云 IoT 平台 MQTT 连接参数 ======================== */

#define HUAWEI_DEVICE_ID "69ce6bd8e094d615922d9e08_Smart_Home"                   ///< 设备ID，在华为云 IoTDA 控制台注册设备时生成
#define HUAWEI_MQTT_USERNAME "69ce6bd8e094d615922d9e08_Smart_Home"               ///< MQTT 用户名，通常与设备ID相同
#define HUAWEI_MQTT_PASSWORD "b859e0be5c2f5ed05ec764914e485d1204b37bb341afe10c91d4a9c8dae43a19"  ///< MQTT 密码，由华为云 IoTDA 生成
#define HUAWEI_MQTT_ClientID "69ce6bd8e094d615922d9e08_Smart_Home_0_0_2026040213" ///< MQTT 客户端ID，包含设备ID和时间戳
#define HUAWEI_MQTT_ADDRESS "52e4e17470.st1.iotda-device.cn-east-3.myhuaweicloud.com" ///< MQTT 服务器地址（华为云华东3区）
#define HUAWEI_MQTT_PORT "8883"                  ///< MQTT TLS 加密端口
#define HUAWEI_SERVICE_ID "Smart_Home"           ///< 服务ID，对应华为云 IoTDA 产品模型中定义的服务
#define HUAWEI_MQTT_PUBLISH_TOPIC "$oc/devices/" HUAWEI_DEVICE_ID "/sys/properties/report"  ///< 设备属性上报 Topic

/**
 * @brief  传感器数据上报华为云 IoT 平台 FreeRTOS 任务
 * @param  argument 任务参数（未使用）
 * @note   以约 2 秒为周期，采集所有传感器数据并按 JSON 格式通过 UART3 发送给 ESP32-S3，
 *         由 ESP32-S3 负责与华为云 IoT 平台的 MQTT 通信
 */
void esp_report(void * argument);

#endif
