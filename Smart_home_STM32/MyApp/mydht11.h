/**
 * @file    mydht11.h
 * @brief   DHT11 温湿度传感器驱动头文件
 * @details 定义 DHT11 驱动所需的数据线方向切换宏、数据读写宏、
 *          外部变量声明及函数原型。
 * @note    硬件连接：DHT11 数据引脚接 PA8（GPIOA Pin 8）
 */

#ifndef _MYDHT11_H
#define _MYDHT11_H
#include "usart.h"

#include <stdint.h>
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_tim.h"

/*
 * ============================================================================
 * GPIOx_CRH 寄存器说明（Control Register High）
 * ============================================================================
 * CRH 寄存器控制 GPIO 端口高 8 位引脚（Pin8 ~ Pin15）的工作模式。
 * 每 4 个 bit（CNF + MODE）控制一个引脚：
 *   - MODE[1:0]：输出速度（00=输入, 01=10MHz, 10=2MHz, 11=50MHz）
 *   - CNF[1:0] ：配置模式（输入时：00=模拟, 01=浮空, 10=上/下拉;
 *                          输出时：00=推挽, 01=开漏, 10=复用推挽, 11=复用开漏）
 * ============================================================================
 */

/**
 * @brief   将 DHT11 数据线（PA8）切换为输入模式
 * @details 通过直接操作 GPIOA->CRH 寄存器实现，避免 HAL 库函数调用开销：
 *          1. GPIOA->CRH &= 0xFFFFFF0：清除 Pin8 对应的最低 4 位（bit[3:0]），
 *             保留 Pin9~Pin15 的配置不变
 *          2. GPIOA->CRH |= 0x00000008：将 bit[3:0] 设为 1000（二进制）
 *             - MODE8[1:0] = 00 → 输入模式
 *             - CNF8[1:0]  = 10 → 上拉/下拉输入模式
 *          切换为输入模式后，可通过读取 IDR 寄存器获取 DHT11 数据线电平
 */
#define DHT11_IO_IN()  {GPIOA->CRH&=0XFFFFFFF0;GPIOA->CRH|=8;}

/**
 * @brief   将 DHT11 数据线（PA8）切换为推挽输出模式
 * @details 通过直接操作 GPIOA->CRH 寄存器实现：
 *          1. GPIOA->CRH &= 0xFFFFFF0：清除 Pin8 对应的最低 4 位（bit[3:0]），
 *             保留 Pin9~Pin15 的配置不变
 *          2. GPIOA->CRH |= 0x00000003：将 bit[3:0] 设为 0011（二进制）
 *             - MODE8[1:0] = 11 → 通用推挽输出，速度 50MHz
 *             - CNF8[1:0]  = 00 → 通用推挽输出
 *          切换为输出模式后，可通过写入 ODR 寄存器控制 DHT11 数据线电平
 */
#define DHT11_IO_OUT() {GPIOA->CRH&=0XFFFFFFF0;GPIOA->CRH|=3;}

/**
 * @brief   DHT11 数据线输出控制宏
 * @param   x  输出电平（非 0 为高电平，0 为低电平）
 * @note    封装 HAL_GPIO_WritePin 操作 PA8
 */
#define DHT11_DQ_OUT(x) HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, (x)?GPIO_PIN_SET:GPIO_PIN_RESET)

/**
 * @brief   DHT11 数据线输入读取宏
 * @note    封装 HAL_GPIO_ReadPin 读取 PA8 当前电平
 */
#define DHT11_DQ_IN     HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8)

/**
 * @brief   TIM1 句柄（外部定义，用于微秒级延时）
 */
extern TIM_HandleTypeDef htim1;

/**
 * @brief   UART1 句柄（外部定义，用于调试打印）
 */
extern UART_HandleTypeDef huart1;

/**
 * @brief   湿度全局变量（由 dht11_task 任务周期性更新）
 * @note    单位：%RH，整数部分
 */
extern uint8_t humi;

/**
 * @brief   温度全局变量（由 dht11_task 任务周期性更新）
 * @note    单位：摄氏度，整数部分
 */
extern uint8_t temp;

/**
 * @brief   微秒级延时函数
 * @param   us 延时微秒数
 * @note    基于 TIM1 实现，用于 DHT11 单总线精确时序控制
 */
void Delay_us(uint16_t us);

/**
 * @brief   初始化 DHT11 传感器
 * @return  0 表示成功，1 表示失败
 * @note    配置 PA8 为推挽输出，启动 TIM1，并检测传感器是否在线
 */
uint8_t DHT11_Init(void);

/**
 * @brief   从 DHT11 读取温湿度数据
 * @param   temp 温度值输出指针
 * @param   humi 湿度值输出指针
 * @return  0 表示成功，1 表示失败
 */
uint8_t DHT11_Read_Data(uint8_t *temp, uint8_t *humi);

/**
 * @brief   DHT11 FreeRTOS 任务函数
 * @param   argument 任务参数（未使用）
 * @note    周期性读取 DHT11 数据并通过 UART 打印，
 *          使用 vTaskSuspendAll 保护微秒级时序
 */
void dht11_task(void * argument);

#endif
