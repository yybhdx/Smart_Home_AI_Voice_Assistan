/**
 * @file    myoled.c
 * @brief   0.96寸 OLED 显示屏驱动（SSD1306 控制器，软件 I2C）
 * @details 基于软件模拟 I2C 协议驱动 0.96 寸 OLED 显示屏，提供字符、字符串、
 *          数字显示等功能，并实现 FreeRTOS 任务实时显示传感器数据。
 *
 * @note    硬件连接：
 *          - OLED SCL -> PB8 (GPIO_OUTPUT，软件模拟 I2C 时钟线)
 *          - OLED SDA -> PB9 (GPIO_OUTPUT，软件模拟 I2C 数据线)
 *          - OLED VCC -> 3.3V, GND -> GND
 *
 * @note    软件 I2C 原理说明：
 *          本项目使用 PB8/PB9 而非 STM32 硬件 I2C 默认引脚（PB6/PB7），
 *          因为本驱动完全通过 GPIO 模拟 I2C 时序（软件 I2C），而非使用
 *          STM32 内置的硬件 I2C 外设。软件 I2C 的优势：
 *          - 引脚选择灵活，不受硬件 I2C 引脚映射限制
 *          - 时序完全可控，避免 STM32 硬件 I2C 的已知 BUG
 *          - 适用于低速外设（OLED 刷新率要求不高）
 *
 * @note    OLED 显示布局（128x64 像素，4行 x 16列，8x16 字体）：
 *          - 第1行：温度 + 湿度（DHT11）
 *          - 第2行：MQ-7 ADC 原始值
 *          - 第3行：CO 浓度 PPM 值
 *          - 第4行：人体检测状态 + CO 超标警告
 */

#include "OLED_Font.h"
#include "temp_humi.h"
#include "myoled.h"
#include "mymq-7.h"
#include "hc-sr501.h"
#include "gpio.h"

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* ======================== 引脚操作宏定义 ======================== */

/**
 * @brief  软件 I2C SCL 写操作宏
 * @param  x 电平值，1 为高电平，0 为低电平
 * @note   使用 HAL_GPIO_WritePin 操作 PB8 引脚
 */
#define OLED_W_SCL(x) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, (x) ? GPIO_PIN_SET : GPIO_PIN_RESET)

/**
 * @brief  软件 I2C SDA 写操作宏
 * @param  x 电平值，1 为高电平，0 为低电平
 * @note   使用 HAL_GPIO_WritePin 操作 PB9 引脚
 */
#define OLED_W_SDA(x) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, (x) ? GPIO_PIN_SET : GPIO_PIN_RESET)

/* ======================== 软件 I2C 底层函数 ======================== */

/**
 * @brief  软件 I2C 引脚初始化
 *
 * @param  无
 * @retval 无
 *
 * @details 调用 HAL 库 GPIO 初始化函数配置 PB8(SCL) 和 PB9(SDA)，
 *          并将两条线拉高（I2C 空闲状态为高电平）
 */
void OLED_I2C_Init(void)
{
  MX_GPIO_Init(); // 调用 CubeMX 生成的 GPIO 初始化函数
  OLED_W_SCL(1);  // SCL 拉高，I2C 空闲状态
  OLED_W_SDA(1);  // SDA 拉高，I2C 空闲状态
}

/**
 * @brief  I2C 起始信号
 *
 * @param  无
 * @retval 无
 *
 * @details I2C 起始条件：SCL 为高电平期间，SDA 由高变低
 *          时序：SDA=1 -> SCL=1 -> SDA=0 -> SCL=0
 */
void OLED_I2C_Start(void)
{
  OLED_W_SDA(1);
  OLED_W_SCL(1);
  OLED_W_SDA(0);
  OLED_W_SCL(0);
}

/**
 * @brief  I2C 停止信号
 *
 * @param  无
 * @retval 无
 *
 * @details I2C 停止条件：SCL 为高电平期间，SDA 由低变高
 *          时序：SDA=0 -> SCL=1 -> SDA=1
 */
void OLED_I2C_Stop(void)
{
  OLED_W_SDA(0);
  OLED_W_SCL(1);
  OLED_W_SDA(1);
}

/**
 * @brief  I2C 发送一个字节
 *
 * @param  Byte 要发送的字节数据（8位）
 * @retval 无
 *
 * @details 按高位在前（MSB）的顺序逐位发送：
 *          - 每一位：先设置 SDA 电平，再拉高 SCL（从机读取），再拉低 SCL
 *          - 发送完 8 位后，额外产生一个 SCL 时钟脉冲用于从机应答（ACK）
 */
void OLED_I2C_SendByte(uint8_t Byte)
{
  uint8_t i;
  for (i = 0; i < 8; i++)
  {
    OLED_W_SDA(!!(Byte & (0x80 >> i))); // 从最高位开始，逐位写入 SDA
    OLED_W_SCL(1);  // SCL 拉高，从机在此期间读取 SDA
    OLED_W_SCL(0);  // SCL 拉低，准备下一位
  }
  OLED_W_SCL(1); // 额外的一个时钟脉冲，用于容纳从机应答信号（ACK）
  OLED_W_SCL(0);
}

/* ======================== OLED 命令/数据写入 ======================== */

/**
 * @brief  OLED 写命令
 *
 * @param  Command 要写入的命令字节
 * @retval 无
 *
 * @details 通过 I2C 向 SSD1306 发送命令：
 *          - 从机地址：0x78（SSD1306 默认地址，写模式）
 *          - 控制字节：0x00（表示后续字节为命令）
 *          - 命令数据：Command
 */
void OLED_WriteCommand(uint8_t Command)
{
  OLED_I2C_Start();
  OLED_I2C_SendByte(0x78); // SSD1306 I2C 从机地址（7位地址 0x3C + 写位 0）
  OLED_I2C_SendByte(0x00); // 控制字节：后续为命令
  OLED_I2C_SendByte(Command);
  OLED_I2C_Stop();
}

/**
 * @brief  OLED 写数据
 *
 * @param  Data 要写入的显示数据字节
 * @retval 无
 *
 * @details 通过 I2C 向 SSD1306 发送显示数据：
 *          - 从机地址：0x78
 *          - 控制字节：0x40（表示后续字节为显示数据）
 *          - 显示数据：Data
 */
void OLED_WriteData(uint8_t Data)
{
  OLED_I2C_Start();
  OLED_I2C_SendByte(0x78); // SSD1306 I2C 从机地址
  OLED_I2C_SendByte(0x40); // 控制字节：后续为显示数据
  OLED_I2C_SendByte(Data);
  OLED_I2C_Stop();
}

/**
 * @brief  OLED 设置光标位置
 *
 * @param  Y 行坐标，范围：0~7（共8页，每页8像素高）
 * @param  X 列坐标，范围：0~127
 * @retval 无
 *
 * @details 通过发送 SSD1306 命令设置显示起始位置：
 *          - 0xB0 | Y：设置页地址（Y方向）
 *          - X 高4位和低4位分别设置列地址
 */
void OLED_SetCursor(uint8_t Y, uint8_t X)
{
  OLED_WriteCommand(0xB0 | Y);                 // 设置 Y 位置（页地址）
  OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4)); // 设置 X 位置高4位
  OLED_WriteCommand(0x00 | (X & 0x0F));        // 设置 X 位置低4位
}

/* ======================== OLED 显示功能函数 ======================== */

/**
 * @brief  OLED 清屏
 *
 * @param  无
 * @retval 无
 *
 * @details 将所有 128x64 像素点清零，即向所有位置写入 0x00
 */
void OLED_Clear(void)
{
  uint8_t i, j;
  for (j = 0; j < 8; j++)   // 遍历8页
  {
    OLED_SetCursor(j, 0);
    for (i = 0; i < 128; i++) // 每页128列
    {
      OLED_WriteData(0x00);
    }
  }
}

/**
 * @brief  OLED 显示一个字符（8x16 字体）
 *
 * @param  Line   行位置，范围：1~4（对应 OLED 的页0~3）
 * @param  Column 列位置，范围：1~16（每列8像素宽）
 * @param  Char   要显示的字符，范围：ASCII 可见字符（0x20~0x7E）
 * @retval 无
 *
 * @details 8x16 字体分上下两部分显示：
 *          - 上半部分：字符点阵前8字节（页 Y）
 *          - 下半部分：字符点阵后8字节（页 Y+1）
 *          字模数据取自 OLED_F8x16 字库数组
 */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{
  uint8_t i;
  OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8); // 设置光标到上半部分
  for (i = 0; i < 8; i++)
  {
    OLED_WriteData(OLED_F8x16[Char - ' '][i]); // 写入上半部分点阵数据
  }
  OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8); // 设置光标到下半部分
  for (i = 0; i < 8; i++)
  {
    OLED_WriteData(OLED_F8x16[Char - ' '][i + 8]); // 写入下半部分点阵数据
  }
}

/**
 * @brief  OLED 显示字符串
 *
 * @param  Line   起始行位置，范围：1~4
 * @param  Column 起始列位置，范围：1~16
 * @param  String 要显示的字符串（以 '\0' 结尾）
 * @retval 无
 */
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
  uint8_t i;
  for (i = 0; String[i] != '\0'; i++)
  {
    OLED_ShowChar(Line, Column + i, String[i]);
  }
}

/**
 * @brief  OLED 幂运算辅助函数
 *
 * @param  X 底数
 * @param  Y 指数
 * @retval 返回 X 的 Y 次方
 */
uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
  uint32_t Result = 1;
  while (Y--)
  {
    Result *= X;
  }
  return Result;
}

/**
 * @brief  OLED 显示数字（十进制，无符号整数）
 *
 * @param  Line   起始行位置，范围：1~4
 * @param  Column 起始列位置，范围：1~16
 * @param  Number 要显示的数字，范围：0~4294967295
 * @param  Length 要显示的数字位数，范围：1~10
 * @retval 无
 *
 * @details 将数字按指定位数逐位提取并显示，不足位数前导补零
 */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
  uint8_t i;
  for (i = 0; i < Length; i++)
  {
    OLED_ShowChar(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0');
  }
}

/**
 * @brief  OLED 显示数字（十进制，有符号整数）
 *
 * @param  Line   起始行位置，范围：1~4
 * @param  Column 起始列位置，范围：1~16
 * @param  Number 要显示的数字，范围：-2147483648~2147483647
 * @param  Length 要显示的数字位数（不含符号位），范围：1~10
 * @retval 无
 *
 * @details 自动判断正负并显示符号，正数显示 '+'，负数显示 '-'
 */
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
  uint8_t i;
  uint32_t Number1;
  if (Number >= 0)
  {
    OLED_ShowChar(Line, Column, '+');
    Number1 = Number;
  }
  else
  {
    OLED_ShowChar(Line, Column, '-');
    Number1 = -Number;
  }
  for (i = 0; i < Length; i++)
  {
    OLED_ShowChar(Line, Column + i + 1, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0');
  }
}

/**
 * @brief  OLED 显示数字（十六进制，无符号整数）
 *
 * @param  Line   起始行位置，范围：1~4
 * @param  Column 起始列位置，范围：1~16
 * @param  Number 要显示的数字，范围：0~0xFFFFFFFF
 * @param  Length 要显示的十六进制位数，范围：1~8
 * @retval 无
 */
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
  uint8_t i, SingleNumber;
  for (i = 0; i < Length; i++)
  {
    SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
    if (SingleNumber < 10)
    {
      OLED_ShowChar(Line, Column + i, SingleNumber + '0'); // 0~9
    }
    else
    {
      OLED_ShowChar(Line, Column + i, SingleNumber - 10 + 'A'); // A~F
    }
  }
}

/**
 * @brief  OLED 显示数字（二进制，无符号整数）
 *
 * @param  Line   起始行位置，范围：1~4
 * @param  Column 起始列位置，范围：1~16
 * @param  Number 要显示的数字，范围：0~0xFFFFFFFF
 * @param  Length 要显示的二进制位数，范围：1~16
 * @retval 无
 */
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
  uint8_t i;
  for (i = 0; i < Length; i++)
  {
    OLED_ShowChar(Line, Column + i, Number / OLED_Pow(2, Length - i - 1) % 2 + '0');
  }
}

/* ======================== OLED 初始化 ======================== */

/**
 * @brief  OLED 初始化
 *
 * @param  无
 * @retval 无
 *
 * @details 按照 SSD1306 数据手册推荐的初始化序列配置 OLED 显示参数：
 *          1. 上电延时等待 OLED 电源稳定
 *          2. 初始化 I2C 引脚
 *          3. 依次发送配置命令（时钟分频、复用率、偏移、方向、对比度等）
 *          4. 开启显示并清屏
 */
void OLED_Init(void)
{
  uint32_t i, j;

  for (i = 0; i < 1000; i++) // 上电延时，等待 OLED 电源稳定
  {
    for (j = 0; j < 1000; j++)
      ;
  }

  OLED_I2C_Init(); // 初始化 I2C 通信引脚

  OLED_WriteCommand(0xAE); // 关闭显示

  OLED_WriteCommand(0xD5); // 设置显示时钟分频因子/振荡频率
  OLED_WriteCommand(0x80);

  OLED_WriteCommand(0xA8); // 设置多路复用率（1/64 duty）
  OLED_WriteCommand(0x3F);

  OLED_WriteCommand(0xD3); // 设置显示偏移
  OLED_WriteCommand(0x00);

  OLED_WriteCommand(0x40); // 设置显示开始行 [5:0]

  OLED_WriteCommand(0xA1); // 设置段映射方向：0xA1 正常，0xA0 左右反置

  OLED_WriteCommand(0xC8); // 设置 COM 扫描方向：0xC8 正常，0xC0 上下反置

  OLED_WriteCommand(0xDA); // 设置 COM 引脚硬件配置
  OLED_WriteCommand(0x12);

  OLED_WriteCommand(0x81); // 设置对比度控制
  OLED_WriteCommand(0xCF);

  OLED_WriteCommand(0xD9); // 设置预充电周期
  OLED_WriteCommand(0xF1);

  OLED_WriteCommand(0xDB); // 设置 VCOMH 电压取消选择级别
  OLED_WriteCommand(0x30);

  OLED_WriteCommand(0xA4); // 0xA4=跟随 RAM 内容显示，0xA5=全部点亮

  OLED_WriteCommand(0xA6); // 0xA6=正常显示，0xA7=反色显示

  OLED_WriteCommand(0x8D); // 设置电荷泵
  OLED_WriteCommand(0x14); // 开启电荷泵（必须开启，否则无显示）

  OLED_WriteCommand(0xAF); // 开启显示

  OLED_Clear(); // 清屏，防止上电时显示随机内容
}

/* ======================== OLED 显示任务（FreeRTOS） ======================== */

/**
 * @brief  OLED 显示更新 FreeRTOS 任务
 *
 * @param  argument 任务参数（未使用，FreeRTOS 任务函数签名要求）
 * @retval 无（死循环任务，永不返回）
 *
 * @details 以 osDelay(500) 约 1 秒为周期刷新 OLED 显示内容，布局如下：
 *
 *          第1行（Line 1）：温度 + 湿度
 *            "temp:XXhumi:YY"
 *            - XX: DHT11 温度值（°C）
 *            - YY: DHT11 湿度值（%RH）
 *
 *          第2行（Line 2）：MQ-7 ADC 原始采样值
 *            "adc:XXXX"
 *            - XXXX: MQ-7 模块 ADC 采样值（12位，0~4095）
 *
 *          第3行（Line 3）：CO 浓度 PPM 计算值
 *            "ppm:XXXX"
 *            - XXXX: 一氧化碳浓度（ppm）
 *
 *          第4行（Line 4）：人体检测 + CO 超标警告
 *            左半部分："People!" 或空格（未检测到人时清除显示）
 *            右半部分："Warning!" 或空格（CO 浓度正常时清除显示）
 *
 * @note   第4行警告条件：mq7_adc_value > 4000 且 ppm > 4000 同时满足
 */
void oled_task(void * argument)
{
  while (1)
  {
    /* ---- 第1行：显示温度和湿度（DHT11） ---- */
    OLED_ShowString(1, 1, "temp:");    // 温度标签
    OLED_ShowNum(1, 6, temp, 2);       // 温度值（2位数）

    OLED_ShowString(1, 9, "humi:");    // 湿度标签
    OLED_ShowNum(1, 15, humi, 2);      // 湿度值（2位数）

    /* ---- 第2行：显示 MQ-7 ADC 原始采样值 ---- */
    OLED_ShowString(2, 1, "adc:");     // ADC 标签
    OLED_ShowNum(2, 5, mq7_adc_value, 4); // ADC 值（4位数）

    /* ---- 第3行：显示 CO 浓度 PPM 值 ---- */
    OLED_ShowString(3, 1, "ppm:");     // PPM 标签
    OLED_ShowNum(3, 5, ppm, 4);        // PPM 值（4位数）

    /* ---- 第4行左半部分：显示人体检测状态 ---- */
    if (hc_sr501_value == 1)
    {
      OLED_ShowString(4, 1, "People!"); // 检测到人体，显示 "People!"
    }
    else
    {
      OLED_ShowString(4, 1, "       "); // 未检测到人体，用空格清除显示
    }

    /* ---- 第4行右半部分：显示 CO 超标警告 ---- */
    if ((mq7_adc_value > 4000) && (ppm > 4000))
    {
      // ADC 值和 PPM 值均超过 4000，显示 CO 超标警告
      OLED_ShowString(4, 9, "Warning!");
    }
    else
    {
      // CO 浓度正常，用空格清除警告显示
      OLED_ShowString(4, 9, "        ");
    }

    osDelay(500); // 延时约 1 秒，进入下一次显示刷新
  }
}
