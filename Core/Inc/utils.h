#ifndef UTILS_H
#define UTILS_H
#include "stm32f1xx_hal.h"

#define ADC_BUF_SIZE 4


//int TIM_Delay(TIM_HandleTypeDef htim, uint32_t ms);
int ESP_Init();
// len是指msg缓冲区的大小，不是消息长度
int Handle_Message(uint8_t* msg, uint16_t len);

typedef enum {
    Normal, // 普通状态（WIFI和TCP都连接）
    TCP_Disconnected, // WIFI连接但TCP未连接
    WIFI_Disconnected // WIFI断开
} Stat;

extern Stat Global_Stat;
extern uint8_t adc_buf[ADC_BUF_SIZE];
#endif