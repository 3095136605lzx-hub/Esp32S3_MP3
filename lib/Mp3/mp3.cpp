#include "mp3.h"

int mp3_volume = 10; // 音量控制 (0-256)
static unsigned char mp3InputBuffer[mp3InputBufferSize] = {0};

static i2s_config_t i2sConfig = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = 44100,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 512,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
};

static i2s_pin_config_t pinConfig = {
    .mck_io_num = -1,
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num = -1
};

static SdFat sd;
static SPIClass myspi;
// File32 file;
static File32 root;
static HMP3Decoder mp3Decoder;
static MP3FrameInfo frameInfo;
static esp_err_t err;
static int bytesLeft = 0;        // 缓冲区中剩余字节数
static unsigned char* readPtr = mp3InputBuffer;  // 当前读取位置

Mp3Status myMp3_Init(void)
{
    err = i2s_driver_install(I2S_NUM_0, &i2sConfig, 0, NULL);
    if (err != ESP_OK) {
        Serial.println("I2S驱动安装失败");
        return MP3_STATUS_ERROR;
    }
    Serial.println("I2S驱动安装成功");

    err = i2s_set_pin(I2S_NUM_0, &pinConfig);
    if (err != ESP_OK) {
        Serial.println("I2S引脚设置失败");
        return MP3_STATUS_ERROR;
    }
    Serial.println("I2S引脚设置成功");

    mp3Decoder = MP3InitDecoder();
    if (mp3Decoder == NULL) {
        Serial.println("MP3解码器初始化失败");
        return MP3_STATUS_ERROR;
    }
    Serial.println("MP3解码器初始化成功");

    myspi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_SS); // SCK, MISO, MOSI, SS
    if(!sd.begin(SdSpiConfig(SD_SS, SHARED_SPI, 8000000, &myspi))) 
    {
        Serial.println("SD卡初始化失败");
        return MP3_STATUS_ERROR;
    }
    Serial.println("SD卡初始化成功");

    // root.open("/", O_READ);
    // if (!root) {
    //     Serial.println("Failed to open root directory!");
    //     return MP3_STATUS_ERROR;
    // }
    // Serial.println("Root directory contents:");

    // while (true) {
    //     File32 entry = root.openNextFile(O_READ);
    //     if (!entry) {
    //         break; // No more files
    //     }
    //     char filename[64];
    //     entry.getName(filename, sizeof(filename));
    //     String name = String(filename);
    //     if(name.endsWith(".mp3")) 
    //     {
    //         Serial.print("MP3文件: ");
    //         Serial.print(filename);
    //         Serial.print("  大小: ");
    //         Serial.println(entry.size());
    //     } 
    //     entry.close();
    // }

    return MP3_STATUS_OK;
}

uint32_t getID3Size(uint8_t* buffer) 
{
    // ID3v2标签大小使用同步安全整数编码（每个字节最高位为0）
    // 注意：这是大端序
    uint32_t size = 0;
    size = ((uint32_t)(buffer[6] & 0x7F) << 21) |
           ((uint32_t)(buffer[7] & 0x7F) << 14) |
           ((uint32_t)(buffer[8] & 0x7F) << 7)  |
           (buffer[9] & 0x7F);
    return size;
}

// 将 ID3 帧中的 UTF-16LE 数据转换为 UTF-8 字符串
String utf16leToUtf8(uint8_t* data, int len) 
{
    String result = "";
    if (len < 2) return result;

    int start = 0;
    // 检查并跳过 BOM (FF FE)
    if (data[0] == 0xFF && data[1] == 0xFE) {
        start = 2;
    }

    for (int i = start; i < len - 1; i += 2) {
        // 获取 Unicode 码点 (小端序：低位在前，高位在后)
        uint16_t unicode = data[i] | (data[i + 1] << 8);

        // 简单的 Unicode 转 UTF-8 逻辑 (覆盖常用中英文字符)
        if (unicode < 0x80) {
            result += (char)unicode;
        } else if (unicode < 0x800) {
            result += (char)(0xC0 | (unicode >> 6));
            result += (char)(0x80 | (unicode & 0x3F));
        } else {
            result += (char)(0xE0 | (unicode >> 12));
            result += (char)(0x80 | ((unicode >> 6) & 0x3F));
            result += (char)(0x80 | (unicode & 0x3F));
        }
    }
    return result;
}

void parseID3Metadata(File32 &file, uint32_t totalSize, Mp3Info &mp3Info) 
{
    uint8_t status = 0;
    uint32_t readSize = 0;
    // 假设当前文件指针已经在 ID3 头部之后（即第 10 字节处）
    
    while (readSize < totalSize) 
    {
        char frameID[5] = {0};
        file.read(frameID, 4);   // 读取 "TIT2", "TPE1" 等
        
        uint32_t frameLen;
        uint8_t lenBuf[4];
        file.read(lenBuf, 4);
        // ID3v2.3 帧大小是标准 4 字节大端整数
        frameLen = (lenBuf[0] << 24) | (lenBuf[1] << 16) | (lenBuf[2] << 8) | lenBuf[3];
        
        uint8_t flags[2];
        file.read(flags, 2);
        
        if (frameLen == 0 || frameLen > 1024 * 50) break; // 安全保护

        if (memcmp(frameID, "TIT2", 4) == 0 || memcmp(frameID, "TPE1", 4) == 0) 
        {
            uint8_t encoding;
            file.read(&encoding, 1); // 读取编码标识位
            
            uint32_t dataLen = frameLen - 1;
            uint8_t* buffer = (uint8_t*)malloc(dataLen);
            file.read(buffer, dataLen);
            
            if (encoding == 0x01) // UTF-16LE
            {
                String info = utf16leToUtf8(buffer, dataLen);
                // Serial.printf("%s: %s\n", frameID, info.c_str());
                if(memcmp(frameID, "TIT2", 4) == 0) {
                    mp3Info.title = info;
                } else if(memcmp(frameID, "TPE1", 4) == 0) {
                    mp3Info.artist = info;
                }
            } 
            else { // 0x00 为 ISO-8859-1 (ASCII)
                // Serial.printf("%s: %s\n", frameID, buffer);
                if(memcmp(frameID, "TIT2", 4) == 0) {
                    mp3Info.title = String((char*)buffer);
                } else if(memcmp(frameID, "TPE1", 4) == 0) {
                    mp3Info.artist = String((char*)buffer);
                }
            }
            free(buffer);
            status++;
        } 
        else {
            // 跳过不感兴趣的帧（如 APIC 封面，太大，不建议直接读入内存）
            file.seek(file.position() + frameLen);
        }

        if(status >= 2) 
            break; // 已经获取到歌曲名和艺术家，退出循环
        
        readSize += (10 + frameLen);
    }
}

Mp3Status myMp3CheckID3(File32 &file, Mp3Info &mp3Info) 
{
    int bytesRead = file.read(mp3InputBuffer, mp3InputBufferSize);
    if (bytesRead < 0) {
        Serial.println("读取文件失败");
        return MP3_STATUS_ERROR;
    }
    Serial.printf("读取了 %d 字节的MP3数据\n", bytesRead);
    bytesLeft = bytesRead;

    // 检查是否有ID3v2标签
    if(bytesRead >= 10 && memcmp(mp3InputBuffer, "ID3", 3) == 0) 
    {
        uint32_t id3Size = getID3Size(mp3InputBuffer);
        Serial.printf("检测到ID3v2标签,大小: %d 字节\n", id3Size);
        file.seek(10); // 跳过ID3头部
        parseID3Metadata(file, id3Size, mp3Info); // 解析ID3元数据并打印
        if(file.seek(id3Size + 10))
        {
            Serial.println("成功跳过ID3标签");
            readPtr = mp3InputBuffer; // 重置读取指针
            bytesLeft = 0; // 清空缓冲区，等待下一轮读取
        }
        else
        {
            Serial.println("跳过ID3标签失败,可能是因为文件指针移动失败");
            readPtr = mp3InputBuffer; // 重置读取指针
            bytesLeft = 0; // 清空缓冲区，等待下一轮读取
            // 继续使用原始数据进行解码，虽然可能包含ID3标签，但至少不会丢失数据
        }
    } 
    else
        Serial.println("未检测到ID3v2标签");

    return MP3_STATUS_OK;
}

Mp3Status myMp3Decode(File32 &file, AudioBlock &audioBlock) 
{
    if(bytesLeft < 1024 && bytesLeft >= 0 && file.available())
    {
        memmove(mp3InputBuffer, readPtr, bytesLeft); // 将剩余数据移动到缓冲区开始位置
        int bytesRead = file.read(mp3InputBuffer + bytesLeft, mp3InputBufferSize - bytesLeft);
        if (bytesRead < 0) {
            Serial.println("读取文件失败");
            return MP3_STATUS_ERROR;
        }
        // Serial.printf("读取了 %d 字节的MP3数据\n", bytesRead);
        readPtr = mp3InputBuffer; // 重置读取指针
        bytesLeft += bytesRead;
    }

    if(bytesLeft < 4 && !file.available()) 
    {
        Serial.println(">>> 播放完成！");
        return MP3_STATUS_DONE;
    }

    int sycnOffset = MP3FindSyncWord(readPtr, bytesLeft);
    if (sycnOffset < 0) 
    {
        if(file.available()) 
        {
            bytesLeft = 0; // 清空缓冲区，等待下一轮读取
            Serial.println("当前数据中未找到MP3同步字,但文件中仍有数据可读,继续读取...");
            return MP3_STATUS_CONTINUE; // 继续等待更多数据
        }
        else
        {
            // 如果文件读完了也找不到同步字，说明到头了
            Serial.println(">>> 到达文件末尾，未发现更多有效帧");
            return MP3_STATUS_DONE;
        }
    }

    readPtr += sycnOffset;
    bytesLeft -= sycnOffset;
    int decodeResult = MP3Decode(mp3Decoder, &readPtr, &bytesLeft, audioBlock.pcmData, 0);
    if (decodeResult < 0) {
        Serial.printf("MP3解码失败,错误码: %d\n", decodeResult);
        // free(audiodecodeBlock);
        if(file.available()) 
        {
            Serial.println("解码失败，但文件中仍有数据可读，继续读取...");
            readPtr++;
            bytesLeft--;
            return MP3_STATUS_CONTINUE; // 继续等待更多数据
        }
        else
        {
            Serial.println(">>> 到达文件末尾，未发现更多有效帧");
            return MP3_STATUS_DONE;
        }
    }
    else
    {
        MP3GetLastFrameInfo(mp3Decoder, &frameInfo);

        int samples = frameInfo.outputSamps; // 总采样点数
        // 调整音量：遍历所有采样点
        for (int i = 0; i < samples; i++) 
        {
            // 将采样值乘以音量因子并右移8位以实现0-256的音量控制
            audioBlock.pcmData[i] = (short)((audioBlock.pcmData[i] * mp3_volume) >> 8);
        }

        audioBlock.size = samples * (frameInfo.bitsPerSample / 8); // 计算实际数据大小
        audioBlock.SampleRate = frameInfo.samprate;
        audioBlock.Channels = frameInfo.nChans;
        // Serial.printf("数据大小： %d字节\n", audioBlock.size);

        return MP3_STATUS_OK;
    }
}

Mp3Status myMp3Play(AudioBlock &audioBlock) 
{
    static int lastSampleRate = 44100;
    static int lastChannels = 2;
    // 如果采样率或通道数发生变化，重新配置I2S
    if (abs(audioBlock.SampleRate - lastSampleRate) > 100 || audioBlock.Channels != lastChannels)
    {
        i2s_set_clk(I2S_NUM_0, audioBlock.SampleRate, I2S_BITS_PER_SAMPLE_16BIT, (i2s_channel_t)audioBlock.Channels);
        lastSampleRate = audioBlock.SampleRate;
        lastChannels = audioBlock.Channels;
    }

    size_t bytesWritten;
    i2s_write(I2S_NUM_0, audioBlock.pcmData, audioBlock.size, &bytesWritten, portMAX_DELAY);
    if(bytesWritten != audioBlock.size) 
    {
        Serial.printf("I2S写入失败,期望写入: %d 字节, 实际写入: %d 字节\n", audioBlock.size, bytesWritten);
        return MP3_STATUS_ERROR;
    }

    return MP3_STATUS_OK;
}

void mymp3Reset(File32 &file)
{
    bytesLeft = 0;
    readPtr = mp3InputBuffer;
    file.close();
}
