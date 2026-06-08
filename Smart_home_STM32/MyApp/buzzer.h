/**
 * @file    buzzer.h
 * @brief   有源蜂鸣器驱动头文件
 * @details 提供蜂鸣器开关控制函数和 FreeRTOS 报警任务的函数声明。
 *
 * @note    硬件连接：蜂鸣器控制端 -> PB12 (GPIO_OUTPUT)
 *          低电平触发发声，高电平关闭
 */

#ifndef BUZZER_H
#define BUZZER_H

/**
 * @brief  开启蜂鸣器（PB12 拉低，有源蜂鸣器低电平触发）
 */
void Buzzer_On(void);

/**
 * @brief  关闭蜂鸣器（PB12 拉高）
 */
void Buzzer_Off(void);

/**
 * @brief  蜂鸣器控制 FreeRTOS 任务
 * @param  argument 任务参数（未使用）
 * @note   双条件报警：buzzer_bit1(CO超标) 或 buzzer_bit2(人体检测) 任一为1则响
 */
void Buzzer_Task(void * argument);

#endif
