#ifndef ESP_UTILS_H
#define ESP_UTILS_H

// 约定：esp32uart串口波特率115200，使用ESP-AT
#include <stdint.h>

int ESP_Check_Status();
int ESP_Check_WIFI_Status();
int ESP_Set_WIFI_Mode(uint8_t mode);
int ESP_Connect_To_AP(const uint8_t* ssid, uint8_t ssid_len, const uint8_t* passwd, uint8_t pwd_len);
#endif