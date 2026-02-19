#ifndef _DHT11_H_
#define _DHT11_H_

#include <Arduino.h>

#define DHT11_DataPin   42

void DHT11_Init();
void DHT11_Start(void);
uint8_t DHT11_ReadData(uint8_t* temperature, uint8_t* humidity);


#endif
