#include <Arduino.h>
#include "mp3.h"
#include "IR.h"

// put function declarations here:
void mp3change(void* parameter);
void mp3decoder(void* parameter);
void mp3player(void* parameter);
void ajDectect_Task(void* parameter);
void IRReceive_Task(void* parameter);

void aj_Interrupt(void);

Mp3Status myMp3Status;
File32 root;
File32 file;
// AudioBlock audioBlock;
Mp3Info mp3Info;
String mp3FileName[20];
uint8_t mp3FileCount = 0;
int currentSongIndex = 0;

QueueHandle_t audioBlockQueue;
EventGroupHandle_t mp3EventGroup;
TaskHandle_t mp3ChangeTaskHandle = NULL;
TaskHandle_t mp3DecoderTaskHandle = NULL;
TaskHandle_t mp3PlayerTaskHandle = NULL;
TaskHandle_t ajDetectTaskHandle = NULL;
TaskHandle_t irReceiveTaskHandle = NULL;

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);

    pinMode(45, INPUT_PULLUP); // 设置GPIO45为输入并启用上拉电阻
    attachInterrupt(digitalPinToInterrupt(45), aj_Interrupt, RISING);

    IRReceive_Init();

    myMp3Status = myMp3_Init();
    if(myMp3Status != MP3_STATUS_OK) 
    {
        Serial.println("MP3初始化失败");
        while(1) { delay(1000); }
    }
    Serial.println("MP3初始化成功");

    root.open("/", O_READ);
    if (!root) {
        Serial.println("Failed to open root directory!");
        while(1) { delay(1000); }
    }
    Serial.println("Root directory contents:");

    while (true) 
    {
        File32 entry = root.openNextFile(O_READ);
        if (!entry) {
            break; // No more files
        }
        char filename[64];
        entry.getName(filename, sizeof(filename));
        String name = String(filename);
        if(name.endsWith(".mp3")) 
        {
            if(mp3FileCount < 20) 
            {
                mp3FileName[mp3FileCount] = name; // 存储MP3文件名
                mp3FileCount++;
            }
            Serial.print("MP3文件: ");
            Serial.print(filename);
            Serial.print("  大小: ");
            Serial.println(entry.size());
        } 
        entry.close();
    }

    // file.open("青花瓷 - 周杰伦.mp3", O_READ);
    // if (!file) {
    //     Serial.println("无法打开文件");
    //     while(1) { delay(1000); }
    // }
    // Serial.println("文件打开成功");

    // myMp3Status = myMp3CheckID3(file, mp3Info);
    // if(myMp3Status != MP3_STATUS_OK) 
    // {
    //     Serial.println("检查ID3标签失败");
    //     while(1) { delay(1000); }
    // }
    // Serial.println("检查ID3标签成功");
    // Serial.printf("歌曲标题: %s\n", mp3Info.title.c_str());
    // Serial.printf("艺术家: %s\n", mp3Info.artist.c_str());

    audioBlockQueue = xQueueCreate(10, sizeof(AudioBlock)); // 创建一个队列来存储AudioBlock
    mp3EventGroup = xEventGroupCreate(); // 创建一个事件组来同步任务
    xTaskCreatePinnedToCore(mp3change, "MP3ChangeTask", 4096, NULL, 12, &mp3ChangeTaskHandle, 0); // 创建切歌任务
    xTaskCreatePinnedToCore(mp3decoder, "MP3DecoderTask", 4096, NULL, 10, &mp3DecoderTaskHandle, 0); // 创建解码任务
    xTaskCreatePinnedToCore(mp3player, "MP3PlayerTask", 4096, NULL, 10, &mp3PlayerTaskHandle, 1); // 创建播放任务
    xTaskCreatePinnedToCore(ajDectect_Task, "AJDetectTask", 2048, NULL, 12, &ajDetectTaskHandle, 1); // 创建按键检测任务
    xTaskCreatePinnedToCore(IRReceive_Task, "IRReceiveTask", 4096, NULL, 12, &irReceiveTaskHandle, 1); // 创建红外接收任务
}

void loop() {
    // put your main code here, to run repeatedly:
    // myMp3Status = myMp3Decode(file, audioBlock);
    // if(myMp3Status == MP3_STATUS_ERROR) 
    // {
    //     Serial.println("MP3解码失败");
    //     while(1) { delay(1000); }
    // }
    // else if(myMp3Status == MP3_STATUS_DONE) 
    // {
    //     Serial.println("MP3播放完成");
    //     while(1) { delay(1000); }
    // }
   
    // if(myMp3Status != MP3_STATUS_CONTINUE)
    // {
    //     myMp3Play(audioBlock);
    // }
}

// put function definitions here:
void mp3change(void* parameter) 
{
    xEventGroupSetBits(mp3EventGroup, 0x01); // 触发切歌事件，开始播放第一首歌

    // 切歌任务的实现
    while(1) 
    {
        // 等待切歌事件
        xEventGroupWaitBits(mp3EventGroup, 0x01, pdTRUE, pdTRUE, portMAX_DELAY);
        Serial.println(">>> 收到切歌事件，准备切换歌曲...");

        while(uxQueueMessagesWaiting(audioBlockQueue) > 0); // 等待播放队列清空，确保当前歌曲播放完毕
        i2s_zero_dma_buffer(I2S_NUM_0); // 清空I2S DMA缓冲区，停止播放

        if(currentSongIndex >= mp3FileCount) 
        {
            currentSongIndex = 0; // 循环播放
        }
        file.open(mp3FileName[currentSongIndex].c_str(), O_READ);
        currentSongIndex++; // 准备下一首歌的索引
        if (!file) {
            Serial.printf("无法打开文件: %s\n", mp3FileName[currentSongIndex].c_str());
            xEventGroupSetBits(mp3EventGroup, 0x01); // 触发切歌事件，尝试切换到下一首歌
            Serial.println(">>> 切歌失败，继续尝试下一首歌...");
            vTaskDelay(1000); // 适当的延迟，避免任务占用过多CPU时间
            continue; // 如果无法打开文件，继续等待下一次切歌事件
        }
        myMp3Status = myMp3CheckID3(file, mp3Info); // 解析ID3标签，获取歌曲信息
        if(myMp3Status != MP3_STATUS_OK) 
        {
            Serial.println("解析ID3标签失败");
            file.close();
            xEventGroupSetBits(mp3EventGroup, 0x01); // 触发切歌事件，尝试切换到下一首歌
            Serial.println(">>> 切歌失败，继续尝试下一首歌...");
            vTaskDelay(500); // 适当的延迟，避免任务占用过多CPU时间
            continue;
        }
        Serial.println("检查ID3标签成功");
        Serial.printf("正在播放: %s - %s\n", mp3Info.title.c_str(), mp3Info.artist.c_str());

        xEventGroupSetBits(mp3EventGroup, 0x02); // 通知解码任务开始解码新歌曲

        Serial.println(">>> 切歌完成，等待下一次切歌事件...");
    }
}

void mp3decoder(void* parameter) 
{
    static AudioBlock audiodecodeBlock;

    while(1) 
    {
        xEventGroupWaitBits(mp3EventGroup, 0x02, pdFALSE, pdTRUE, portMAX_DELAY); // 等待切歌事件，开始解码

        myMp3Status = myMp3Decode(file, audiodecodeBlock);
        if(myMp3Status == MP3_STATUS_ERROR) 
        {
            Serial.println("MP3解码失败");
            vTaskDelay(1000); // 适当的延迟，避免任务占用过多CPU时间
            mymp3Reset(file); // 重置状态，准备下一次解码
            xEventGroupClearBits(mp3EventGroup, 0x02); // 清除解码事件，等待下一次切歌事件
            xEventGroupSetBits(mp3EventGroup, 0x01); // 触发切歌事件，尝试切换到下一首歌
            continue; // 继续等待下一次切歌事件
        }
        else if(myMp3Status == MP3_STATUS_DONE) 
        {
            Serial.println("MP3解码完成");
            mymp3Reset(file); // 重置状态，准备下一次解码
            xEventGroupClearBits(mp3EventGroup, 0x02); // 清除解码事件，等待下一次切歌事件
            xEventGroupSetBits(mp3EventGroup, 0x01); // 触发切歌事件，尝试切换到下一首歌
            vTaskDelay(200);
            continue; // 继续等待下一次切歌事件
        }
        else if(myMp3Status == MP3_STATUS_CONTINUE) 
        {
            Serial.println("MP3解码需要更多数据,继续解码...");
            vTaskDelay(10); // 适当的延迟，避免任务占用过多CPU时间
            continue; // 继续等待更多数据进行解码
        }
        else
        {
            xQueueSend(audioBlockQueue, &audiodecodeBlock, portMAX_DELAY); // 将解码后的音频块发送到队列
        }

        vTaskDelay(1); // 适当的延迟，避免任务占用过多CPU时间
    }
}

void mp3player(void* parameter) 
{
    static AudioBlock audioBlock;

    while(1) 
    {
        if (xQueueReceive(audioBlockQueue, &audioBlock, portMAX_DELAY) == pdPASS) 
        {
            myMp3Status = myMp3Play(audioBlock);
            if(myMp3Status != MP3_STATUS_OK) 
            {
                Serial.println("MP3播放失败");
                vTaskDelay(1000); // 适当的延迟，避免任务占用过多CPU时间
                continue; // 继续等待下一个音频块
            }
        }
    }
}

void aj_Interrupt(void) 
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR(ajDetectTaskHandle, 0, eNoAction, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken); // 如果需要切换到更高优先级的任务，立即切换
}

void ajDectect_Task(void* parameter) 
{
    uint8_t songRunning = 1; // 标志当前是否有歌曲正在播放

    while(1) 
    {
        xTaskNotifyWait(0, 0, NULL, portMAX_DELAY); // 等待中断通知
        vTaskDelay(20); // 简单的去抖动处理
        if(digitalRead(45) == HIGH) // 确认按键状态
        {
            Serial.println(">>> 按键事件触发");
            if(songRunning == 1)
            {
                vTaskSuspend(mp3DecoderTaskHandle); // 暂停解码任务，停止播放
                vTaskSuspend(mp3PlayerTaskHandle); // 暂停播放任务，停止播放
                songRunning = 0;
                Serial.println(">>> 暂停播放");
            }
            else
            {
                vTaskResume(mp3PlayerTaskHandle); // 恢复播放任务，继续播放
                vTaskResume(mp3DecoderTaskHandle); // 恢复解码任务，继续播放
                songRunning = 1;
                Serial.println(">>> 恢复播放");
            }
        }
        xTaskNotifyWait(0, 0, NULL, 0); // 清除通知，等待下一次中断通知
        
    }
}

void IRReceive_Task(void* parameter)
{
    uint8_t code;
    uint8_t songRunning = 1; // 标志当前是否有歌曲正在播放

    while(1) 
    {
        code = IRReceive_GetCode();
        if(code != 0)
        {
            Serial.print("Received NEC IR Code: 0x");
            Serial.print(code, HEX);
            Serial.println();

            switch(code)
            {
                case 0x5A: // 切歌
                    xEventGroupClearBits(mp3EventGroup, 0x02); // 清除解码事件，等待下一次切歌事件
                    mymp3Reset(file); // 重置状态，准备下一次解码
                    xEventGroupSetBits(mp3EventGroup, 0x01); // 触发切歌事件
                    Serial.println(">>> 切歌事件已触发");
                    break;
                case 0x08: // 上一首
                    currentSongIndex -= 2; // 上一首歌的索引（因为切歌事件会自动加1，所以这里减2）
                    if(currentSongIndex < 0)
                        currentSongIndex = mp3FileCount - 1; // 循环到最后一首歌
                    xEventGroupClearBits(mp3EventGroup, 0x02); // 清除解码事件，等待下一次切歌事件
                    mymp3Reset(file); // 重置状态，准备下一次解码
                    xEventGroupSetBits(mp3EventGroup, 0x01); // 触发切歌事件
                    Serial.println(">>> 上一首事件已触发");
                    break;
                case 0x1C: // 播放/暂停
                    if(songRunning == 1)
                    {
                        vTaskSuspend(mp3DecoderTaskHandle); // 暂停解码任务，停止播放
                        vTaskSuspend(mp3PlayerTaskHandle); // 暂停播放任务，停止播放
                        songRunning = 0;
                        Serial.println(">>> 暂停播放");
                    }
                    else
                    {
                        vTaskResume(mp3PlayerTaskHandle); // 恢复播放任务，继续播放
                        vTaskResume(mp3DecoderTaskHandle); // 恢复解码任务，继续播放
                        songRunning = 1;
                        Serial.println(">>> 恢复播放");
                    }
                    break;
                case 0x18: // 音量加
                    if(mp3_volume < 256) 
                    {
                        mp3_volume += 5; // 每次增加5
                        if(mp3_volume > 256) mp3_volume = 256; // 限制最大值为256
                        Serial.printf(">>> 音量增加, 当前音量: %d\n", mp3_volume);
                    }
                    else
                    {
                        Serial.println(">>> 音量已达到最大值");
                    }
                    break;
                case 0x52: // 音量减
                    if(mp3_volume > 0) 
                    {
                        mp3_volume -= 5; // 每次减少5
                        if(mp3_volume < 0) mp3_volume = 0; // 限制最小值为0
                        Serial.printf(">>> 音量减少, 当前音量: %d\n", mp3_volume);
                    }
                    else
                    {
                        Serial.println(">>> 音量已达到最小值");
                    }
                    break;
            }

        }
        vTaskDelay(100); // 避免任务占用过多CPU时间
    }
}
