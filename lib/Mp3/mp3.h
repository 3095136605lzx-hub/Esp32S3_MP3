#ifndef _MP3_H_
#define _MP3_H_

#include <Arduino.h>
#include "mp3dec.h"
#include <SdFat.h>
#include <SPI.h>
#include <driver/i2s.h>

// 引脚定义
#define SD_SCK  7
#define SD_MISO 17
#define SD_MOSI 15
#define SD_SS   16

#define I2S_BCLK 42
#define I2S_LRC  41
#define I2S_DOUT 39

extern int mp3_volume; // 音量控制 (0-256)
// #define mp3_volume 10 // 音量控制 (0-256)
#define mp3InputBufferSize   (1024 * 4) // 4KB buffer for MP3 data
#define pcmOutputBufferSize  (1152 * 2) // Buffer for PCM output (max samples per MP3 frame * number of channels * bytes per sample)


enum Mp3Status 
{
    MP3_STATUS_OK = 0,
    MP3_STATUS_ERROR = 1,
    MP3_STATUS_DONE = 2,        // 播放完成或文件末尾
    MP3_STATUS_CONTINUE = 3     // 需要更多数据继续解码，即重新调用解码函数
};

struct Mp3Info
{
    String title;
    String artist;
};

struct AudioBlock
{
    short pcmData[pcmOutputBufferSize];
    int size; // 实际有效数据大小，单位为字节
    int SampleRate;
    int Channels;
};

Mp3Status myMp3_Init(void);
Mp3Status myMp3CheckID3(File32 &file, Mp3Info &mp3Info);
Mp3Status myMp3Decode(File32 &file, AudioBlock &audioBlock);
Mp3Status myMp3Play(AudioBlock &audioBlock);
void mymp3Reset(File32 &file);

#endif
