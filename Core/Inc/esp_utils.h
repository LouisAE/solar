#ifndef ESP_UTILS_H
#define ESP_UTILS_H

// 约定：使用ESP-AT，TCP使用单连接模式
#include <stdint.h>

// 检查模块是否正常
int ESP_Check_Status();

// 检查WIFI状态;2:已经正常连接
int ESP_Check_WIFI_Status();

// 设置WIFI模式,0-4,0:AP模式，1:客户端模式
int ESP_Set_WIFI_Mode(uint8_t mode);

// 发送连接指令，只要指令发出去了就返回成功
int ESP_Connect_To_AP(const uint8_t* ssid, uint8_t ssid_len, const uint8_t* passwd, uint8_t pwd_len);

// 发送连接TCP服务器指令，仅支持IPv4，只要指令发出去了就返回成功
int ESP_TCP_Connect(const uint8_t* host, uint8_t host_len,uint16_t port);

// 断开TCP连接，只发送命令不管结果
int ESP_TCP_Disconnect();

// 检查TCP状态，仅能检查是否有连接
int ESP_TCP_Check_Status();

// 发送数据
int ESP_TCP_Send(uint8_t* data, uint16_t data_len);
#endif