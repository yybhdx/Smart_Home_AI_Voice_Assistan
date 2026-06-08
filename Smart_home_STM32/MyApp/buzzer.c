/**
 * @file    buzzer.c
 * @brief   有源蜂鸣器驱动
 * @details 基于 FreeRTOS 任务实现蜂鸣器报警功能，支持双条件触发：
 *          1) MQ-7 一氧化碳传感器检测到 CO 浓度超标 (buzzer_bit1)
 *          2) HC-SR501 人体红外传感器检测到有人 (buzzer_bit2)
 *          任一条件满足即触发蜂鸣器报警。
 *
 * @note    硬件连接：
 *          - 蜂鸣器控制端 -> PB12 (GPIO_OUTPUT)
 *          - 使用有源蜂鸣器，低电平触发发声，高电平关闭
 *          - 有源蜂鸣器内部自带振荡源，只需给低电平即可发声，
 *            无需 PWM 驱动
 */

#include "buzzer.h"
#include "gpio.h"
#include "usart.h"

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/**
 * @brief  蜂鸣器报警标志位1（来自 MQ-7 一氧化碳传感器）
 * @note   在 mymq-7.c 中定义，此处声明为 extern 引用
 *         当 buzzer_bit1 = 1 时，表示 CO 浓度超标
 */
extern uint8_t buzzer_bit1;

/**
 * @brief  蜂鸣器报警标志位2（来自 HC-SR501 人体红外传感器）
 * @note   在 hc-sr501.c 中定义，此处声明为 extern 引用
 *         当 buzzer_bit2 = 1 时，表示检测到人体
 */
extern uint8_t buzzer_bit2;

/**
 * @brief  开启蜂鸣器（低电平触发）
 *
 * @param  无
 * @retval 无
 *
 * @details 将 PB12 拉低（GPIO_PIN_RESET），有源蜂鸣器导通发声。
 *          有源蜂鸣器原理：内部集成振荡电路，当控制端为低电平时，
 *          三极管导通，蜂鸣器获得工作电压开始鸣叫。
 */
void Buzzer_On(void)
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
}

/**
 * @brief  关闭蜂鸣器（高电平关闭）
 *
 * @param  无
 * @retval 无
 *
 * @details 将 PB12 拉高（GPIO_PIN_SET），有源蜂鸣器断电停止鸣叫。
 */
void Buzzer_Off(void)
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
}

/**
 * @brief  蜂鸣器控制 FreeRTOS 任务
 *
 * @param  argument 任务参数（未使用，FreeRTOS 任务函数签名要求）
 * @retval 无（死循环任务，永不返回）
 *
 * @details 双条件报警逻辑：
 *          - buzzer_bit1：来自 MQ-7 传感器，CO 浓度超标时置1
 *          - buzzer_bit2：来自 HC-SR501 传感器，检测到人体时置1
 *          - 当两个标志位均为0时，关闭蜂鸣器
 *          - 当任一标志位为1时，开启蜂鸣器报警
 *
 * @note   以 osDelay(100) 约 200ms 为周期循环检测报警条件，
 *         同时通过串口打印当前蜂鸣器状态用于调试
 */
void Buzzer_Task(void *argument)
{
	while (1)
	{
		/* 双条件均为0，无报警触发，关闭蜂鸣器 */
		if (buzzer_bit1 == 0 && buzzer_bit2 == 0)
		{
			Buzzer_Off();
			my_printf(&huart1, "BUZZER_OFF\r\n");
		}
		/* 任一条件满足（人体检测或CO浓度超标），开启蜂鸣器报警 */
		else
		{
			Buzzer_On();
			my_printf(&huart1, "BUZZER_ON\r\n");
		}
		osDelay(100); // 延时约 200ms，进入下一次检测
	}
}
