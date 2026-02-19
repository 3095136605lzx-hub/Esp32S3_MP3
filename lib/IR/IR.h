#ifndef _IR_H_
#define _IR_H_

#include <Arduino.h>
#include "driver/rmt.h"

#define IR_RX_GPIO_NUM   GPIO_NUM_4
#define IR_TX_GPIO_NUM   GPIO_NUM_5

uint8_t IRTransmit_Init(void);
uint8_t IRReceive_Init(void);
void IRTransmit_Send(uint8_t data);
uint8_t IRReceive_GetCode(void);

#endif
