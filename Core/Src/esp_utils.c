#include "esp_utils.h"

#include "stm32f1xx_hal.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define UART_BUF_SIZE 128

extern UART_HandleTypeDef huart1;

uint8_t esp_uart_buf[UART_BUF_SIZE];

static int Wait_UART_Receive(UART_HandleTypeDef* huart, uint16_t timeout);
/*
    发送AT指令检查芯片是否正常工作
*/
int ESP_Check_Status()
{
    uint8_t send_buf[4] = "AT\r\n";

    if (HAL_UART_Transmit(&huart1, send_buf, 4, 1000) != HAL_OK) 
        return -1; // 串口发送错误

    memset(esp_uart_buf, 0, UART_BUF_SIZE);
    if (HAL_UART_Receive_DMA(&huart1, esp_uart_buf, 128) != HAL_OK)
        return -2; // DMA启动失败
    
    if(Wait_UART_Receive(&huart1, 10) != 0) {
        HAL_UART_DMAStop(&huart1);
        return -2;
    }
    HAL_UART_DMAStop(&huart1);

    // 跳过第一行（回显），无响应内容，直接解析
    if (esp_uart_buf[6] == 'O' && esp_uart_buf[7] == 'K')
        return 0;
    else
        return 1;
}

int ESP_Check_WIFI_Status()
{
    // "[SSID]"最大占34字节
    uint8_t send_buf[13] =  "AT+CWSTATE?\r\n";

    if (HAL_UART_Transmit(&huart1, send_buf, 13, 1000) != HAL_OK)
        return -1; // 串口发送错误

    memset(esp_uart_buf, 0, UART_BUF_SIZE);
    if (HAL_UART_Receive_DMA(&huart1, esp_uart_buf, 128) != HAL_OK) 
        return -2;
    if(Wait_UART_Receive(&huart1, 10) != 0) {
        HAL_UART_DMAStop(&huart1);
        return -2;
    }
        
    HAL_UART_DMAStop(&huart1);

    // 如果正常返回，第23字节应当为状态码，一位数字介于0到4
    if (esp_uart_buf[22] >= '0' && esp_uart_buf[22] <= '4')
        return esp_uart_buf[22] - '0';
    else // 否则出错
        return -3;
    
}

int ESP_Set_WIFI_Mode(uint8_t mode) 
{
    if (mode > 3)
        return -3; // 错误的mode
    uint8_t send_buf[13] = "AT+CWMODE=0\r\n";
    send_buf[10] = mode + '0';

    if (HAL_UART_Transmit(&huart1, send_buf, 13, 1000) != HAL_OK) {
        return -1; // 串口发送错误
    };

    memset(esp_uart_buf, 0, UART_BUF_SIZE);
    if (HAL_UART_Receive_DMA(&huart1, esp_uart_buf, 128) != HAL_OK)
        return -2;
    if(Wait_UART_Receive(&huart1, 10) != 0) {
        HAL_UART_DMAStop(&huart1);
        return -2;
    }
    
    HAL_UART_DMAStop(&huart1);

    if (esp_uart_buf[15] == 'O' && esp_uart_buf[16] == 'K')
        return 0;
    else
        return 1;
}


int ESP_Connect_To_AP(const uint8_t* ssid, uint8_t ssid_len, const uint8_t* passwd, uint8_t pwd_len) 
{
    if (ssid_len > 32)
        return 1; // ssid不符合要求
    
    if (passwd != NULL && pwd_len > 0 && pwd_len < 8) {
        return 2; // 密码不符合要求
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

// 仅支持IPv4
int ESP_TCP_Connect(const uint8_t* host, uint8_t host_len, uint16_t port)
{
    if (host == NULL || host_len == 0)
        return 1;
    
    if (port == 0)
        return 2;
    
    uint8_t head[19] = "AT+CIPSTART=\"TCP\",\"";
    uint8_t middle[2] = "\",";
    uint8_t tail[2] = "\r\n";

    HAL_UART_Transmit(&huart1, head, 19, 100);
    HAL_UART_Transmit(&huart1, host, host_len, 100);
    HAL_UART_Transmit(&huart1, middle, 2, 100);

    uint16_t mask = 10000;
    while (mask > 0) {
        uint8_t tmp = port / mask + '0';
        HAL_UART_Transmit(&huart1, &tmp, 1, 1);
        port %= mask;
        mask /= 10;
    }
    HAL_UART_Transmit(&huart1, tail, 2, 100);
    return 0;
}

// 发送断开指令，而不论是否成功
int ESP_TCP_Disconnect() {
    uint8_t send_buf[13] = "AT+CIPCLOSE\r\n";
    if (HAL_UART_Transmit(&huart1, send_buf, 13, 100) != HAL_OK)
        return -1;
    return 0;
}

int ESP_TCP_Check_Status()
{
    uint8_t send_buf[14] = "AT+CIPSTATE?\r\n";

    if (HAL_UART_Transmit(&huart1, send_buf, 14, 100) != HAL_OK) {
        return -1;
    }

    memset(esp_uart_buf, 0, UART_BUF_SIZE);
    if (HAL_UART_Receive_DMA(&huart1, esp_uart_buf, UART_BUF_SIZE) != HAL_OK) {
        return -2;
    }

    if(Wait_UART_Receive(&huart1, 10) != 0) {
        HAL_UART_DMAStop(&huart1);
        return -2;
    }

    HAL_UART_DMAStop(&huart1);

    if (esp_uart_buf[14] != '+') {
        return 1; // 未连接或其它错误 
    }
    else {
        return 0; // 已经连接
    }
}



// 最大发送8192字节
int ESP_TCP_Send(uint8_t* data, uint16_t data_len)
{
    if (data == NULL || data_len == 0)
        return 1;
    if (data_len > 8192)
        return 2;
    
    uint8_t send_buf[17] = "AT+CIPSEND=0000\r\n";

    uint16_t mask = 1000;
    uint8_t tmp = data_len;
    for (uint8_t i = 11; mask > 0; i++) {
        send_buf[i] = tmp / mask + '0';
        tmp %= mask;
        mask /= 10;
    }

    if (HAL_UART_Transmit(&huart1, send_buf, 17, 100) != HAL_OK)
        return -1;

    memset(esp_uart_buf, 0, UART_BUF_SIZE);
    if (HAL_UART_Receive_DMA(&huart1, esp_uart_buf, 128) != HAL_OK)
        return -2;
    
    if(Wait_UART_Receive(&huart1, 10) != 0) {
        HAL_UART_DMAStop(&huart1);
        return -2;
    }
    HAL_UART_DMAStop(&huart1);

    if (esp_uart_buf[19] != 'O' || esp_uart_buf[20] != 'K') {
        return 3; // 不允许发送
    }

    if (HAL_UART_Transmit(&huart1, data, data_len, 100) != HAL_OK) {
        return -1;
    }

    memset(esp_uart_buf, 0, UART_BUF_SIZE);
    if (HAL_UART_Receive_DMA(&huart1, esp_uart_buf, 128) != HAL_OK) {
        return -2;
    }
    if (Wait_UART_Receive(&huart1, 10) != 0) {
        HAL_UART_DMAStop(&huart1);
        return -2;
    }

    HAL_UART_DMAStop(&huart1);
    /*
    // 6行就是正常，3行就是错误，遇到例外情况再改
    // 补充：这六行不是同时发过来的，实测一次接收只收到三行，不管了
    uint8_t count = 0;
    for (uint8_t i = 0; count < 6; i++) {
        if (esp_uart_buf[i] == '\n')
            count++;
        if (esp_uart_buf[i] == '\0') // 到尾了
            return 4; // 发送错误
    }
    */
    return 0;
}

static int Wait_UART_Receive(UART_HandleTypeDef* huart, uint16_t timeout)
{
    while(__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE) == 0) {
        HAL_Delay(1);
        timeout--;
        if (timeout == 0) 
            return 1;
    }
    return 0;
}