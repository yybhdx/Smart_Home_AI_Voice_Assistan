/**
 * @file    mydht11.c
 * @brief   DHT11 温湿度传感器驱动程序
 * @details 实现 DHT11 单总线通信协议，包括微秒级延时、复位时序、
 *          数据位/字节读取、校验以及 FreeRTOS 任务封装。
 * @note    硬件连接：DHT11 数据引脚接 PA8（GPIOA Pin 8）
 *          依赖 TIM1 实现微秒级延时
 *          使用 FreeRTOS + CMSIS-RTOS V2
 */

#include "mydht11.h"
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/**
 * @brief   微秒级延时函数（使用定时器1）
 * @param   us  延时的微秒数
 * @note    利用 TIM1 的计数器实现精确延时。先将计数器设为 (0xffff - us - 5)，
 *          然后启动定时器，等待计数器增长到接近 0xffff 时停止。
 *          减去 5 是为了补偿函数调用和寄存器操作的开销。
 */
void Delay_us(uint16_t us)
{
    /* 计算定时器初始值：从 (0xFFFF - us - 5) 开始向上计数 */
    uint16_t differ = 0xffff - us - 5;
    __HAL_TIM_SET_COUNTER(&htim1, differ); /* 设置定时器计数器初始值 */
    HAL_TIM_Base_Start(&htim1);            /* 启动定时器 */

    /* 循环等待计数器增长，直到达到 (0xFFFF - 5) 附近 */
    while (differ < 0xffff - 5)
    {
        differ = __HAL_TIM_GET_COUNTER(&htim1); /* 读取当前计数值 */
    }
    HAL_TIM_Base_Stop(&htim1); /* 停止定时器 */
}

/**
 * @brief   复位 DHT11 传感器（主机发送起始信号）
 * @note    单总线复位时序：
 *          1. 主机将数据线拉低，保持至少 18ms（此处用 HAL_Delay(20)）
 *          2. 主机释放数据线（拉高），等待 20~40us
 *          3. DHT11 检测到起始信号后，会拉低数据线 80us，再拉高 80us 作为应答
 */
void DHT11_Rst(void)
{
    DHT11_IO_OUT();  /* 将数据线切换为输出模式（主机发送信号） */
    DHT11_DQ_OUT(0); /* 主机拉低数据线 */
    HAL_Delay(20);   /* 保持低电平至少 18ms */
    DHT11_DQ_OUT(1); /* 主机释放数据线（拉高） */
    Delay_us(30);    /* 保持高电平 20~40us */
}

/**
 * @brief   检测 DHT11 应答信号（判断从机 DHT11 是否正常响应）
 * @return  0 表示检测到 DHT11；1 表示未检测到（超时）
 * @note    应答时序：
 *          - DHT11 收到起始信号后，先拉低数据线 40~80us，再拉高 40~80us
 *          - 第一个 while 等待数据线变低（DHT11 开始响应）
 *          - 第二个 while 等待数据线变高（DHT11 响应结束，准备发送数据）
 */
uint8_t DHT11_Check(void)
{
    uint8_t retry = 0;
    DHT11_IO_IN(); /* 将数据线切换为输入模式（准备接收从机信号） */

    /* 等待 DHT11 拉低数据线（应答信号开始），超时阈值 100us */
    while (DHT11_DQ_IN && retry < 100)
    {
        retry++;
        Delay_us(1);
    }
    if (retry >= 100)
        return 1; /* 超时未检测到 DHT11 响应 */

    retry = 0;

    /* 等待 DHT11 拉高数据线（应答信号结束），超时阈值 100us */
    while (!DHT11_DQ_IN && retry < 100)
    {
        retry++;
        Delay_us(1);
    }
    if (retry >= 100)
        return 1; /* 超时，DHT11 应答异常 */

    return 0; /* 成功检测到 DHT11 应答 */
}

/**
 * @brief   从 DHT11 读取一个数据位
 * @return  读取到的位值（0 或 1）
 * @note    DHT11 数据位编码原理：
 *          每个数据位都以约 50us 的低电平信号开始，然后通过高电平的持续时间区分 0 和 1：
 *          - 数据位 "0"：高电平持续约 26~28us
 *          - 数据位 "1"：高电平持续约 70us
 *          本函数通过延时 40us 后读取数据线电平来判断：
 *          - 若仍为高电平，说明高电平持续时间 > 40us，即数据位为 1
 *          - 若已变为低电平，说明高电平持续时间 < 40us，即数据位为 0
 */
uint8_t DHT11_Read_Bit(void)
{
    uint8_t retry = 0;

    /* 等待数据线变低（位传输的前导低电平约 50us） */
    while (DHT11_DQ_IN && retry < 100)
    {
        retry++;
        Delay_us(1);
    }

    retry = 0;
    /* 等待数据线变高（进入高电平部分，持续时间决定是 0 还是 1） */
    while (!DHT11_DQ_IN && retry < 100)
    {
        retry++;
        Delay_us(1);
    }

    /* 延时 40us 后判断电平状态，区分数据位 0 和 1 */
    Delay_us(40);

    if (DHT11_DQ_IN)
        return 1; /* 仍为高电平 → 高电平持续 > 40us → 数据位为 1 */
    else
        return 0; /* 已变低电平 → 高电平持续 < 40us → 数据位为 0 */
}

/**
 * @brief   从 DHT11 读取一个字节（8 位）
 * @return  读取到的字节数据
 * @note    连续调用 DHT11_Read_Bit() 8 次，按从高位到低位的顺序组装成一个字节。
 *          每次先左移 dat 腾出最低位，再将读取的 bit 放入最低位。
 */
uint8_t DHT11_Read_Byte(void)
{
    uint8_t i, dat;
    dat = 0;

    /* 循环读取 8 个数据位，从最高位(MSB)到最低位(LSB) */
    for (i = 0; i < 8; i++)
    {
        dat <<= 1;            /* 左移一位，为新 bit 腾出最低位位置 */
        dat |= DHT11_Read_Bit(); /* 读取一位并放入最低位 */
    }
    return dat;
}

/**
 * @brief   从 DHT11 读取一次完整数据
 * @param   temp 存储温度值的指针（单位：摄氏度，整数部分）
 * @param   humi 存储湿度值的指针（单位：%RH，整数部分）
 * @return  0 表示读取成功；1 表示读取失败
 * @note    DHT11 一次完整传输 40 位（5 字节）数据：
 *          - buf[0]：湿度整数部分
 *          - buf[1]：湿度小数部分（DHT11 通常为 0）
 *          - buf[2]：温度整数部分
 *          - buf[3]：温度小数部分（DHT11 通常为 0）
 *          - buf[4]：校验和 = buf[0] + buf[1] + buf[2] + buf[3]
 */
uint8_t DHT11_Read_Data(uint8_t *temp, uint8_t *humi)
{
    uint8_t buf[5]; /* 存储接收到的 5 字节数据 */
    uint8_t i;

    DHT11_Rst();            /* 发送复位信号（起始信号） */
    if (DHT11_Check() == 0) /* 检测 DHT11 应答，0 表示检测成功 */
    {
        /* 依次读取 5 个字节 */
        for (i = 0; i < 5; i++)
        {
            buf[i] = DHT11_Read_Byte();
        }

        /* 校验：前 4 字节之和等于第 5 字节则数据有效 */
        if ((buf[0] + buf[1] + buf[2] + buf[3]) == buf[4])
        {
            *humi = buf[0]; /* 湿度整数部分 */
            *temp = buf[2]; /* 温度整数部分 */
        }
        else
        {
            return 1; /* 校验和错误，读取失败 */
        }
    }
    else
    {
        return 1; /* 未检测到 DHT11 应答，读取失败 */
    }

    return 0; /* 读取成功 */
}

/**
 * @brief   初始化 DHT11 传感器
 * @return  0 表示初始化成功；1 表示初始化失败
 * @note    配置 PA8 为推挽输出模式，初始电平为高，然后发送复位信号检测传感器是否在线。
 *          同时启动 TIM1 用于微秒级延时。
 */
uint8_t DHT11_Init(void)
{
    /* 配置 DHT11 数据引脚 GPIO */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE(); /* 使能 GPIOA 时钟 */

    GPIO_InitStruct.Pin = GPIO_PIN_8;             /* PA8 引脚 */
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;   /* 推挽输出模式 */
    GPIO_InitStruct.Pull = GPIO_NOPULL;           /* 无上下拉 */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; /* 高速模式 */
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);       /* 初始化 GPIO */

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET); /* 初始化为高电平（空闲状态） */

    HAL_TIM_Base_Start(&htim1); /* 启动 TIM1 基础定时（用于微秒延时） */

    DHT11_Rst();          /* 发送复位信号 */
    return DHT11_Check(); /* 检测 DHT11 是否在线 */
}

/**
 * @brief   湿度数据全局变量（由 DHT11 读取任务更新）
 * @note    单位：%RH，整数部分
 */
uint8_t humi;

/**
 * @brief   温度数据全局变量（由 DHT11 读取任务更新）
 * @note    单位：摄氏度，整数部分
 */
uint8_t temp;

/**
 * @brief   DHT11 温湿度采集 FreeRTOS 任务函数
 * @param   argument 任务参数（未使用）
 * @note    任务流程：
 *          1. 调用 vTaskSuspendAll() 挂起 FreeRTOS 调度器
 *             —— DHT11 使用单总线微秒级时序通信，FreeRTOS 的上下文切换
 *                会打断时序导致数据读取失败，因此必须关闭调度器保护临界时序
 *          2. 读取 DHT11 数据（温度存入 temp，湿度存入 humi）
 *          3. 调用 xTaskResumeAll() 恢复调度器
 *          4. 通过 UART1 打印温湿度数据
 *          5. 延时 500 个 RTOS tick 后再次采集
 */
void dht11_task(void * argument)
{
    while (1)
    {
        /* 挂起 FreeRTOS 调度器，防止上下文切换打乱 DHT11 微秒级时序 */
        vTaskSuspendAll();

        /* 读取 DHT11 传感器数据，温度存储到 temp，湿度存储到 humi */
        if (DHT11_Read_Data(&temp, &humi) == 0)
        {
            xTaskResumeAll(); /* 恢复调度器 */
            /* 通过 UART1 打印温湿度数据 */
            my_printf(&huart1, "temp:%d,humi:%d\r\n", temp, humi);
        }
        else
        {
            xTaskResumeAll(); /* 恢复调度器 */
            my_printf(&huart1, "ERROR - Failed to read DHT11\r\n");
        }

        osDelay(500); /* 延时 500 个 tick 后再次采集 */
    }
}
