/**
 * @file    hc-sr501.c
 * @brief   HC-SR501 人体红外感应传感器驱动
 * @details 基于 FreeRTOS 任务实现人体检测功能，通过读取 GPIO 电平判断是否有人，
 *          并联动蜂鸣器进行报警。
 *
 * @note    硬件连接：
 *          - HC-SR501 OUT -> PA0 (GPIO_INPUT)
 *          - VCC -> 5V, GND -> GND
 *          - 输出高电平(1)表示检测到人体，低电平(0)表示无人
 */

#include "hc-sr501.h"
#include "gpio.h"
#include "usart.h"
#include "myoled.h"

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/**
 * @brief  蜂鸣器报警标志位2（来自 HC-SR501 人体红外检测）
 * @note   在 buzzer.c 中定义，此处声明为 extern 引用
 *         当 buzzer_bit2 = 1 时，表示检测到人体，蜂鸣器将触发报警
 */
extern uint8_t buzzer_bit2;

/**
 * @brief  HC-SR501 人体红外检测结果
 * @note   全局变量，供其他模块读取
 *         - 0: 未检测到人体
 *         - 1: 检测到人体
 */
uint8_t hc_sr501_value = 0;

/**
 * @brief  HC-SR501 人体红外检测 FreeRTOS 任务
 *
 * @param  argument 任务参数（未使用，FreeRTOS 任务函数签名要求）
 * @retval 无（死循环任务，永不返回）
 *
 * @details 任务以约 200ms 为周期轮询 PA0 引脚电平：
 *          - PA0 = 高电平(1)：检测到人体，设置 hc_sr501_value = 1，
 *            同时将 buzzer_bit2 置1，联动蜂鸣器报警
 *          - PA0 = 低电平(0)：未检测到人体，清零 hc_sr501_value 和 buzzer_bit2，
 *            蜂鸣器恢复正常
 *
 * @note   采用 osDelay(100) 实现 200ms 轮询周期（CMSIS-RTOS V2 的 osDelay
 *         参数单位为 RTOS tick，默认 1 tick = 2ms（如果 configTICK_RATE_HZ=500），
 *         因此 100 ticks 约为 200ms）
 */
void hc_sr501_task(void *argument)
{
  while (1)
  {
    /* 读取 PA0 引脚电平，获取 HC-SR501 当前输出状态 */
    uint8_t current_pin_state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);

    if (current_pin_state == 1)
    {
      /* 检测到人体红外信号 */
      hc_sr501_value = 1;   // 更新全局检测状态
      buzzer_bit2 = 1;      // 置位蜂鸣器报警标志，触发蜂鸣器
    }
    else
    {
      /* 未检测到人体 */
      hc_sr501_value = 0;   // 清除检测状态
      buzzer_bit2 = 0;      // 清除蜂鸣器报警标志
    }

    // 调试输出（默认关闭，取消注释可启用串口调试打印）
    // my_printf(&huart1, "hc_sr501_value = %d\r\n", hc_sr501_value);

    osDelay(100); // 延时约 200ms，进入下一次轮询
  }
}
