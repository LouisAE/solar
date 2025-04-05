#include "modbus.h"

int ModBus_CRC(uint8_t* data, uint8_t length, uint16_t* result)
{
    if (data == NULL || result == NULL || length == 0)
        return 1;

    uint16_t res = 0xffff;
    uint8_t tail = 0;
    
    for (int i = 0; i < length; i++) {
        res ^= data[i];
        for (int j = 0; j < 8; j++) {
            tail = res & 0x1;
            res >>= 1;
            if (tail == 1) 
                res ^= 0xa001;
        }
    }

    *result = (res >> 8) + (res << 8);
    return 0;
}