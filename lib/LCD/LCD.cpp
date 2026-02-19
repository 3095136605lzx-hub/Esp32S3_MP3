#include "LCD.h"
#include "LCD_Font.h"

#define LCD_Start()         digitalWrite(CS_PIN, LOW)
#define LCD_Stop()          digitalWrite(CS_PIN, HIGH)
#define LCD_Set_A0()        digitalWrite(A0_PIN, HIGH)
#define LCD_Clr_A0()        digitalWrite(A0_PIN, LOW)
#define LCD_Set_Reset()     digitalWrite(RESET_PIN, HIGH)
#define LCD_Clr_Reset()     digitalWrite(RESET_PIN, LOW)

// SPIClass LCD_SPI(HSPI);
SPIClass LCD_SPI;

void LCD_SendData_8(uint8_t ByteSend)
{
    LCD_SPI.beginTransaction(SPISettings(40000000, MSBFIRST, SPI_MODE0));
    LCD_Start();
    LCD_SPI.transfer(ByteSend);
    LCD_Stop();
    LCD_SPI.endTransaction();
}

void LCD_SendData_16(uint16_t ByteSend)
{
    LCD_SPI.beginTransaction(SPISettings(40000000, MSBFIRST, SPI_MODE0));
    LCD_Start();
    LCD_SPI.transfer16(ByteSend);
    LCD_Stop();
    LCD_SPI.endTransaction();
}

void LCD_SendCommand(uint8_t command)
{
	LCD_Clr_A0();
	LCD_SendData_8(command);
	LCD_Set_A0();
}

void LCD_Init(void)
{
    pinMode(RESET_PIN, OUTPUT);
    pinMode(A0_PIN, OUTPUT);
    pinMode(BL_PIN, OUTPUT);
    pinMode(CS_PIN, OUTPUT);

    digitalWrite(BL_PIN, HIGH); // 打开背光

    LCD_SPI.begin(CLK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);

	LCD_Stop();
	LCD_Set_A0();
	LCD_Set_Reset();    // 复位引脚拉高

    //************* Start Initial Sequence **********//
    LCD_SendCommand ( 0x11 ); //Sleep out
		//-----------------------ST7789V Frame rate setting-----------------//
	LCD_SendCommand(0x3A);        //65k mode
    LCD_SendData_8(0x05);
    LCD_SendCommand(0xC5); 		//VCOM
    LCD_SendData_8(0x1A);
    LCD_SendCommand(0x36);                 // 屏幕显示方向设置
    LCD_SendData_8(0x60);
	// LCD_SendData_8(0x00);
	//-------------ST7789V Frame rate setting-----------//
	LCD_SendCommand(0xB2);  //Porch Setting
	LCD_SendData_8(0x05);
	LCD_SendData_8(0x05);
	LCD_SendData_8(0x00);
	LCD_SendData_8(0x33);
	LCD_SendData_8(0x33);

    LCD_SendCommand(0xB7); //GCTRL (Gate Control)
    LCD_SendData_8(0x05); //12.2v   -10.43v
    //------------------------------------ST7735S Power Sequence-----------------------------------------//
	LCD_SendCommand(0xBB);//VCOM
    LCD_SendData_8(0x3F);

    LCD_SendCommand(0xC0); //Power control
    LCD_SendData_8(0x2c);

    LCD_SendCommand(0xC2);		//VDV and VRH Command Enable
    LCD_SendData_8(0x01);

    LCD_SendCommand(0xC3);			//VRH Set
    LCD_SendData_8(0x0F);		//4.3+( vcom+vcom offset+vdv)

    LCD_SendCommand(0xC4);			//VDV Set
    LCD_SendData_8(0x20);				//0v

    LCD_SendCommand(0xC6);				//Frame Rate Control in Normal Mode
    LCD_SendData_8(0X01);			//111Hz

    LCD_SendCommand(0xd0);				//Power Control 1
    LCD_SendData_8(0xa4);
    LCD_SendData_8(0xa1);

    LCD_SendCommand(0xE8);				//Power Control 1
    LCD_SendData_8(0x03);

    LCD_SendCommand(0xE9);				//Equalize time control
    LCD_SendData_8(0x09);
    LCD_SendData_8(0x09);
    LCD_SendData_8(0x08);
    //---------------------------------End ST7789V Power Sequence---------------------------------------//

    //------------------------------------ST7789V Gamma Sequence-----------------------------------------//
    LCD_SendCommand ( 0xE0 );
    LCD_SendData_8(0xD0);
	LCD_SendData_8(0x05);
	LCD_SendData_8(0x09);
	LCD_SendData_8(0x09);
	LCD_SendData_8(0x08);
	LCD_SendData_8(0x14);
	LCD_SendData_8(0x28);
	LCD_SendData_8(0x33);
	LCD_SendData_8(0x3F);
	LCD_SendData_8(0x07);
	LCD_SendData_8(0x13);
	LCD_SendData_8(0x14);
	LCD_SendData_8(0x28);
	LCD_SendData_8(0x30);

    LCD_SendCommand ( 0xE1 );
	LCD_SendData_8(0xD0);
	LCD_SendData_8(0x05);
	LCD_SendData_8(0x09);
    LCD_SendData_8(0x09);
    LCD_SendData_8(0x08);
    LCD_SendData_8(0x03);
    LCD_SendData_8(0x24);
    LCD_SendData_8(0x32);
    LCD_SendData_8(0x32);
    LCD_SendData_8(0x3B);
    LCD_SendData_8(0x14);
    LCD_SendData_8(0x13);
    LCD_SendData_8(0x28);
    LCD_SendData_8(0x2F);

//------------------------------------End ST7789V Gamma Sequence-----------------------------------------//
	LCD_SendCommand(0x20);  //反显

	LCD_SendCommand(0x11);

	LCD_SendCommand(0x29); //Display on
}

void LCD_SetOrientation(uint8_t orientation)
{
	LCD_SendCommand(0x36); // Memory Data Access Control
	switch(orientation)
	{
		case 0:
			LCD_SendData_8(0x00);
			break;
		case 1:
			LCD_SendData_8(0x60);
			break;
		case 2:
			LCD_SendData_8(0xC0);
			break;
		case 3:
			LCD_SendData_8(0xA0);
			break;
		default:
			break;
	}
}

void LCD_Set_Address(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    // 列地址设置 (0x2A)
    LCD_SendCommand(0x2A);
	LCD_SendData_16(x1);
	LCD_SendData_16(x2);

    // 行地址设置 (0x2B)
    LCD_SendCommand(0x2B);
	LCD_SendData_16(y1);
	LCD_SendData_16(y2);

    // 存储器写 (0x2C)
    LCD_SendCommand(0x2C);
}

void LCD_Fill(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint16_t color)
{
    uint16_t i, j;
    LCD_Set_Address(xsta, ysta, xend - 1, yend - 1); //设置显示范围

    for (i = ysta; i < yend; i++)
    {
        for (j = xsta; j < xend; j++)
        {
            LCD_SendData_16(color);     	
        }
    }
}

void LCD_ShowChar(uint16_t x, uint8_t y, char str)
{
    uint16_t i, j, temp;
    LCD_Set_Address( x, y, x + 8 - 1, y + 16 - 1 );

    for ( i = 0; i < 16; i++ )
    {
		temp = LCD_F8x16[str - ' '][i];
        for ( j = 0; j < 8; j++ )
        {
			if(temp & (0x01 << j))
			{
				LCD_SendData_16(WHITE);
			}
			else
				LCD_SendData_16(BLACK);
        }
    }
}

void LCD_ShowString(uint16_t x, uint8_t y, const char* String)
{
	uint8_t i;
	for(i = 0; String[i] != '\0'; i++)
	{
		LCD_ShowChar(x + i * 8, y, String[i]);
	}
}

uint32_t LCD_Pow(uint32_t x, uint32_t y)
{
	uint32_t result = 1;
	while (y--)
	{
		result *= x;
	}
	return result;
}

void LCD_ShowUintNum(uint16_t x, uint8_t y, uint32_t num, uint16_t length)
{
	uint16_t i;

	for(i = 0; i < length; i++)
	{
		LCD_ShowChar(x + i * 8, y, num / LCD_Pow(10, length - i - 1) % 10 + '0');
	}
}

void LCD_ShowNum(uint16_t x, uint8_t y, int32_t num, uint16_t length)
{
	uint16_t i;
	if(num < 0)
	{
		LCD_ShowChar(x, y, '-');
		num = -num;
	}
	else
		LCD_ShowChar(x, y, '+');
	x += 8;

	for(i = 0; i < length; i++)
	{
		LCD_ShowChar(x + i * 8, y, num / LCD_Pow(10, length - i - 1) % 10 + '0');
	}
}

void LCD_ShowHexNum(uint16_t x, uint8_t y, uint32_t num, uint16_t length)
{
	uint16_t i, SingleNum;
	LCD_ShowString(x, y, "0x");
	x += 16;
	for(i = 0; i < length; i++)
	{
		SingleNum = num / LCD_Pow(16, length - i - 1) % 16;
		if(SingleNum < 10)
		{
			LCD_ShowChar(x + i * 8, y, SingleNum + '0');
		}
		else
		{
			LCD_ShowChar(x + i * 8, y, SingleNum - 10 + 'A');
		}
	}
}

void LCD_ShowBinNum(uint16_t x, uint8_t y, uint32_t num, uint16_t length)
{
	uint16_t i;
	for(i = 0; i < length; i++)
	{
		LCD_ShowChar(x + i * 8, y, num / LCD_Pow(2, length - i - 1) % 2 + '0');
	}
}

void LCD_ShowFloat(uint16_t x, uint8_t y, float num, uint16_t intLength, uint16_t pointLength)
{
	int32_t intpart = num;
	float pointpart = num - intpart;
	if(pointpart < 0)
	{
		pointpart = -pointpart;
	}
	pointpart *= LCD_Pow(10, pointLength);

	LCD_ShowNum(x, y, intpart, intLength);
	LCD_ShowChar(x + intLength * 8 + 8, y, '.');
	LCD_ShowUintNum(x + intLength * 8 + 8 * 2, y, pointpart, pointLength);
}

void LCD_ShowChinese(uint16_t x, uint8_t y, const uint8_t Chinese_buffer[][32], uint8_t length)
{
    uint16_t i, j, k, temp;

	for(k = 0; k < length; k++)
	{
		LCD_Set_Address( x + 16 * k, y, x + 16 * (k + 1) - 1, y + 16 - 1);
		for (i = 0; i < 32; i++ )
		{
			temp = Chinese_buffer[k][i];
			for ( j = 0; j < 8; j++ )
			{
				if(temp & (0x01 << j))
				{
					LCD_SendData_16(WHITE);
				}
				else
					LCD_SendData_16(BLACK);
			}
		}
	}
}
