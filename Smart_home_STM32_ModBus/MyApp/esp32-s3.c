#include "esp32-s3.h"
#include "usart.h"
#include "temp_humi.h"
#include "mymq-7.h"
#include "hc-sr501.h"
#include <stdio.h>
#include <string.h>

/**
 * ESP32-S3 通信模块
 * 
 * 功能描述：
 * 1. 将 STM32 采集到的各项传感器数据进行汇总。
 * 2. 格式化为 JSON 字符串。
 * 3. 通过 USART3 发送给 ESP32-S3，用于上传云端或语音交互。
 */

extern uint8_t temp;           // 来自 temp_humi.c 的温度值
extern uint8_t humi;           // 来自 temp_humi.c 的湿度值
extern float ppm;              // 来自 mymq-7.c 的 CO 浓度值
extern uint32_t mq7_adc_value; // 来自 mymq-7.c 的 MQ-7 ADC 原始值
extern uint8_t hc_sr501_value; // 来自 hc-sr501.c 的人体感应状态

/**
 * @brief  上报数据到 ESP32-S3 的任务
 * @param  argument: 任务参数
 * @retval None
 */
void esp_report(void *argument)
{
    char payload[256];
    char people_str[10];
    char warning_str[10];

    while (1)
    {
        // 1. 准备显示用的中文字符串（用于调试或特定格式）
        // 如果感应到人，people_str 为 "有人"，否则为 "无人"
        strcpy(people_str, hc_sr501_value ? "有人" : "无人");
        
        // 如果 CO ADC 值超过阈值，warning_str 为 "报警"，否则为 "正常"
        strcpy(warning_str, (mq7_adc_value > 2500) ? "报警" : "正常");

        // 2. 构造 JSON 格式的数据包
        // 包含：温度、湿度、MQ7采样值、PPM浓度、人体感应状态等
        sprintf(payload,
                "{"
                "\"temp\":%d,"
                "\"humi\":%d,"
                "\"mq7_raw\":%lu,"
                "\"ppm\":%.1f,"
                "\"people\":\"%s\","
                "\"warning\":\"%s\""
                "}\r\n",
                (int)temp, (int)humi, (unsigned long)mq7_adc_value, ppm,
                people_str, warning_str);

        // 3. 通过 USART3 发送数据包
        // 这里使用阻塞式发送，确保数据包完整发出
        HAL_UART_Transmit(&huart3, (uint8_t *)payload, strlen(payload), 1000);

        // 4. 设置上报周期，例如每 2 秒上报一次
        osDelay(2000);
    }
}
