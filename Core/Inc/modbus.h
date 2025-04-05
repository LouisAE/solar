#ifndef MODBUS_H
#define MODBUS_H
#include <stdint.h>

typedef struct  {
    uint8_t addr;
    uint8_t func_code;
} ModBus_Metadata;

int ModBus_CRC(uint8_t* data, uint8_t size, uint16_t* result);

int ModBus_Construct_Frame(uint8_t addr, uint8_t func_code, uint8_t* data, uint8_t data_len, uint8_t* result);

#endif
