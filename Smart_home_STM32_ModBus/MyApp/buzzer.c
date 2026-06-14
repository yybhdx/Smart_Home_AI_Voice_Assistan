#include "buzzer.h"
#include "gpio.h"
#include "cmsis_os.h"

/**
 * 蜂鸣器控制模块
 * 
 * 触发条件：
 * 1. MQ-7 传感器检测到 CO 浓度超标 (buzzer_bit1 == 1)
 * 2. HC-SR501 传感器检测到有人经过 (buzzer_bit2 == 1)
 */

extern uint8_t buzzer_bit1;
extern uint8_t buzzer_bit2;

/**
 * @brief  开启蜂鸣器
 * @note   硬件连接：GPIOB PIN 12，低电平触发（根据代码逻辑判断）
 */
void Buzzer_On(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
}

/**
 * @brief  关闭蜂鸣器
 */
void Buzzer_Off(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
}

/**
 * @brief  蜂鸣器控制任务
 * @param  argument: 任务参数
 * @retval None
 */
void Buzzer_Task(void *argument)
{
    while (1)
    {
        // 逻辑判断：如果 CO 报警和人体感应报警均未触发
        if (buzzer_bit1 == 0 && buzzer_bit2 == 0)
        {
            Buzzer_Off(); // 关闭蜂鸣器
        }
        else
        {
            // 只要有一个报警标志位被置 1，则开启蜂鸣器
            // 这里可以根据需要实现间歇性鸣叫或其他逻辑
            Buzzer_On();
        }

        // 任务延时，控制检查频率
        osDelay(100);
    }
}
