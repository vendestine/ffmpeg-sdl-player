#ifndef DECODETHREAD_H
#define DECODETHREAD_H

#include "thread.h"         // 包含基类 Thread 的定义
#include "avpacketqueue.h"  // 包含 AVPacketQueue 的头文件
#include "avframequeue.h"   // 包含 AVFrameQueue 的头文件

// 解码线程类，负责音视频解码
class DecodeThread : public Thread {
public:
    DecodeThread(AVPacketQueue *packet_queue, AVFrameQueue *frame_queue); // 构造函数
    ~DecodeThread(); // 析构函数
    int Init(AVCodecParameters *par); // 初始化解码器
    int Start(); // 启动解码线程
    int Stop(); // 停止解码线程
    void Run(); // 实际的解码线程函数

private:
    char err2str[256] = {0}; // 存储错误信息的字符串
    AVCodecContext *codec_ctx_ = NULL; // FFmpeg 编解码上下文
    AVPacketQueue *packet_queue_ = NULL; // 指向数据包队列的指针
    AVFrameQueue *frame_queue_ = NULL; // 指向帧队列的指针
};

#endif // DECODETHREAD_H
