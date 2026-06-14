#include <stdint.h>
#ifndef TEMP_HUMI_H
#define TEMP_HUMI_H

extern uint8_t humi;
extern uint8_t temp;

extern uint8_t rx_buffer[100];    // 串口接收缓冲区
extern volatile uint8_t rx_flag;  // 接收完成标志
extern volatile uint16_t rx_size; // 接收到的数据长度

void Temp_Humi_Read(void *argument);

#endif // !TEMP_HUMI_H
