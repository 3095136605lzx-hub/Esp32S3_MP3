#ifndef _WS2812_H_
#define _WS2812_H_

#include <Arduino.h>
#include <driver/rmt.h>

void WS282_Init(void);
void ws2812_set_pixel(uint8_t r, uint8_t g, uint8_t b);
void WS2812_Updata(void);

#endif
