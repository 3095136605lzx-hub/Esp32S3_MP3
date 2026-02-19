#include "OLED.h"
#include "OLED_Font.h"

#define OLED_ADDR 0x3C

void OLED_WriteCommand(uint8_t command)
{
    Wire.beginTransmission(OLED_ADDR);
    Wire.write(0x00); 
    Wire.write(command);
    Wire.endTransmission();
}

void OLED_WriteData(uint8_t data)
{
    Wire.beginTransmission(OLED_ADDR);
    Wire.write(0x40); 
    Wire.write(data);
    Wire.endTransmission();
}

void OLED_WriteDatas(uint8_t* data, uint16_t count)
{
    Wire.beginTransmission(OLED_ADDR);
    Wire.write(0x40);
    Wire.write(data, count);
    Wire.endTransmission();
}

void OLED_Init(void)
{
    Wire.begin(SDA_Pin, SCL_Pin);
    delay(100); 

    OLED_WriteCommand(0xAE); // Display OFF
    OLED_WriteCommand(0x20); // Set Memory Addressing Mode
    OLED_WriteCommand(0x10); // 00,Horizontal Addressing Mode;01,Vertical Addressing Mode;10,Page Addressing Mode (RESET);11,Invalid
    OLED_WriteCommand(0xB0); // Set Page Start Address for Page Addressing Mode,0-7
    OLED_WriteCommand(0xC8); // Set COM Output Scan Direction       
    OLED_WriteCommand(0x00); // ---set low column address
    OLED_WriteCommand(0x10); // ---set high column address
    OLED_WriteCommand(0x40); // --set start line address
    OLED_WriteCommand(0x81); // --set contrast control register
    OLED_WriteCommand(0xFF); // Set SEG Output Current Brightness
    OLED_WriteCommand(0xA1); // Set SEG/Column Mapping     0xA0左右反置 0xA1正常
    OLED_WriteCommand(0xA6); // Set Normal/Inverse Display 0xA6正常 0xA7反相显示
    OLED_WriteCommand(0xA8); // --set multiplex ratio(1 to 64)
    OLED_WriteCommand(0x3F); // --1/64 duty
    OLED_WriteCommand(0xA4); // 0xa4,Output follows RAM content;0xa5,Output ignores RAM content
    OLED_WriteCommand(0xD3); // -set display offset
    OLED_WriteCommand(0x00); // -not offset
    OLED_WriteCommand(0xD5); // --set display clock divide ratio/oscillator frequency
    OLED_WriteCommand(0xF0); // --set divide ratio
    OLED_WriteCommand(0xD9); // --set pre-charge period
    OLED_WriteCommand(0x22); // --set pre-charge period
    OLED_WriteCommand(0xDA); // --set com pins hardware configuration
    OLED_WriteCommand(0x12);
    OLED_WriteCommand(0xDB); // --set vcomh
    OLED_WriteCommand(0x20); // 0x20,0.77xVcc
    OLED_WriteCommand(0x8D); // --set DC-DC enable
    OLED_WriteCommand(0x14); //
    OLED_WriteCommand(0xAF); // Display ON

    // delay(100); 

    OLED_Clear();
}

void OLED_SetCursor(uint8_t x, uint8_t page)
{
    OLED_WriteCommand(0x00 | (x & 0x0F)); // Set Lower Column Start Address
    OLED_WriteCommand(0x10 | ((x & 0xF0) >> 4)); // Set Higher Column Start Address
    OLED_WriteCommand(0xB0 | page); // Set Page Start Address
}

void OLED_Clear(void)
{
    uint8_t i;
    uint8_t writebuf[128] = {0};
    for(i = 0; i < 8; i++)
    {
        OLED_WriteCommand(0x01 | (0 & 0x0F)); // Set Lower Column Start Address
        OLED_WriteCommand(0x10 | ((0 & 0xF0) >> 4)); // Set Higher Column Start Address
        OLED_WriteCommand(0xB0 | i); // Set Page Start Address
        OLED_WriteDatas(writebuf, 128);
    }
}

void OLED_ShowChar(uint8_t x, uint8_t page, char str)
{
	uint8_t i, j;
	uint8_t writebuffer[8];
	for(i = 0; i < 2; i++)
	{
		OLED_SetCursor(x, page + i);
		for(j = 0; j < 8; j++)
		{
			writebuffer[j] = OLED_F8x16[str - ' '][j + i * 8];
		}
		OLED_WriteDatas(writebuffer, 8);
	}
}

void OLED_ShowString(uint8_t x, uint8_t page, const char* String)
{
	uint8_t i;
	for(i = 0; String[i] != '\0'; i++)
	{
		OLED_ShowChar(x + i * 8, page, String[i]);
	}
}

uint32_t OLED_Pow(uint32_t x, uint32_t y)
{
	uint32_t result = 1;
	while (y--)
	{
		result *= x;
	}
	return result;
}

void OLED_ShowUintNum(uint8_t x, uint8_t page, uint32_t num, uint16_t length)
{
	uint16_t i;
	
	for(i = 0; i < length; i++)
	{
		OLED_ShowChar(x + i * 8, page, num / OLED_Pow(10, length - i - 1) % 10 + '0');
	}
}

void OLED_ShowNum(uint8_t x, uint8_t page, int32_t num, uint16_t length)
{
	uint16_t i;
	if(num < 0)
	{
		OLED_ShowChar(x, page, '-');
		num = -num;
	}
	else
	{
		OLED_ShowChar(x, page, '+');
	}
	x += 8;
	
	for(i = 0; i < length; i++)
	{
		OLED_ShowChar(x + i * 8, page, num / OLED_Pow(10, length - i - 1) % 10 + '0');
	}
}

void OLED_ShowHexNum(uint8_t x, uint8_t page, uint32_t num, uint16_t length)
{
	uint16_t i, SingleNum;
	OLED_ShowString(x, page, "0x");
	x += 16;
	for(i = 0; i < length; i++)
	{
		SingleNum = num / OLED_Pow(16, length - i - 1) % 16;
		if(SingleNum < 10)
		{
			OLED_ShowChar(x + i * 8, page, SingleNum + '0');
		}
		else
		{
			OLED_ShowChar(x + i * 8, page, SingleNum - 10 + 'A');
		}
	}
}

void OLED_ShowBinNum(uint8_t x, uint8_t page, uint32_t num, uint16_t length)
{
	uint16_t i;
	for(i = 0; i < length; i++)
	{
		OLED_ShowChar(x + i * 8, page, num / OLED_Pow(2, length - i - 1) % 2 + '0');
	}
}

void OLED_ShowFloat(uint8_t x, uint8_t page, float num, uint16_t intLength, uint16_t pointLength)
{
	int32_t intpart = num;
	float pointpart = num - intpart;
	if(pointpart < 0)
	{
		pointpart = -pointpart;
	}
	pointpart *= OLED_Pow(10, pointLength);
		
	OLED_ShowNum(x, page, intpart, intLength);
	OLED_ShowChar(x + intLength * 8 + 8, page, '.');
	OLED_ShowUintNum(x + intLength * 8 + 16, page, pointpart, pointLength);
}
