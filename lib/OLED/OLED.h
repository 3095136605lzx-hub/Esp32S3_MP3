#ifndef _OLED_H_
#define _OLED_H_

#include <Arduino.h>
#include <Wire.h>

#define SCL_Pin		15
#define SDA_Pin		16

void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowChar(uint8_t x, uint8_t page, char str);
void OLED_ShowString(uint8_t x, uint8_t page, const char* String);
void OLED_ShowUintNum(uint8_t x, uint8_t page, uint32_t num, uint16_t length);
void OLED_ShowNum(uint8_t x, uint8_t page, int32_t num, uint16_t length);
void OLED_ShowHexNum(uint8_t x, uint8_t page, uint32_t num, uint16_t length);
void OLED_ShowBinNum(uint8_t x, uint8_t page, uint32_t num, uint16_t length);
void OLED_ShowFloat(uint8_t x, uint8_t page, float num, uint16_t intLength, uint16_t pointLength);

#endif
