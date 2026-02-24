#ifndef _INMP41_H_
#define _INMP441_H_

#include <Arduino.h>
#include "SdFat.h"
#include <driver/i2s.h>
#include <cstring>

#define INMP441_SCK 46
#define INMP441_WS  9
#define INMP441_SD  3

enum INMP441Status
{
    INMP441_OK,
    INMP441_ERROR,
};

struct WAV_HEADER
{
    char riffType[4];
    unsigned int riffSize;
    char wavType[4];
    char formateType[4];
    unsigned int formateSize;
    unsigned short compressionCode;
    unsigned short numChannels;
    unsigned int sampleRate;
    unsigned int bytesPerSecond;
    unsigned short blockAlign;
    unsigned short bitsPerSample;
    char dataType[4];
    unsigned int dataSize;
};

INMP441Status INMP441_Init(File32 &file);
void INMP441_Start(File32 &file);
void INMP441_Recording(File32 &file);
void INMP441_Stop(File32 &file);

#endif
