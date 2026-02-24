#include "INMP441.h"

static i2s_config_t i2sConfig  = 
{
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 44100,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 128,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
};

static i2s_pin_config_t i2sPinConfig = 
{
    .mck_io_num = -1,
    .bck_io_num = INMP441_SCK,
    .ws_io_num = INMP441_WS,
    .data_out_num = -1,
    .data_in_num = INMP441_SD
};

const static uint8_t RecordTime = 60; //默认录音时间
const static uint32_t wavDataSize = RecordTime * 44100 * 4;
static WAV_HEADER wavhead;
static uint32_t ReceiveData[1024];
static size_t receiveSize;

void CreatWavHeader(WAV_HEADER &head, unsigned int numChannels, unsigned int sampleRate, unsigned short bitsPerSample)
{
    memcpy(head.riffType, "RIFF", 4);
    head.riffSize = wavDataSize + 44 - 8;
    memcpy(head.wavType, "WAVE", 4);
    memcpy(head.formateType, "fmt ", 4);
    head.formateSize = 16;
    head.compressionCode = 1;
    head.numChannels = numChannels;
    head.sampleRate = sampleRate;
    head.bytesPerSecond = sampleRate * numChannels * bitsPerSample / 8;
    head.blockAlign = numChannels * bitsPerSample / 8;
    head.bitsPerSample = bitsPerSample;
    memcpy(head.dataType, "data", 4);
    head.dataSize = wavDataSize;
}

INMP441Status INMP441_Init(File32 &file)
{
    esp_err_t err;
    err = i2s_driver_install(I2S_NUM_0, &i2sConfig, 0, NULL);
    if(err != ESP_OK)
    {
        Serial.println("i2s驱动安装失败");
        return INMP441_ERROR;
    }
    Serial.println("i2s驱动安装成功");

    err = i2s_set_pin(I2S_NUM_0, &i2sPinConfig);
    if(err != ESP_OK)
    {
        Serial.println("i2s引脚设置失败");
        return INMP441_ERROR;
    }
    Serial.println("i2s引脚设置成功");

    return INMP441_OK;
}

void INMP441_Start(File32 &file)
{
    CreatWavHeader(wavhead, 1, 44100, 32);
    file.write(&wavhead, 44);
}

void INMP441_Recording(File32 &file)
{
    i2s_read(I2S_NUM_0, ReceiveData, 1024 * 4, &receiveSize, portMAX_DELAY);
    file.write(ReceiveData, receiveSize);
}

void INMP441_Stop(File32 &file)
{
    uint32_t actualDataSize = file.size() - 44;

    wavhead.riffSize = actualDataSize + 44 - 8;
    wavhead.dataSize = actualDataSize;

    file.seek(0);
    file.write(&wavhead, 44);
    file.flush();
}
