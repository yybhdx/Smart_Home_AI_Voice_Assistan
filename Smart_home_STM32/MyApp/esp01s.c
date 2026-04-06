#include "esp01s.h"
#include "usart.h"
#include "mydht11.h"
#include "mymq-7.h"
#include "hc-sr501.h"
#include "buzzer.h"
#include <stdio.h>

// 引用外部变量
extern uint8_t temp;           // 温度 (DHT11)
extern uint8_t humi;           // 湿度 (DHT11)
extern uint32_t mq7_adc_value; // MQ-7 ADC值
extern float ppm;              // CO浓度 ppm
extern uint8_t hc_sr501_value; // 人体红外感应
extern uint8_t buzzer_bit1;    // 蜂鸣器标志1
extern uint8_t buzzer_bit2;    // 蜂鸣器标志2

/**
 * @brief ESP01S初始化，连接WiFi和华为云MQTT
 */
void esp_init(void)
{
    // 1. 复位ESP8266模块
    my_printf(&huart3, "AT+RST\r\n");
    HAL_Delay(2000);

    // 2. 设置为Station模式
    my_printf(&huart3, "AT+CWMODE=1\r\n");
    HAL_Delay(1000);

    // 3. 连接Wi-Fi
    my_printf(&huart3, "AT+CWJAP=\"%s\",\"%s\"\r\n", WIFI_SSID, WIFI_PWD);
    HAL_Delay(5000);

    // 4. 配置MQTT用户信息 (使用SSL，端口8883)
    // AT+MQTTUSERCFG=<link_id>,<scheme>,"<client_id>","<username>","<password>",<cert_key_ID>,<CA_ID>,"<path>"
    // scheme: 1 = TLS, 2 = TLS验证客户端证书
    my_printf(&huart3, "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"\r\n",
              HUAWEI_MQTT_ClientID,
              HUAWEI_MQTT_USERNAME,
              HUAWEI_MQTT_PASSWORD);
    HAL_Delay(1000);

    // 5. 连接MQTT服务器
    // AT+MQTTCONN=<link_id>,"<host>",<port>,<reconnect>
    my_printf(&huart3, "AT+MQTTCONN=0,\"%s\",%s,1\r\n",
              HUAWEI_MQTT_ADDRESS,
              HUAWEI_MQTT_PORT);
    HAL_Delay(2000);
}

/**
 * @brief 上报传感器数据到华为云
 *
 * 数据格式:
 * {
 *   "services": [{
 *     "service_id": "BasicData",
 *     "properties": {
 *       "temp": 25,
 *       "humi": 60,
 *       "mq-7": 1234,
 *       "ppm": 12,
 *       "hc_sr_501": true,
 *       "people": "有人",
 *       "warning": "正常",
 *       "beep": false
 *     }
 *   }]
 * }
 */
void esp_report(void)
{
    char payload[512];
    char people_str[16];
    char warning_str[16];
    uint8_t beep_status;

    // 构造属性值
    // 人员检测字符串
    sprintf(people_str, hc_sr501_value ? "有人" : "无人");

    // CO浓度警告字符串 (ppm > 50 认为超标)
    sprintf(warning_str, (ppm > 50.0f) ? "超标" : "正常");

    // 蜂鸣器状态
    beep_status = (buzzer_bit1 || buzzer_bit2) ? 1 : 0;

    // 构造JSON负载
    // 注意: AT指令中的双引号需要转义为 \"
    // 简化后的 JSON 构造 (不再需要转义反斜杠，不再需要 AT 指令包裹)
    sprintf(payload,
            "{\"services\":[{\"service_id\":\"%s\","
            "\"properties\":{"
            "\"temp\":%d,"
            "\"humi\":%d,"
            "\"mq-7\":%lu,"
            "\"ppm\":%.0f,"
            "\"hc_sr_501\":%s,"
            "\"people\":\"%s\","
            "\"warning\":\"%s\","
            "\"beep\":%s"
            "}}]}",
            HUAWEI_SERVICE_ID,
            (int)temp, (int)humi, (unsigned long)mq7_adc_value, ppm,
            hc_sr501_value ? "true" : "false",
            people_str, warning_str, beep_status ? "true" : "false");

    // 直接通过串口发送 JSON 字符串，末尾加 \n 方便 ESP32 读取
    my_printf(&huart3, "%s\n", payload); 
}
