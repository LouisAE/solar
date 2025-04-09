#include "utils.h"
#include "esp_utils.h"

#include <stdio.h>
#include <stdbool.h>

extern UART_HandleTypeDef huart1;

// 比较两个字节数组开头len个字节是否相同，必须满足len<=max(size(a),size(b))
// 返回值：0,不相等; 1,相等
static int headcmp(const uint8_t* a, const uint8_t* b, uint16_t len);

int __io_putchar(int ch)
{
  HAL_UART_Transmit(&huart1, (uint8_t*)&ch, 1, 1000);
  return ch;
}

// 使用通用TIM进行延时，约定配置：时钟8MHz，正增加模式
// 该函数不会改变htim的内容
int TIM_Delay(TIM_HandleTypeDef htim, uint32_t ms)
{
    if (htim.State != HAL_TIM_STATE_READY && htim.State != HAL_TIM_STATE_RESET)
        return 1;

    if (ms > 8) {
        uint32_t count = ms / 8;
        ms %= 8;

        htim.Init.Period = 64000;
        htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
        HAL_TIM_Base_Init(&htim);
        HAL_TIM_Base_Start(&htim);
        while (count > 0) {
            while (__HAL_TIM_GET_FLAG(&htim,TIM_FLAG_UPDATE) == 0);
            __HAL_TIM_CLEAR_FLAG(&htim, TIM_FLAG_UPDATE);
            count--;
        }
        HAL_TIM_Base_Stop(&htim);
    }

    if (ms != 0) {
        htim.Init.Period = ms * 8000;
        htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
        HAL_TIM_Base_Init(&htim);
        HAL_TIM_Base_Start(&htim);
        while (__HAL_TIM_GET_FLAG(&htim,TIM_FLAG_UPDATE) == 0);
        __HAL_TIM_CLEAR_FLAG(&htim, TIM_FLAG_UPDATE);
        HAL_TIM_Base_Stop(&htim);
    }
    
    return 0;
}

// 初始化ESP模块的无线和TCP连接
int ESP_Init() 
{
    if (ESP_Check_Status() != 0) {
        HAL_Delay(1000);
        if (ESP_Check_Status() != 0) {
          return 1; // ESP模块可能工作不正常
        }
      }
    
      if (ESP_Check_WIFI_Status() != 2) {
        if (ESP_Set_WIFI_Mode(1) != 0) {
          return 2; // 设置WIFI模式失败，ESP模块可能工作不正常
        }
          
        if (ESP_Connect_To_AP(NULL, 0, NULL, 0) != 0) {
          return 3; // 发送连接指令失败，ESP模块可能工作不正常
        }
        HAL_Delay(5000);
      }
      
      if (ESP_Check_WIFI_Status() != 2) {
        return 4; // WIFI没能成功连接
      }
    
      ESP_TCP_Disconnect();
    
      if (ESP_TCP_Connect((uint8_t*)"192.168.137.1", 13, 1025) != 0) {
        return 5; // TCP连接指令发送失败，ESP模块可能工作不正常
      }
    
      HAL_Delay(2000);
    
      if (ESP_TCP_Check_Status() == 0) {
        return 0; // 初始化完毕
      }
      else {
        return 6; // TCP连接失败
      }
}

static int Handle_WIFI_Message(uint8_t* msg, uint16_t len) {
    return 0;
}

// 处理通过TCP接收到的消息
static int Handle_TCP_Message(uint8_t* msg, uint16_t len) {
    return 0;
}

static int Handle_TCP_Closed_Message(uint8_t msg, uint16_t len) {
    
}

int Handle_Message(uint8_t* msg, uint16_t len)
{
    if (headcmp((uint8_t*)"WIFI", msg, 4) != 0) {
        return Handle_WIFI_Message(msg, len);
    }

    if (headcmp((uint8_t*)"+IPD", msg, 4) != 0) {
        return Handle_TCP_Message(msg, len);
    }

    if (headcmp((uint8_t*)"CLOSED", msg, 6) != 0) {
        return Handle_TCP_Closed_Message(msg, len);
    }
    return -1; // 无法处理的消息
}

static int headcmp(const uint8_t* a, const uint8_t* b, uint16_t len)
{
    for (int i = 0; i < len; i++) {
        if(a[i] != b[i])
            return 0;
    }
    return 1;
}