/**
 * @file    mymq-7.c
 * @brief   MQ-7 一氧化碳传感器驱动程序
 * @details 通过 ADC 读取 MQ-7 传感器的模拟电压值，计算传感器电阻比值 RS/R0，
 *          再利用 power curve 公式计算 CO 浓度（ppm）。
 *          当 ADC 值超过阈值时触发蜂鸣器报警。
 * @note    硬件连接：MQ-7 模拟输出接 ADC1 通道
 *          传感器工作电压：5V
 *          负载电阻 RL = 0.5kΩ（分压电路）
 *          R0（洁净空气中传感器电阻）= 6.64kΩ（校准值）
 *          报警阈值：ADC >= 2500
 */

#include "mymq-7.h"
#include "usart.h"
#include "adc.h"

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/**
 * @brief   蜂鸣器报警标志位（外部定义）
 * @note    当 buzzer_bit1 = 1 时，蜂鸣器报警任务将触发蜂鸣器鸣响；
 *          当 buzzer_bit1 = 0 时，蜂鸣器静默。由 mq7_task 根据CO浓度更新。
 */
extern uint8_t buzzer_bit1;

/**
 * @brief   MQ-7 ADC 采样原始值
 * @note    范围 0~4095（12 位 ADC），对应 0~5V 模拟输入
 */
uint32_t mq7_adc_value = 0;

/**
 * @brief   计算得到的 CO 浓度值（单位：ppm）
 */
float ppm = 0;

/**
 * @brief   MQ-7 一氧化碳传感器 FreeRTOS 任务函数
 * @param   argument 任务参数（未使用）
 * @note    任务流程：
 *          1. 启动 ADC 并等待转换完成，读取原始 ADC 值
 *          2. 将 ADC 值转换为电压值：Vol = ADC_value * 5.0 / 4096.0
 *          3. 计算传感器电阻 RS：RS = (Vc - Vol) / (Vol * RL)，
 *             其中 Vc = 5V，RL = 0.5kΩ（负载电阻）
 *          4. 利用 power curve 公式计算 CO 浓度：
 *             ppm = 11.5428 * (R0/RS)^0.6549
 *             其中 R0 = 6.64kΩ（洁净空气中校准值）
 *          5. 判断报警阈值：ADC >= 2500 时置 buzzer_bit1 = 1 触发蜂鸣器
 *          6. 延时 500 个 tick 后再次采样
 *
 * @details CO 浓度计算原理：
 *          MQ-7 传感器内部有一个 SnO2 气敏元件，其电阻 RS 随 CO 浓度增大而减小。
 *          分压电路中：Vout = Vc * RL / (RS + RL)，可推导出：
 *          RS = (Vc - Vout) * RL / Vout = (Vc - Vout) / (Vout / RL)
 *          洁净空气中 RS = R0，CO 浓度越高 RS 越小，R0/RS 比值越大。
 *          Power curve 公式通过标定实验数据拟合得到。
 */
void mq7_task(void *argument)
{

  while (1)
  {
    /* ---- 第一步：ADC 采样 ---- */
    HAL_ADC_Start(&hadc1); /* 启动 ADC1 转换 */

    if (HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY) == HAL_OK)
    {
      /* 读取 ADC 转换结果（12位精度，范围 0~4095） */
      mq7_adc_value = HAL_ADC_GetValue(&hadc1);
    }
    else
    {
      my_printf(&huart1, "adc_read_error\r\n");
    }

    /* 打印 ADC 原始值，用于调试 */
    my_printf(&huart1, "adc_value= %d\r\n", mq7_adc_value);

    /* ---- 第二步：电压转换与 CO 浓度计算 ---- */
    /* 将 ADC 原始值转换为实际电压（参考电压 5V，12位分辨率 4096） */
    float Vol = ((float)mq7_adc_value * 5 / 4096.0f);

    /* 计算传感器电阻 RS（单位 kΩ）
     * 分压公式推导：Vout = Vc * RL / (RS + RL)
     * 解得 RS = (Vc - Vout) / (Vout / RL) = (Vc - Vol) / (Vol * RL)
     * 其中 RL = 0.5kΩ 为负载电阻 */
    float RS = (5 - Vol) / (Vol * 0.5);

    /* R0 为洁净空气中传感器的电阻值（校准值 6.64kΩ） */
    float R0 = 6.64;

    /* 利用 power curve 公式计算 CO 浓度（ppm）
     * 公式来源：MQ-7 数据手册标定曲线拟合
     * ppm = 11.5428 * (R0/RS)^0.6549
     * R0/RS 比值越大，说明 CO 浓度越高（RS 随浓度增大而减小） */
    ppm = pow(11.5428 * R0 / RS, 0.6549f);

    /* ---- 第三步：报警阈值判断 ---- */
    /* 当 ADC 原始值 >= 2500 时，认为 CO 浓度过高，置位蜂鸣器报警标志 */
    if ((mq7_adc_value >= 2500))
    {
      buzzer_bit1 = 1; /* 触发蜂鸣器报警 */
    }
    else
    {
      buzzer_bit1 = 0; /* CO 浓度正常，关闭报警 */
    }

    /* 打印计算的 ppm 值，用于调试 */
    my_printf(&huart1, "ppm_value = %.f\r\n", ppm);

    osDelay(500); /* 延时 500 个 tick 后再次采样 */
  }
}
