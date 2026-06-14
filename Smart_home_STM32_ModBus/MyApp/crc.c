#include <stdint.h>
 
/**
 * @brief 计算 Modbus-RTU 报文的 CRC16 校验值
 * @param data  报文主体字节数组（不含 CRC）
 * @param len   报文主体长度
 * @return      16 位 CRC 值（高字节在前）
 */
uint16_t modbus_crc16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;  // 初始值
    uint8_t i;
 
    while (len--)
    {
        crc ^= *data++;     // 异或当前字节
        for (i = 0; i < 8; i++)  // 8 次移位循环
        {
            if (crc & 0x0001)  // 最低位为 1
                crc = (crc >> 1) ^ 0xA001;
            else               // 最低位为 0
                crc >>= 1;
        }
    }
    return crc;
}
 
// 测试示例
// int main(void)
// {
//     uint8_t frame[] = {0x01, 0x06, 0x00, 0x01, 0xAA, 0xBB};
//     uint16_t crc = modbus_crc16(frame, sizeof(frame));
    
//     // 输出：CRC 值 = 0xD9E6，报文 CRC 段 = 0xE6 0xD9
//     // crc & 0xFF  → 低字节 E6
//     // crc >> 8    → 高字节 D9
//     return 0;
// }