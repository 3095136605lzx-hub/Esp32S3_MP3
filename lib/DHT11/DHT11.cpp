#include "DHT11.h"

void DHT11_Init()
{
    pinMode(DHT11_DataPin, OUTPUT_OPEN_DRAIN);
    digitalWrite(DHT11_DataPin, HIGH);
}

void DHT11_Start(void)
{
    uint16_t timeout = 0;

    digitalWrite(DHT11_DataPin, LOW);
    delay(20); //至少18ms
    digitalWrite(DHT11_DataPin, HIGH);
    delayMicroseconds(20);

    while(digitalRead(DHT11_DataPin)); //等待DHT11拉低
    {
        timeout++;
        delayMicroseconds(1);
        if(timeout > 100) //超时处理
        {
            return;
        }
    }
    timeout = 0;

    while(!digitalRead(DHT11_DataPin)); //等待DHT11拉高
    {
        timeout++;
        delayMicroseconds(1);
        if(timeout > 100) //超时处理
        {
            return;
        }
    }

    timeout = 0;

    while(digitalRead(DHT11_DataPin)); //等待DHT11拉低
    {
        timeout++;
        delayMicroseconds(1);
        if(timeout > 100) //超时处理
        {
            return;
        }
    }
}

uint8_t DHT11_ReadBit(void)
{
    uint16_t timeout = 0;

    while(!digitalRead(DHT11_DataPin)); //等待DHT11拉高
    {
        timeout++;
        delayMicroseconds(1);
        if(timeout > 100) //超时处理
        {
            return 0;
        }
    }
    timeout = 0;

    delayMicroseconds(40);
    if(digitalRead(DHT11_DataPin)) //高电平持续时间>40us则为1，否则为0
    {
        while(digitalRead(DHT11_DataPin)); //等待DHT11拉低
        {
            timeout++;
            delayMicroseconds(1);
            if(timeout > 100) //超时处理
            {
                return 0;
            }
        }
        return 1;
    }
    else
    {
        return 0;
    }
}

uint8_t DHT11_ReadByte(void)
{
	uint8_t i, data = 0;
	for(i = 0; i < 8; i++)
	{
		data <<= 1;
		data |= DHT11_ReadBit();
	}
	return data;
}

uint8_t DHT11_ReadData(uint8_t* temperature, uint8_t* humidity)
{
	uint8_t i, state;
	uint8_t buf[5];
	for(i = 0; i < 5; i++)
	{
		buf[i] = DHT11_ReadByte();
	}

	if(buf[4] == (buf[0] + buf[1]+ buf[2]+ buf[3]))
	{
		state = 1;
		*humidity = buf[0];
		*temperature = buf[2];
	}
	else
		state = 0;

	return state;
}

