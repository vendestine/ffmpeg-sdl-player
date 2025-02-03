#include "avframequeue.h"
#include "log.h" // 引入日志功能

// 构造函数
AVFrameQueue::AVFrameQueue()
{
    // 可以进行初始化操作
}

// 析构函数
AVFrameQueue::~AVFrameQueue()
{
    // 在析构时不需要特别的操作
}

// 中止队列操作，释放资源
void AVFrameQueue::Abort()
{
    release(); // 释放所有在队列中的 AVFrames
    queue_.Abort(); // 中止队列操作
}

// 将 AVFrame 推送到队列
int AVFrameQueue::Push(AVFrame *val)
{
    AVFrame *tmp_frame = av_frame_alloc(); // 分配一个新的 AVFrame
    av_frame_move_ref(tmp_frame, val); // 移动数据到 tmp_frame，避免深拷贝
    return queue_.Push(tmp_frame); // 推入队列并返回结果
}

// 从队列弹出 AVFrame
AVFrame *AVFrameQueue::Pop(const int timeout)
{
    AVFrame *tmp_frame = NULL; // 初始化临时指针
    int ret = queue_.Pop(tmp_frame, timeout); // 从队列中弹出 AVFrame
    if(ret < 0) { // 检查是否成功
        if(ret == -1)
            LogError("AVFrameQueue::Pop failed"); // 记录错误日志
    }
    return tmp_frame; // 返回弹出的 AVFrame
}

// 返回队头的 AVFrame
AVFrame *AVFrameQueue::Front()
{
    AVFrame *tmp_frame = NULL; // 初始化临时指针
    int ret = queue_.Front(tmp_frame); // 获取队头 AVFrame 而不弹出
    if(ret < 0) { // 检查是否成功
        if(ret == -1)
            LogError("AVFrameQueue::Front failed"); // 记录错误日志
    }
    return tmp_frame; // 返回队头指针
}

// 获取队列大小
int AVFrameQueue::Size()
{
    return queue_.Size(); // 返回内部队列的大小
}

// 释放队列中的所有 AVFrame
void AVFrameQueue::release()
{
    while (true) {
        AVFrame *frame = NULL; // 初始化 AVFrame 指针
        int ret = queue_.Pop(frame, 1); // 尝试弹出队列
        if(ret < 0) { // 检查结果
            break; // 如果出错或队列为空，退出循环
        } else {
            av_frame_free(&frame); // 释放当前的 AVFrame
            continue; // 继续处理下一个
        }
    }
}
