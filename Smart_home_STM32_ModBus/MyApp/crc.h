
#include <stdint.h>

#ifndef CRC_H
#define CRC_H

uint16_t modbus_crc16(const uint8_t *data, uint16_t len);

#endif /* CRC_H */