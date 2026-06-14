#include "mymq-7.h"
#include "adc.h"
#include "usart.h"
#include "gpio.h"
#include "cmsis_os.h"
#include "math.h"

/**
 * MQ-7 一氧化碳传感器模块
 * 
 * 工作原理：
 * 1. MQ-7 对一氧化碳具有很高的灵敏度。
 * 2. 传感器输出为模拟电压，通过 STM32 的 ADC 进行采集。
 * 3. 根据电压值计算出传感器的电阻 RS，再结合 R0（在清洁空气中的电阻）计算比值。
 * 4. 通过公式或查表计算出 PPM（浓度单位）。
 */

extern uint8_t buzzer_bit1;

// MQ-7 相关全局变量
uint32_t mq7_adc_value = 0; // 存储 ADC 原始采样值
float ppm = 0;              // 存储计算后的一氧化碳浓度 (PPM)

/**
 * @brief  MQ-7 传感器处理任务
 * @param  argument: 任务参数
 * @retval None
 */
void mq7_task(void *argument)
{
    while (1)
    {
        // 1. 启动 ADC 转换
        HAL_ADC_Start(&hadc1); 
        
        // 2. 等待转换完成，超时时间设为 10ms
        if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
        {
            // 3. 读取 ADC 转换结果（STM32F103 ADC 为 12 位，范围 0-4095）
            mq7_adc_value = HAL_ADC_GetValue(&hadc1);
            
            // 4. 计算传感器输出电压 Vol
            // 公式：Vol = ADC值 * 系统电压(5V) / 4096
            // 注意：虽然 STM32 IO 是 3.3V，但传感器通常由 5V 供电，需注意分压或参考电压
            float Vol = ((float)mq7_adc_value * 5.0f / 4096.0f);

            // 5. 计算传感器当前电阻 RS
            // 根据分压电路原理计算，这里假设负载电阻为一定值
            if (Vol > 0.1f) // 防止除以 0
            {
                float RS = (5.0f - Vol) / (Vol * 0.5f); 
                
                // 6. 计算 PPM 值
                // R0 为传感器在清洁空气中的参考电阻（需根据实际标定调整，此处取 6.64）
                float R0 = 6.64f;
                ppm = pow(11.66f * (RS / R0), -1.58f) * 100.0f;
                
                // 7. 阈值报警逻辑
                // 如果浓度超过 50 PPM，触发蜂鸣器报警标志位
                if (ppm > 50.0f)
                {
                    buzzer_bit1 = 1;
                }
                else
                {
                    buzzer_bit1 = 0;
                }
            }
        }
        else
        {
            // ADC 读取失败处理
            // 可以添加错误日志输出
        }

        // 任务延时，降低功耗和 CPU 占用
        osDelay(1000); 
    }
}
