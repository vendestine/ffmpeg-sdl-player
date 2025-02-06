#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H

#ifdef __cplusplus
extern "C" {
// 包含 SDL 和 FFmpeg 的头文件
#include "SDL.h"
#include "libswresample/swresample.h"
}
#endif

#include "avsync.h"          // 音频同步模块
#include "avframequeue.h"    // 帧队列模块，存储解码后的音频帧

// 音频参数结构体
typedef struct AudioParams {
    int freq;               // 采样率
    int channels;           // 声道数
    int64_t channel_layout; // 声道布局
    enum AVSampleFormat fmt;// 采样格式
    int frame_size;         // 每帧大小
} AudioParams;

// AudioOutput 类
class AudioOutput {
public:
    // 构造函数和析构函数
    AudioOutput(AVSync *avsync, AVRational time_base, const AudioParams &audio_params, AVFrameQueue *frame_queue);
    ~AudioOutput();

    // 初始化函数和反初始化函数
    int Init();  // 初始化音频设备和相关资源
    int DeInit(); // 释放资源

public:
    int64_t pts_ = AV_NOPTS_VALUE;  // 当前音频帧的显示时间戳
    AudioParams src_tgt_;           // 源音频参数（解码后的音频格式）
    AudioParams dst_tgt_;           // 目标音频参数（SDL实际输出的音频格式）
    AVFrameQueue *frame_queue_ = NULL; // 存放解码后的音频帧队列

    struct SwrContext *swr_ctx_ = NULL; // FFmpeg 音频重采样上下文

    uint8_t *audio_buf_ = NULL;     // 当前正在使用的音频缓冲区
    uint8_t *audio_buf1_ = NULL;    // 重采样后的音频缓冲区（这里即使不重采样，也先使用audio_buf1分配缓冲区）
    uint32_t audio_buf_size = 0;    // 当前音频缓冲区的大小
    uint32_t audio_buf1_size = 0;   // 重采样后的音频缓冲区的大小（这里即使不重采样，也先使用audio_buf1分配缓冲区）
    uint32_t audio_buf_index = 0;   // 当前音频缓冲区的偏移量

    AVSync *avsync_;                // 音视频同步模块
    AVRational time_base_;          // 时间基准
};

#endif // AUDIOOUTPUT_H
