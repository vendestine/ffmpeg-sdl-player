#include <iostream>          // 包含输入输出流库
#include "log.h"            // 包含日志功能的头文件
#include "demuxthread.h"    // 包含 DemuxThread 类的头文件
using namespace std;        // 使用标准命名空间以简化代码

int main(int argc, char *argv[]) {
    LogInit(); // 初始化日志功能

    int ret = 0; // 定义返回值，初始为 0

    // 创建音频和视频队列
    AVPacketQueue audio_queue; // 音频数据包队列
    AVPacketQueue video_queue; // 视频数据包队列

    // 1. 解复用（Demuxing）
    DemuxThread *demux_thread = new DemuxThread(&audio_queue, &video_queue); // 创建解复用线程对象，传入音频和视频队列的指针
    ret = demux_thread->Init("time.mp4"); // 初始化解复用器，传入待处理的媒体文件
    // 检查初始化是否成功
    if(ret < 0) {
        LogError("demux_thread.Init failed"); // 记录初始化失败日志
        return -1;
    }

    // 启动解复用线程
    ret = demux_thread->Start(); // 开始解复用操作
    if(ret < 0) { // 检查启动是否成功
        LogError("demux_thread.Start() failed"); // 记录启动失败日志
        return -1;
    }

    // 休眠2秒，让解复用线程有时间运行
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    demux_thread->Stop(); // 停止解复用线程
    delete demux_thread; // 释放解复用线程对象的内存

    LogInfo("main finish");

    return 0;
}
