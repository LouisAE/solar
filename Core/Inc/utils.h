#ifndef UTILS_H
#define UTILS_H
#include "stm32f1xx_hal.h"

int __io_putchar(int ch);
int TIM_Delay(TIM_HandleTypeDef htim, uint32_t ms);
#endif