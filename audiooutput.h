#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H

#ifdef __cplusplus  // 支持 C++ 和 C 混合编译
extern "C"
{
// 包含 SDL 和 FFmpeg 的头文件
#include "SDL.h"
#include "libswresample/swresample.h"
}
#endif

#include "avframequeue.h"  // 包含帧队列的定义

// 定义音频参数结构体
typedef struct AudioParams
{
    int freq;               // 采样率
    int channels;           // 声道数
    int64_t channel_layout; // 声道布局
    enum AVSampleFormat fmt;// 采样格式
    int frame_size;         // 每帧大小
} AudioParams;

// AudioOutput 类
class AudioOutput
{
public:
    // 构造函数和析构函数
    AudioOutput(const AudioParams &audio_params, AVFrameQueue *frame_queue);
    ~AudioOutput();

    // 初始化函数和反初始化函数
    int Init();
    int DeInit();

public:
    AudioParams src_tgt_;     // 解码后的音频参数
    AudioParams dst_tgt_;     // SDL 实际输出的音频参数
    AVFrameQueue *frame_queue_ = NULL; // 存放解码后的音频帧队列

    struct SwrContext *swr_ctx_ = NULL; // 音频重采样上下文

    uint8_t *audio_buf_ = NULL;       // 音频数据缓冲区
    uint8_t *audio_buf1_ = NULL;      // 重采样后的音频数据缓冲区（可能比audio_buf大，这里无论是否重采样，都先让audio_buf1指向音频数据缓冲区）
    uint32_t audio_buf_size = 0;      // 当前音频数据缓冲区的大小
    uint32_t audio_buf1_size = 0;     // 重采样后的音频数据的大小（可能比audio_buf大，这里无论是否重采样，都先让audio_buf1指向音频数据缓冲区）
    uint32_t audio_buf_index = 0;     // 当前音频数据缓冲区的偏移量
};

#endif // AUDIOOUTPUT_H
