#include "esp_utils.h"

#include "stm32f1xx_hal.h"

#include <stdint.h>

extern UART_HandleTypeDef huart1;


/*
    发送AT指令检查芯片是否正常工作
*/
int ESP_Check_Status()
{
    uint8_t recv_buf[7], send_buf[4] = "AT\r\n";
    const uint8_t ok[2] = "OK";


    if (HAL_UART_Transmit(&huart1, (uint8_t*)send_buf, 4, 1000) != HAL_OK) 
        return -1; // 串口发送错误


    if (HAL_UART_Receive(&huart1, (uint8_t*)recv_buf, 6, 1000) != HAL_OK) // 接收回显以及换行
        return -2; // 串口接收错误

    uint8_t* ptr = recv_buf;
    while(1) {
        if (HAL_UART_Receive(&huart1, &ptr, 1, 100) != HAL_OK)
            return -2;
        if (*ptr == '\n' || ptr == &recv_buf[6]) // 达到缓冲区上限或到达结尾
            break;
        ptr++;
    }
        
    for (int i = 0; i < 2; i++) {
        if (ok[i] != recv_buf[i]) {
            return 1; // ESP返回错误
        }
    }
    return 0;
}

int ESP_Check_WIFI_Status()
{
    // "[SSID]"最大占34字节
    uint8_t recv_buf[34];
    uint8_t send_buf[13] =  "AT+CWSTATE?\r\n";
    uint8_t ret_status;

    if (HAL_UART_Transmit(&huart1, (uint8_t*)send_buf, 13, 1000) != HAL_OK) {
        return -1; // 串口发送错误
    }

    //接收状态信息
    if (HAL_UART_Receive(&huart1, (uint8_t*)recv_buf, 11, 1000) != HAL_OK) {
        return -2; // 串口接收错误
    }

    ret_status = recv_buf[9];

    // 逐字节接收后续内容，共三行，但丢弃
    uint8_t tmp, count = 3;
    while(1) {
        if (HAL_UART_Receive(&huart1, &tmp, 1, 100) != HAL_OK) {
            return -2;
        }
        if (tmp == '\n') {
            if (count == 0)
                break;
            else 
                count--;
        }
    }

    if (ret_status >= '0' && ret_status <= '4')
        return (int)(recv_buf - '0');
    else 
        return -3; // 错误的状态
}

int ESP_Set_WIFI_Mode(uint8_t mode) 
{
    if (mode > 3)
        return -3; // 错误的mode
    uint8_t send_buf[13] = "AT+CWMODE=0\r\n", recv_buf[15];
    const uint8_t ok[4] = "OK";
    send_buf[10] = mode + '0';

    if (HAL_UART_Transmit(&huart1, (uint8_t*)send_buf, 13, 1000) != HAL_OK) {
        return -1; // 串口发送错误
    };

    if (HAL_UART_Receive(&huart1, (uint8_t*)recv_buf, 15, 1000) != HAL_OK) {
        return -2; // 串口接收错误
    }

    uint8_t* ptr = recv_buf;
    while(1) {
        if(HAL_UART_Receive(&huart1, ptr, 1, 100) != HAL_OK) {
            return -2;
        }

        if (*ptr == '\n' || ptr == &recv_buf[14])
            break;
        ptr++;
    }

    for (int i = 0; i < 2; i++) {
        if (ok[i] != recv_buf[i])
            return 1;
    }
    return 0;
}


int ESP_Connect_To_AP(const char* ssid, const char* passwd) 
{
    
}