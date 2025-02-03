#include "avpacketqueue.h"
#include "log.h"

// 构造函数
AVPacketQueue::AVPacketQueue() {
    // 这里可以进行任何必要的初始化
}

// 析构函数
AVPacketQueue::~AVPacketQueue() {
    // 在析构时不需要特别的操作
}

// 中止队列操作，释放资源
void AVPacketQueue::Abort() {
    release(); // 释放所有在队列中的 AVPackets
    queue_.Abort(); // 中止队列操作（设置 abort 标志并通知所有等待线程）
}

// 获取队列大小
int AVPacketQueue::Size() {
    return queue_.Size(); // 直接返回内部队列的大小
}

// 推送 AVPacket 到队列
int AVPacketQueue::Push(AVPacket *val) {
    AVPacket *tmp_pkt = av_packet_alloc(); // 分配一个新的 AVPacket
    av_packet_move_ref(tmp_pkt, val); // 将数据移动到 tmp_pkt，避免深拷贝
    return queue_.Push(tmp_pkt); // 推入队列并返回结果
}

// 从队列弹出 AVPacket
AVPacket *AVPacketQueue::Pop(const int timeout) {
    AVPacket *tmp_pkt = NULL; // 初始化临时指针
    int ret = queue_.Pop(tmp_pkt, timeout); // 从队列中弹出数据
    if (ret < 0) { // 如果出错
        if (ret == -1) {
            LogError("AVPacketQueue::Pop failed"); // 记录错误日志
        }
    }
    return tmp_pkt; // 返回弹出的 AVPacket
}

// 释放队列中的所有 AVPacket
void AVPacketQueue::release() {
    while (true) {
        AVPacket *packet = NULL; // 初始化指针
        int ret = queue_.Pop(packet, 1); // 尝试弹出队列中的 AVPacket
        if (ret < 0) { // 如果出错或队列为空
            break; // 退出循环
        } else {
            av_packet_free(&packet); // 释放当前的 AVPacket
            continue; // 继续释放下一个
        }
    }
}
