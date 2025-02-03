#include <iostream>          // 包含输入输出流库
#include "log.h"            // 包含日志功能的头文件
#include "demuxthread.h"    // 包含 DemuxThread 类的头文件
using namespace std;

int main(int argc, char *argv[]) {
    LogInit(); // 初始化日志功能

    // 1. 解复用（Demuxing）
    DemuxThread demux_thread; // 创建 DemuxThread 对象
    int ret = 0;
    ret = demux_thread.Init("time.mp4"); // 初始化解复用器，传入媒体文件路径
    // 检查初始化是否成功
    if(ret < 0) {
        LogError("demux_thread.Init failed"); // 记录错误日志
        return -1;
    }

    // 启动解复用线程
    ret = demux_thread.Start();
    if(ret < 0) { // 检查启动是否成功
        LogError("demux_thread.Start() failed"); // 记录错误日志
        return -1;
    }

    // 休眠2秒，允许解复用线程运行
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    demux_thread.Stop(); // 停止解复用线程

    return 0; // 正常退出程序
}
