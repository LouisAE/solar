#ifndef UTILS_H
#define UTILS_H
#include "stm32f1xx_hal.h"

int __io_putchar(int ch);
int TIM_Delay(TIM_HandleTypeDef htim, uint32_t ms);
int ESP_Init();
int Handle_Message(uint8_t* msg, uint16_t len);

typedef enum {
    Normal, // 普通状态（WIFI和TCP都连接）
    TCP_Disconnected, // WIFI连接但TCP未连接
    WIFI_Disconnected // WIFI断开
    
} Stat;

static Stat Global_Stat;
#endif