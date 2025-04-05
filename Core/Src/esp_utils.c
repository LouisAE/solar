#include "esp_utils.h"

#include "stm32f1xx_hal.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define UART_BUF_SIZE 128

extern UART_HandleTypeDef huart1;

uint8_t uart_buf[UART_BUF_SIZE];
/*
    发送AT指令检查芯片是否正常工作
*/
int ESP_Check_Status()
{
    uint8_t send_buf[4] = "AT\r\n";

    if (HAL_UART_Transmit(&huart1, (uint8_t*)send_buf, 4, 1000) != HAL_OK) 
        return -1; // 串口发送错误

    memset(uart_buf, 0, UART_BUF_SIZE);
    if (HAL_UART_Receive_DMA(&huart1, (uint8_t*)uart_buf, 128) != HAL_OK)
        return -2; // DMA启动失败
    
    while(!__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE));
    HAL_UART_DMAStop(&huart1);

    // 跳过第一行（回显），无响应内容，直接解析
    if (uart_buf[6] == 'O' && uart_buf[7] == 'K')
        return 0;
    else
        return 1;
}

int ESP_Check_WIFI_Status()
{
    // "[SSID]"最大占34字节
    uint8_t send_buf[13] =  "AT+CWSTATE?\r\n";

    if (HAL_UART_Transmit(&huart1, (uint8_t*)send_buf, 13, 1000) != HAL_OK)
        return -1; // 串口发送错误

    memset(uart_buf, 0, UART_BUF_SIZE);
    if (HAL_UART_Receive_DMA(&huart1, uart_buf, 128) != HAL_OK) 
        return -2;
    while(!__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE));
    HAL_UART_DMAStop(&huart1);

    // 如果正常返回，第23字节应当为状态码，一位数字介于0到4
    if (uart_buf[22] >= '0' && uart_buf[22] <= '4')
        return uart_buf[22] - '0';
    else // 否则出错
        return -3;
    
}

int ESP_Set_WIFI_Mode(uint8_t mode) 
{
    if (mode > 3)
        return -3; // 错误的mode
    uint8_t send_buf[13] = "AT+CWMODE=0\r\n";
    send_buf[10] = mode + '0';

    if (HAL_UART_Transmit(&huart1, (uint8_t*)send_buf, 13, 1000) != HAL_OK) {
        return -1; // 串口发送错误
    };

    memset(uart_buf, 0, UART_BUF_SIZE);
    if (HAL_UART_Receive_DMA(&huart1, uart_buf, 128) != HAL_OK)
        return -2;
    while(!__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE));
    HAL_UART_DMAStop(&huart1);

    if (uart_buf[15] == 'O' && uart_buf[16] == 'K')
        return 0;
    else
        return 1;
}


int ESP_Connect_To_AP(const uint8_t* ssid, uint8_t ssid_len, const uint8_t* passwd, uint8_t pwd_len) 
{
    if (ssid_len > 32)
        return -1; // ssid不符合要求
    
    if (passwd != NULL && pwd_len > 0 && pwd_len < 8) {
        return -2; // 密码不符合要求
    }
 
    const uint8_t head[10] = "AT+CWJAP=\"";
    const uint8_t middle[3] = "\",\"";
    const uint8_t tail[3] = "\"\r\n";

    if (ssid == NULL || ssid_len == 0) {// 未指定ssid，连接到默认ssid
        HAL_UART_Transmit(&huart1, head, 8, 100);
        HAL_UART_Transmit(&huart1, tail + 1, 2, 100);
    }
    else {
        HAL_UART_Transmit(&huart1, head, 9, 100);
        HAL_UART_Transmit(&huart1, ssid, ssid_len, 100);
        if (passwd != NULL && pwd_len != 0) {
            HAL_UART_Transmit(&huart1, middle, 3, 100);
            HAL_UART_Transmit(&huart1, passwd, pwd_len, 100);
        }
        HAL_UART_Transmit(&huart1, tail, 3, 100);
    }
    return 0;
}