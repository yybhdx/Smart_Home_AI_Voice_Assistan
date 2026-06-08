/**
 * @file    mymq-7.h
 * @brief   MQ-7 一氧化碳传感器驱动头文件
 * @details 声明 MQ-7 传感器任务函数及外部变量。
 * @note    硬件连接：MQ-7 模拟输出接 ADC1 输入通道
 *          ADC 分辨率：12 位（0~4095）
 *          参考电压：5V
 */

#ifndef MQ_7_H
#define MQ_7_H
#include <math.h>
#include "adc.h"

/**
 * @brief   MQ-7 传感器 FreeRTOS 任务函数
 * @param   argument 任务参数（未使用）
 * @note    周期性读取 ADC 值，计算 CO 浓度（ppm），
 *          超过阈值时通过 buzzer_bit1 触发蜂鸣器报警。
 */
void mq7_task(void * argument);

/**
 * @brief   MQ-7 ADC 采样原始值（12 位，范围 0~4095）
 */
extern uint32_t mq7_adc_value;

/**
 * @brief   计算得到的 CO 浓度值（单位：ppm）
 * @note    通过 power curve 公式 ppm = 11.5428*(R0/RS)^0.6549 计算
 */
extern float ppm;

#endif
