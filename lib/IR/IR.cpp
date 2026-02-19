#include "IR.h"

static rmt_config_t rmt_rx_config = {
    .rmt_mode = RMT_MODE_RX,                      // 接收模式
    .channel = RMT_CHANNEL_4,                     // 使用通道
    .gpio_num = IR_RX_GPIO_NUM,                       // 接收引脚
    .clk_div = 80,                               // 时钟分频
    .mem_block_num = 1,                          // 内存块数量
    .flags = 0,                                  // 无特殊标志
    .rx_config = {
        .idle_threshold = 18000,                // 空闲阈值(大于起始码长度)
        .filter_ticks_thresh = 100,             // 滤波器阈值(100ticks = 100us)
        .filter_en = true,                      // 启用滤波器
    }
};

static RingbufHandle_t rb = NULL;
static rmt_item32_t* items = NULL;

static rmt_config_t rmt_tx_config = {
    .rmt_mode = RMT_MODE_TX,                      // 发送模式
    .channel = RMT_CHANNEL_3,                     // 使用通道
    .gpio_num = IR_TX_GPIO_NUM,                       // 发送引脚
    .clk_div = 80,                               // 时钟分频
    .mem_block_num = 1,                          // 内存块数量
    .flags = 0,                                  // 无特殊标志
    .tx_config = {
        .carrier_freq_hz = 38000,                // 载波频率38kHz
        .carrier_level = RMT_CARRIER_LEVEL_HIGH, // 载波高电平有效
        .idle_level = RMT_IDLE_LEVEL_LOW,        // 空闲低电平
        .carrier_duty_percent = 33,              // 占空比33%
        .carrier_en = true,                      // 启用载波
        .loop_en = false,                        // 不循环发送
        .idle_output_en = true,                  // 启用空闲输出
    }
};

static rmt_item32_t ir_tx_items[34]; // NEC协议需要34个符号发送

uint8_t IRTransmit_Init(void)
{
    esp_err_t err = rmt_config(&rmt_tx_config);
    if(err != ESP_OK) {
        Serial.println("RMT TX config failed");
        return 0;
    }

    err = rmt_driver_install(rmt_tx_config.channel, 0, 0);
    if (err != ESP_OK) {
        Serial.println("RMT TX driver install failed");
        return 0;
    }

    return 1;
}


uint8_t IRReceive_Init(void)
{
    esp_err_t err = rmt_config(&rmt_rx_config);
    if(err != ESP_OK) {
        Serial.println("RMT config failed");
        return 0;
    }

    err = rmt_driver_install(rmt_rx_config.channel, 1000, 0);
    if (err != ESP_OK) {
        Serial.println("RMT driver install failed");
        return 0;
    }

    rmt_get_ringbuf_handle(rmt_rx_config.channel, &rb);
    
    rmt_rx_start(rmt_rx_config.channel, true);

    return 1;
}

void IRTransmit_SetItems(uint8_t data)
{
    uint8_t i = 0;
    // 起始码
    ir_tx_items[0].level0 = 1;
    ir_tx_items[0].duration0 = 9000;
    ir_tx_items[0].level1 = 0;
    ir_tx_items[0].duration1 = 4500;
    // 用户码
    for(i = 0; i < 8; i++)
    {
        ir_tx_items[i + 1].level0 = 1;
        ir_tx_items[i + 1].duration0 = 562;
        ir_tx_items[i + 1].level1 = 0;
        ir_tx_items[i + 1].duration1 = 562;
    }
    // 用户反码
    for(i = 0; i < 8; i++)
    {
        ir_tx_items[i + 9].level0 = 1;
        ir_tx_items[i + 9].duration0 = 562;
        ir_tx_items[i + 9].level1 = 0;
        ir_tx_items[i + 9].duration1 = 1687;
    }
    // 数据码
    for(i = 0; i < 8; i++)
    {
        ir_tx_items[i + 17].level0 = 1;
        ir_tx_items[i + 17].duration0 = 562;
        ir_tx_items[i + 17].level1 = 0;
        if(data & (0x01 << i))
        {
            ir_tx_items[i + 17].duration1 = 1687; // 逻辑1
        }
        else
        {
            ir_tx_items[i + 17].duration1 = 562;  // 逻辑0
        }
    }
    // 数据反码
    for(i = 0; i < 8; i++)
    {
        ir_tx_items[i + 25].level0 = 1;
        ir_tx_items[i + 25].duration0 = 562;
        ir_tx_items[i + 25].level1 = 0;
        if((~data) & (0x01 << i))
        {
            ir_tx_items[i + 25].duration1 = 1687; // 逻辑1
        }
        else
        {
            ir_tx_items[i + 25].duration1 = 562;  // 逻辑0
        }
    }
}

void IRTransmit_Send(uint8_t data)
{
    IRTransmit_SetItems(data);
    // 结束符
    ir_tx_items[33].level0 = 1;
    ir_tx_items[33].duration0 = 562;
    ir_tx_items[33].level1 = 0;
    ir_tx_items[33].duration1 = 0;

    rmt_write_items(rmt_tx_config.channel, ir_tx_items, 34, true);

    rmt_wait_tx_done(rmt_tx_config.channel, portMAX_DELAY);
}

rmt_item32_t* IRReceive_GetItems(size_t* item_num, TickType_t wait_ticks)
{
    size_t rx_size = 0;
    items = (rmt_item32_t*) xRingbufferReceive(rb, &rx_size, wait_ticks);
    if (items != NULL) 
    {
        *item_num = rx_size / sizeof(rmt_item32_t);
        vRingbufferReturnItem(rb, (void*)items);
    } 
    else 
    {
        *item_num = 0;
    }
    return items;
}

uint8_t IRReceive_GetCode(void)
{
    size_t item_num = 0;
    items = IRReceive_GetItems(&item_num, portMAX_DELAY);

    uint8_t code = 0;
    if (item_num >= 33)  // NEC协议至少33个符号
    {
        uint8_t i = 0;
        uint8_t buf[4] = {0};
        if(items[0].level0 == 0 && items[0].duration0 > 8000 && items[0].duration0 < 10000 && items[0].level1 == 1 && items[0].duration1 > 4000 && items[0].duration1 < 5000)
        {
            // 起始码检测通过
            for (i = 0; i < 32; i++)
            {
                if (items[i + 1].level0 == 0 && items[i + 1].duration0 > 400 && items[i + 1].duration0 < 650 && items[i + 1].level1 == 1)
                {
                    // 逻辑0或逻辑1
                    if (items[i + 1].duration1 > 1500 && items[i + 1].duration1 < 1800)
                    {
                        buf[i / 8] >>= 1;
                        buf[i / 8] |= 0x80;  // 逻辑1
                    }
                    else if(items[i + 1].duration1 > 450 && items[i + 1].duration1 < 650)
                    {
                        buf[i / 8] >>= 1;    // 逻辑0
                    }
                    else
                        break;
                }
                else
                {
                    break;
                }
            }
            if(i == 32)
            {
                // 数据接收完整
                if(buf[0] == 0x00 && buf[1] == 0xFF && (buf[2] + buf[3] == 0xFF))
                {
                    // 校验通过
                    // Serial.print("Received NEC IR Code: 0x");
                    // Serial.print(buf[2], HEX);
                    // Serial.println();
                    return buf[2];
                }
                else
                {
                    // Serial.println("Checksum error");
                }
            }
            
        }
    }
    return 0;
}
