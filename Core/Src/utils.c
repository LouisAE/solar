#include "utils.h"
#include "esp_utils.h"
#include "modbus.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>


#define MIN(a,b) (a > b ? b : a)

extern UART_HandleTypeDef huart1;
uint8_t adc_buf[ADC_BUF_SIZE];
Stat Global_Stat;

// 比较两个字节数组开头len个字节是否相同，必须满足len<=max(size(a),size(b))
// 返回值：0,不相等; 1,相等
static int headcmp(const uint8_t* a, const uint8_t* b, uint16_t len);

// 发送错误帧，func: 功能码， err:错误代码
static int Send_modbus_error(uint8_t func, uint8_t err);

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
    
      if (ESP_TCP_Connect((uint8_t*)"192.168.137.1", 13, 1502) != 0) {
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
    UNUSED(msg);
    UNUSED(len);
    return 0;
}

// 处理通过TCP接收到的消息
static int Handle_TCP_Message(uint8_t* msg, uint16_t len) {
    UNUSED(len);
    uint8_t* ptr = msg;
    while (*ptr != ':')
      ptr++;
    
    // 读取数据长度
    uint16_t data_len = 0;
    uint16_t base = 1;
    for (int i = 1; *(ptr - i) != ','; i++) {
      data_len += (*(ptr - i) - '0') * base;
      base *= 10;
    }

    // 解析modbus数据
    // 校验数据
    uint16_t crc;
    ModBus_CRC(ptr + 1, data_len - 2, &crc);
    if (crc != *((uint16_t*)(ptr + data_len - 1))) {
      return 0; // CRC校验错误，不响应
    }

    // 检查地址
    if (ptr[1] != 0x1) { // 不是发给我的
      return 0;
    }

    // 检查功能码
    
    switch (ptr[2]) {
      case 0x3: {
        // 检查请求的寄存器数量
        if (ptr[5] != 0x0 || ptr[6] > 0x7d) {
          // 发送错误响应：请求数量太大(0x3)
          Send_modbus_error(0x3, 0x3);
          break;
        }
        // 检查请求的起始地址，ADC共七个通道，因此从31到37
        if (ptr[3] != 0x75 || ptr[4] < 0x31 || ptr[4] > 0x37 || ptr[4] + ptr[6] > 0x37) {
          // 发送错误响应：越界(0x2)
          Send_modbus_error(0x3, 0x2);
          break;
        }

        uint8_t size = MIN(ptr[6] * 2, ADC_BUF_SIZE * 2);
        uint8_t* response = (uint8_t*)malloc(3 + size + 2);
        if (response == NULL) {
          return -3;
        }

        // 构造响应
        response[0] = 0x1;
        response[1] = 0x3;
        response[2] = size;
        memcpy(response + 3, adc_buf + (ptr[4] - 0x31), size);// 从起始地址开始复制size个字节到response[3]
        ModBus_CRC(response, 3 + size, (uint16_t*)(response + 3 + size));

        ESP_TCP_Send(response, 3 + size + 2);

        free(response);
        response = NULL;
        break;
      }
      default: {
        // 发送错误响应：不支持的功能码(0x1)
        Send_modbus_error(ptr[2], 0x1);
        break;
      }
    }
    return 0;
}

static int Handle_TCP_Closed_Message(uint8_t* msg, uint16_t len) {
    UNUSED(msg);
    UNUSED(len);
    Global_Stat = TCP_Disconnected;
    return 0;
}

int Handle_Message(uint8_t* msg, uint16_t len)
{
    if (headcmp((uint8_t*)"WIFI", msg, 4) != 0) {
        return Handle_WIFI_Message(msg, len);
    }

    if (headcmp((uint8_t*)"\r\n+IPD", msg, 6) != 0) {
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

static int Send_modbus_error(uint8_t func, uint8_t err) 
{
  uint8_t response[5];
  response[0] = 0x1;
  response[1] = func | 0x80;
  response[2] = err;
  ModBus_CRC(response, 3, (uint16_t*)response + 2);

  ESP_TCP_Send(response, 5);
  return 0;
}

int __io_putchar(int ch)
{
  HAL_UART_Transmit(&huart1, (uint8_t*)&ch, 1, 1000);
  return ch;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  UNUSED(htim);
  switch (Global_Stat) {
    case Normal: // 绿灯闪烁
      HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_4);
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
      break;
    case TCP_Disconnected: // 绿灯常亮
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
      break;
    case WIFI_Disconnected: // 红灯闪烁
      HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_3);
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
      break;
  }
}