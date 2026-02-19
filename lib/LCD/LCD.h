#ifndef _LCD_H_
#define _LCD_H_

#include <Arduino.h>
#include <SPI.h>

#define CLK_PIN     12
#define MISO_PIN    -1
#define MOSI_PIN    11
#define CS_PIN      10
#define RESET_PIN    4
#define A0_PIN       5
#define BL_PIN       6

#define BLACK 	0X0000
#define WHITE 	0XFFFF
#define PINK   	0xD2F5
#define BLUE  	0x03BD
#define RED    	0xF882
#define YELLOW 	0xFFE0

// SPIClass LCD_SPI(HSPI);

void LCD_Init(void);
void LCD_SetOrientation(uint8_t orientation);
void LCD_Fill(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint16_t color);
void LCD_ShowChar(uint16_t x, uint8_t y, char str);
void LCD_ShowChar(uint16_t x, uint8_t y, char str);		//阴码，行列式，逆向
void LCD_ShowString(uint16_t x, uint8_t y, const char* String);
void LCD_ShowUintNum(uint16_t x, uint8_t y, uint32_t num, uint16_t length);
void LCD_ShowNum(uint16_t x, uint8_t y, int32_t num, uint16_t length);
void LCD_ShowHexNum(uint16_t x, uint8_t y, uint32_t num, uint16_t length);
void LCD_ShowBinNum(uint16_t x, uint8_t y, uint32_t num, uint16_t length);
void LCD_ShowFloat(uint16_t x, uint8_t y, float num, uint16_t intLength, uint16_t pointLength);

#endif
