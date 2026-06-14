#include <string.h>

#include "temp_humi.h"
#include "modbus.h"
#include "crc.h"
#include "usart.h"
#include "gpio.h"
#include "myoled.h"

volatile uint8_t rx_flag;  // 接收完成标志
volatile uint16_t rx_size; // 接收到的数据长度

float temperature; // 解析后的温度值
float humidity;    // 解析后的湿度值

/**
 * @brief   湿度数据全局变量（由任务更新）
 * @note    单位：%RH，整数部分
 */
uint8_t humi = 0;

/**
 * @brief   温度数据全局变量（由任务更新）
 * @note    单位：摄氏度，整数部分
 */
uint8_t temp = 0;

uint8_t rx_buffer[100]; // 串口接收缓冲区

/**
 * @brief  温湿度读取任务
 * @param  argument: 任务参数
 * @retval None
 */
void Temp_Humi_Read(void * argument)
{
    while (1)
    {
        // 在发送请求前，确保之前的串口操作已完成或重置
        HAL_UART_AbortReceive(&huart1); // 停止当前异步接收，防止发送冲突

        __HAL_UART_FLUSH_DRREGISTER(&huart1); // 清空串口接收寄存器，丢弃残留数据

        // 发送 ModBus 读取寄存器请求：
        // 从机地址: 0x01
        // 功能码: 0x03 (读保持寄存器)
        // 起始地址: 0x0000
        // 读取寄存器数量: 2 (对应温度和湿度两个 16 位寄存器)
        ModBus_Read_Registers(0X01, 0x03, 0x0000, 0x0002);

        rx_flag = 0; // 重置接收标志
        rx_size = 0; // 重置数据长度

        memset(rx_buffer, 0, sizeof(rx_buffer)); // 清空接收缓冲区

        // 重新开启异步接收（带空闲中断），准备接收传感器的响应数据
        HAL_UARTEx_ReceiveToIdle_IT(&huart1, rx_buffer, sizeof(rx_buffer));
        
        // 给传感器预留一定的响应处理时间
        osDelay(100);

        // 检查是否收到了响应数据
        if (rx_flag == 1)
        {
            // ModBus RTU 读寄存器响应格式：
            // [从机地址][功能码][字节数][数据1高][数据1低][数据2高][数据2低][CRC低][CRC高]
            // 这里我们期望接收到至少 7 字节 (1+1+1+2*2 = 7) 的有效数据
            if (rx_size >= 7 && rx_buffer[0] == 0x01 && rx_buffer[1] == 0x03)
            {
                // 解析温度（通常为 16 位整数，需除以 10 得到带一位小数的值）
                int16_t temp_raw = (rx_buffer[3] << 8) | rx_buffer[4];
                // 解析湿度
                int16_t humi_raw = (rx_buffer[5] << 8) | rx_buffer[6];

                temperature = temp_raw / 10.0f; // 转换为实际摄氏度
                humidity = humi_raw / 10.0f;    // 转换为实际湿度 %RH

                temp = (uint8_t)temperature; // 保存整数部分到全局变量
                humi = (uint8_t)humidity;    // 保存整数部分到全局变量
            }

            rx_flag = 0; // 处理完毕，重置标志
            
            // 延时一段时间后再进行下一次采集，避免过于频繁占用总线
            osDelay(500); 
        }
    }
}
