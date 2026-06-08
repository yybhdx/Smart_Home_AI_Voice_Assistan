/**
 * @file    myoled.h
 * @brief   0.96寸 OLED 显示屏驱动头文件（SSD1306，软件 I2C）
 * @details 提供 OLED 初始化、清屏、字符/字符串/数字显示等函数声明，
 *          以及 FreeRTOS 显示任务的函数声明。
 *
 * @note    硬件连接：
 *          - SCL -> PB8 (软件 I2C 时钟线)
 *          - SDA -> PB9 (软件 I2C 数据线)
 *          - 128x64 像素，4行 x 16列（8x16 字体）
 */

#ifndef __OLED_H
#define __OLED_H

/**
 * @brief  OLED 初始化（SSD1306 配置序列 + 清屏）
 */
void OLED_Init(void);

/**
 * @brief  OLED 清屏（所有像素熄灭）
 */
void OLED_Clear(void);

/**
 * @brief  OLED 显示单个字符（8x16 字体）
 * @param  Line   行位置（1~4）
 * @param  Column 列位置（1~16）
 * @param  Char   要显示的 ASCII 字符
 */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);

/**
 * @brief  OLED 显示字符串
 * @param  Line   起始行位置（1~4）
 * @param  Column 起始列位置（1~16）
 * @param  String 要显示的字符串（ASCII）
 */
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String);

/**
 * @brief  OLED 显示无符号十进制数字
 * @param  Line   起始行位置（1~4）
 * @param  Column 起始列位置（1~16）
 * @param  Number 要显示的数字（0~4294967295）
 * @param  Length 显示位数（1~10），不足前导补零
 */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);

/**
 * @brief  OLED 显示有符号十进制数字
 * @param  Line   起始行位置（1~4）
 * @param  Column 起始列位置（1~16）
 * @param  Number 要显示的数字（含正负号）
 * @param  Length 显示位数（不含符号位，1~10）
 */
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);

/**
 * @brief  OLED 显示十六进制数字
 * @param  Line   起始行位置（1~4）
 * @param  Column 起始列位置（1~16）
 * @param  Number 要显示的数字（0~0xFFFFFFFF）
 * @param  Length 显示位数（1~8）
 */
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);

/**
 * @brief  OLED 显示二进制数字
 * @param  Line   起始行位置（1~4）
 * @param  Column 起始列位置（1~16）
 * @param  Number 要显示的数字（0~0xFFFFFFFF）
 * @param  Length 显示位数（1~16）
 */
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);

/**
 * @brief  OLED 显示更新 FreeRTOS 任务
 * @param  argument 任务参数（未使用）
 * @note   以约 1 秒为周期刷新 OLED 显示，4行布局：
 *         - 第1行：温度 + 湿度
 *         - 第2行：MQ-7 ADC 值
 *         - 第3行：PPM 浓度值
 *         - 第4行：人体检测 + CO 超标警告
 */
void oled_task(void * argument);

#endif
