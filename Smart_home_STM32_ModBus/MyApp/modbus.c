#include "modbus.h"
#include "crc.h"
#include "usart.h"
#include "gpio.h"

#define RS485_TX_ENABLE()  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET)   // 使能RS485发送模式
#define RS485_RX_ENABLE()  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET) // 使能RS485接收模式


/**
 * @brief  通用 ModBus 主机发送函数
 * @note   该函数会自动计算并追加 2 字节 CRC 校验码，并处理 RS485 收发切换
 * @param  data: 待发送的数据缓冲区（该缓冲区大小应至少为 len + 2）
 * @param  len:  原始数据的长度（不含 CRC）
 * @retval None
 */
void ModBusMasterSend(uint8_t *data, uint16_t len)
{
    // 1. 计算当前数据包的 CRC16 校验码
    // ModBus RTU 协议要求数据包最后必须附带 2 字节的循环冗余校验码
    uint16_t crc = modbus_crc16(data, len);

    // 2. 将计算出的 CRC 值追加到数据末尾（小端模式：先发低字节，再发高字节）
    data[len] = crc & 0xFF;           // CRC 低字节
    data[len + 1] = (crc >> 8) & 0xFF; // CRC 高字节

    // 3. 硬件层面：将 RS485 芯片切换到“发送模式”
    // MAX3485 等芯片是半双工的，发送前必须拉高 RE/DE 引脚
    RS485_TX_ENABLE(); 

    // 4. 调用 HAL 库串口阻塞发送函数
    // 发送总长度 = 原始长度 + 2 字节 CRC
    HAL_UART_Transmit(&huart1, data, len + 2, HAL_MAX_DELAY);

    // 5. 关键步骤：等待 UART 硬件移位寄存器中的数据彻底发送完成 (TC = Transmission Complete)
    // 必须等待 TC 标志，否则如果立即切换回接收模式，会导致数据包的最后一个字节还没发完就被截断
    while(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_TC) == RESET);

    // 6. 硬件层面：将 RS485 芯片切换回“接收模式”，准备接收从机的响应
    RS485_RX_ENABLE(); 
}

/**
 * @brief 发送 ModBus 读取寄存器请求 (03H 或 04H)
 * @param slave_addr 从机地址
 * @param func_code  功能码 (03H: 读保持寄存器, 04H: 读输入寄存器)
 * @param reg_addr   寄存器起始地址
 * @param reg_qty    读取寄存器数量
 */
void ModBus_Read_Registers(uint8_t slave_addr, uint8_t func_code, uint16_t reg_addr, uint16_t reg_qty)
{
    uint8_t msg[8]; // ModBus 读寄存器请求固定为 8 字节（含2字节CRC）

    msg[0] = slave_addr;
    msg[1] = func_code;
    msg[2] = (reg_addr >> 8) & 0xFF; // 地址高位
    msg[3] = reg_addr & 0xFF;        // 地址低位
    msg[4] = (reg_qty >> 8) & 0xFF;  // 数量高位
    msg[5] = reg_qty & 0xFF;         // 数量低位

    // 调用通用发送函数（它会自动计算 CRC 并处理 RS485 切换）
    ModBusMasterSend(msg, 6);
}


