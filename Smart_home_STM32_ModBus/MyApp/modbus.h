#include <stdint.h>

#ifndef MODBUS_H
#define MODBUS_H

void ModBusMasterSend(uint8_t *data, uint16_t len);

/**
 * @brief 发送 ModBus 读取寄存器请求 (03H 或 04H)
 * @param slave_addr 从机地址
 * @param func_code  功能码 (03H: 读保持寄存器, 04H: 读输入寄存器)
 * @param reg_addr   寄存器起始地址
 * @param reg_qty    读取寄存器数量
 */
void ModBus_Read_Registers(uint8_t slave_addr, uint8_t func_code, uint16_t reg_addr, uint16_t reg_qty);

#endif // !ODEBUS_H
