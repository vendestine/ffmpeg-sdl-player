#ifndef AVFRAMEQUEUE_H
#define AVFRAMEQUEUE_H

#include "queue.h" // 引入自定义的线程安全队列

#ifdef __cplusplus
extern "C" {
#include "libavcodec/avcodec.h" // 引入 FFmpeg 编解码库中的 AVFrame 定义
}
#endif

// AVFrameQueue 类用于管理 AVFrame 的队列
class AVFrameQueue
{
public:
    AVFrameQueue();                  // 构造函数
    ~AVFrameQueue();                 // 析构函数
    void Abort();                    // 停止队列操作
    int Push(AVFrame *val);          // 将 AVFrame 添加到队列
    AVFrame *Pop(const int timeout); // 从队列中弹出 AVFrame
    AVFrame *Front();                // 返回队头 AVFrame
    int Size();                      // 返回队列大小
private:
    void release();             // 释放队列中的所有 AVFrame
    Queue<AVFrame *> queue_;    // 内部使用自定义的线程安全队列
};

#endif // AVFRAMEQUEUE_H
