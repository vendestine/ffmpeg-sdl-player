#ifndef DEMUXTHREAD_H
#define DEMUXTHREAD_H

#include "thread.h"           // 包含基类 Thread 的定义
#include "avpacketqueue.h"    // 包含 AVPacketQueue 的头文件
#ifdef __cplusplus
extern "C" {
#include "libavformat/avformat.h" // FFmpeg 格式上下文头文件
}
#endif

// DemuxThread 类，负责音视频解复用
class DemuxThread : public Thread
{
public:
    DemuxThread(AVPacketQueue *audio_queue, AVPacketQueue *video_queue); // 构造函数
    ~DemuxThread(); // 析构函数
    int Init(const char *url);   // 初始化解复用器
    int Start();                 // 启动解复用线程
    int Stop();                  // 停止解复用线程
    void Run();                  // 实际的解复用线程函数

private:
    char err2str[256] = {0}; // 存储错误信息的字符串
    std::string url_;        // 媒体文件的 URL

    AVPacketQueue *audio_queue_; // 指向音频队列的指针
    AVPacketQueue *video_queue_; // 指向视频队列的指针

    AVFormatContext *ifmt_ctx_; // FFmpeg 输入格式上下文
    int audio_index_ = -1;      // 音频流索引
    int video_index_ = -1;      // 视频流索引
};

#endif // DEMUXTHREAD_H
