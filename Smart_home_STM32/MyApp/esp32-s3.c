/**
 * @file    esp32-s3.c
 * @brief   ESP32-S3 WiFi 模块通信驱动（华为云 IoT 数据上报）
 * @details 通过 UART3 与 ESP32-S3 通信，将传感器采集的数据按 JSON 格式
 *          打包后发送给 ESP32-S3，由 ESP32-S3 通过 MQTT 协议上报至华为云 IoT 平台。
 *
 * @note    硬件连接：
 *          - STM32 UART3 TX (PB10) -> ESP32-S3 RX
 *          - STM32 UART3 RX (PB11) -> ESP32-S3 TX
 *          - 共地 GND
 *
 * @note    数据流向：
 *          STM32 传感器采集 -> JSON 格式化 -> UART3 发送 ->
 *          ESP32-S3 接收 -> WiFi/MQTT -> 华为云 IoT 平台
 */

#include "esp32-s3.h"
#include "usart.h"
#include "mydht11.h"
#include "mymq-7.h"
#include "hc-sr501.h"
#include "buzzer.h"
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* ======================== 外部变量引用 ======================== */

extern uint8_t temp;           ///< 温度值，来自 DHT11 传感器（单位：°C）
extern uint8_t humi;           ///< 湿度值，来自 DHT11 传感器（单位：%RH）
extern uint32_t mq7_adc_value; ///< MQ-7 传感器 ADC 原始采样值（12位，0~4095）
extern float ppm;              ///< CO 浓度计算值（单位：ppm）
extern uint8_t hc_sr501_value; ///< HC-SR501 人体红外检测结果（0/1）
extern uint8_t buzzer_bit1;    ///< 蜂鸣器标志位1，来自 MQ-7（CO超标为1）
extern uint8_t buzzer_bit2;    ///< 蜂鸣器标志位2，来自 HC-SR501（检测到人为1）

/* ======================== 数据缓冲区 ======================== */

char payload[512];    ///< JSON 报文缓冲区，存放格式化后的上报数据
char people_str[16];  ///< 人员检测状态字符串（"有人" 或 "无人"）
char warning_str[16]; ///< CO 浓度警告字符串（"超标" 或 "正常"）

/**
 * @brief  传感器数据上报华为云 IoT 平台 FreeRTOS 任务
 *
 * @param  argument 任务参数（未使用，FreeRTOS 任务函数签名要求）
 * @retval 无（死循环任务，永不返回）
 *
 * @details 任务以 osDelay(1000) 约 2 秒为周期执行一次，流程如下：
 *          1. 读取 HC-SR501 状态，构造人员检测字符串（"有人"/"无人"）
 *          2. 判断 MQ-7 ADC 值是否超过阈值 2500，构造警告字符串（"超标"/"正常"）
 *          3. 综合两个蜂鸣器标志位判断蜂鸣器当前状态
 *          4. 将所有传感器数据按华为云 IoT 设备属性格式组装 JSON 报文
 *          5. 通过 UART3 串口发送 JSON 报文给 ESP32-S3
 *
 * @note   JSON 报文格式遵循华为云 IoTDA 平台设备属性上报规范：
 *         - Topic: $oc/devices/{device_id}/sys/properties/report
 *         - service_id: Smart_Home
 *         - 属性包含：temp, humi, mq-7, ppm, hc_sr_501, people, warning, beep
 */
void esp_report(void *argument)
{
    while (1)
    {
        uint8_t beep_status;

        /* ---- 第1步：构造人员检测状态字符串 ---- */
        // hc_sr501_value = 1 表示检测到人体，显示"有人"；否则显示"无人"
        sprintf(people_str, hc_sr501_value ? "有人" : "无人");

        /* ---- 第2步：构造 CO 浓度警告字符串 ---- */
        // ADC 采样值超过 2500 认为浓度超标，显示"超标"；否则显示"正常"
        sprintf(warning_str, (mq7_adc_value > 2500) ? "超标" : "正常");

        /* ---- 第3步：判断蜂鸣器当前状态 ---- */
        // buzzer_bit1(CO超标) 或 buzzer_bit2(人体检测) 任一为1，蜂鸣器即为开启状态
        beep_status = (buzzer_bit1 || buzzer_bit2) ? 1 : 0;

        /* ---- 第4步：构造 JSON 报文 ---- */
        // 按华为云 IoTDA 设备属性上报格式组装 JSON
        // 各字段说明：
        //   temp      - 温度值（°C，整数）
        //   humi      - 湿度值（%RH，整数）
        //   mq-7      - MQ-7 ADC 原始值（0~4095）
        //   ppm       - CO 浓度（ppm，浮点取整）
        //   hc_sr_501 - 人体红外布尔值（true/false）
        //   people    - 人员检测文字（"有人"/"无人"）
        //   warning   - CO 警告文字（"超标"/"正常"）
        //   beep      - 蜂鸣器布尔值（true/false）
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

        /* ---- 第5步：通过 UART3 发送 JSON 报文给 ESP32-S3 ---- */
        // 末尾加 \n 换行符，方便 ESP32-S3 按行解析接收数据
        my_printf(&huart3, "%s\n", payload);

        osDelay(1000); // 延时约 2 秒，进入下一次上报周期
    }
}
