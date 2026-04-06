#ifndef ESP32_S3_H
#define ESP32_S3_H

#include <stdint.h>

// Wi-Fi 连接参数
#define WIFI_SSID "jifei"      // Wi-Fi 热点名称
#define WIFI_PWD "12345678"    // Wi-Fi 密码

// 华为云物联网平台 MQTT 连接参数
#define HUAWEI_DEVICE_ID "69ce6bd8e094d615922d9e08_Smart_Home"
#define HUAWEI_MQTT_USERNAME "69ce6bd8e094d615922d9e08_Smart_Home"
#define HUAWEI_MQTT_PASSWORD "b859e0be5c2f5ed05ec764914e485d1204b37bb341afe10c91d4a9c8dae43a19"
#define HUAWEI_MQTT_ClientID "69ce6bd8e094d615922d9e08_Smart_Home_0_0_2026040213"
#define HUAWEI_MQTT_ADDRESS "52e4e17470.st1.iotda-device.cn-east-3.myhuaweicloud.com"
#define HUAWEI_MQTT_PORT "8883"
#define HUAWEI_SERVICE_ID "Smart_Home"
#define HUAWEI_MQTT_PUBLISH_TOPIC "$oc/devices/" HUAWEI_DEVICE_ID "/sys/properties/report"


// 上报传感器数据到华为云
void esp_report(void);

#endif
