#ifndef AVPACKETQUEUE_H
#define AVPACKETQUEUE_H

#include "queue.h" // 包含自定义的线程安全队列类

#ifdef __cplusplus
extern "C" {
#include "libavcodec/avcodec.h" // 包含 FFmpeg 的 AVPacket 相关头文件
}
#endif

class AVPacketQueue {
public:
    AVPacketQueue();    // 构造函数
    ~AVPacketQueue();   // 析构函数
    void Abort();       // 中止队列操作
    int Size();                         // 获取队列大小
    int Push(AVPacket *val);            // 向队列推送 AVPacket
    AVPacket *Pop(const int timeout);   // 从队列弹出 AVPacket
    // 没有front方法，AVPacket队列不需要

private:
    void release();                     // 释放队列中的所有 AVPacket
    Queue<AVPacket *> queue_;           // 线程安全的 AVPacket 队列
};

#endif // AVPACKETQUEUE_H
