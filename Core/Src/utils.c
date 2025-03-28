#include "stm32f1xx_hal.h"

#include <stdio.h>
#include <stdbool.h>

extern UART_HandleTypeDef huart1;

int __io_putchar(int ch)
{
  HAL_UART_Transmit(&huart1, (uint8_t*)&ch, 1, 1000);
  return ch;
}

// 使用通用TIM进行延时，约定配置：时钟8MHz，正增加模式
// 该函数会改变htim的内容
int TIM_Delay(TIM_HandleTypeDef* htim, uint32_t ms)
{
    if (htim->State != HAL_TIM_STATE_READY && htim->State != HAL_TIM_STATE_RESET)
        return 1;

    if (ms > 8) {
        uint32_t count = ms / 8;
        ms %= 8;

        htim->Init.Period = 64000;
        htim->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
        HAL_TIM_Base_Init(htim);
        HAL_TIM_Base_Start(htim);
        while (count > 0) {
            while (__HAL_TIM_GET_FLAG(htim,TIM_FLAG_UPDATE) == 0);
            __HAL_TIM_CLEAR_FLAG(htim, TIM_FLAG_UPDATE);
            count--;
        }
        HAL_TIM_Base_Stop(htim);
    }

    if (ms != 0) {
        htim->Init.Period = ms * 8000;
        htim->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
        HAL_TIM_Base_Init(htim);
        HAL_TIM_Base_Start(htim);
        while (__HAL_TIM_GET_FLAG(htim,TIM_FLAG_UPDATE) == 0);
        __HAL_TIM_CLEAR_FLAG(htim, TIM_FLAG_UPDATE);
        HAL_TIM_Base_Stop(htim);
    }
    
    return 0;
}