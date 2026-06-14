#include "hc-sr501.h"
#include "gpio.h"
#include "cmsis_os.h"

/**
 * HC-SR501 人体红外感应模块
 * 
 * 工作原理：
 * 1. 当有人进入感应范围时，模块输出高电平。
 * 2. 当人离开或在范围内静止超过延时时间后，输出低电平。
 * 3. STM32 通过读取 GPIO 引脚状态来判断是否检测到人体活动。
 */

extern uint8_t buzzer_bit2;

// 存储人体感应器的当前状态 (0: 未触发, 1: 已触发)
uint8_t hc_sr501_value = 0;

/**
 * @brief  人体红外感应处理任务
 * @param  argument: 任务参数
 * @retval None
 */
void hc_sr501_task(void *argument)
{
    while (1)
    {
        // 1. 读取传感器连接的 GPIO 引脚状态
        // 假设传感器输出接在 GPIOA 的 PIN 0
        uint8_t current_pin_state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
        
        // 2. 更新全局状态变量
        hc_sr501_value = current_pin_state;

        // 3. 报警逻辑
        // 如果感应到有人，则触发蜂鸣器报警标志位 2
        if (hc_sr501_value == GPIO_PIN_SET)
        {
            buzzer_bit2 = 1;
        }
        else
        {
            buzzer_bit2 = 0;
        }

        // 任务延时，采样频率不需要太高，200ms 即可
        osDelay(200);
    }
}
