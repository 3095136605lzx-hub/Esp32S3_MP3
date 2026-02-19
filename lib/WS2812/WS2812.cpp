#include "WS2812.h"

rmt_config_t WS2812_RMT
{
    .rmt_mode = RMT_MODE_TX,
    .channel = RMT_CHANNEL_0,
    .gpio_num = GPIO_NUM_48,
    .clk_div = 8,
    .mem_block_num = 1,
    .tx_config =
        {
            .carrier_freq_hz = 0,
            .loop_en = false,
        },
};

rmt_item32_t item[25];

void WS282_Init(void)
{
    rmt_config(&WS2812_RMT);
    rmt_driver_install(WS2812_RMT.channel, 0, 0);
}

void ws2812_set_pixel(uint8_t r, uint8_t g, uint8_t b) 
{
    uint32_t color = (g << 16) | (r << 8) | b; // WS2812 使用 GRB 顺序
    uint32_t mask = 1 << 23;
    
    for (int i = 0; i < 24; i++) {
        if (color & mask) {
            // 发送比特 '1'
            item[i].duration0 = 8.5;
            item[i].level0 = 1;
            item[i].duration1 = 4;
            item[i].level1 = 0;
        } else {
            // 发送比特 '0'
            item[i].duration0 = 4;
            item[i].level0 = 1;
            item[i].duration1 = 8.5;
            item[i].level1 = 0;
        }
        mask >>= 1;
    }
}

void WS2812_Updata(void)
{
    item[24].duration0 = 1000; // 100us 复位时间
    item[24].level0 = 0;
    item[24].duration1 = 0;
    item[24].level1 = 0;

    rmt_write_items(WS2812_RMT.channel, item, 25, true);
}
