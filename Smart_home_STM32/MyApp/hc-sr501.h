/**
 * @file    hc-sr501.h
 * @brief   HC-SR501 人体红外感应传感器驱动头文件
 * @details 提供 HC-SR501 传感器检测任务的函数声明和全局变量外部声明。
 *
 * @note    硬件连接：HC-SR501 OUT -> PA0 (GPIO_INPUT)
 */

#ifndef HC_SR_501_H
#define HC_SR_501_H

#include "gpio.h"

/**
 * @brief  HC-SR501 人体红外检测 FreeRTOS 任务函数
 * @param  argument 任务参数（未使用）
 * @note   在 FreeRTOS 中以约 200ms 周期轮询 PA0 电平，
 *         检测到人体时联动蜂鸣器报警（buzzer_bit2 置1）
 */
void hc_sr501_task(void * argument);

/**
 * @brief  HC-SR501 人体红外检测结果（全局变量）
 * @note   定义在 hc-sr501.c 中，此处使用 extern 声明
 *         - 0: 未检测到人体
 *         - 1: 检测到人体
 */
extern uint8_t hc_sr501_value;

#endif
