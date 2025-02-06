#ifndef AVSYNC_H
#define AVSYNC_H

#include <chrono>  // 时间库，用于高精度计时
#include <ctime>   // C 标准时间库
#include <math.h>  // 数学函数库
using namespace std::chrono;  // 使用 std::chrono 命名空间

class AVSync {
public:
    AVSync() {
        // 构造函数，初始化时不做任何操作
    }

    // 初始化时钟，将时钟设置为无效值
    void InitClock() {
        SetClock(NAN);  // NAN 表示无效值
    }

    // 设置时钟：根据给定的 PTS（显示时间戳）和时间设置时钟
    void SetClockAt(double pts, double time) {
        pts_ = pts;                // 记录当前的显示时间戳
        pts_drift_ = pts_ - time;  // 计算时间漂移量（PTS 与当前时间的差值）
    }

    // 获取当前时钟值
    double GetClock() {
        double time = GetMicroseconds() / 1000000.0;  // 获取当前的时间，单位转换为秒
        return pts_drift_ + time;  // 返回时钟值（漂移量 + 当前时间）
    }

    // 设置时钟：根据给定的 PTS 更新时钟
    void SetClock(double pts) {
        double time = GetMicroseconds() / 1000000.0;   // 获取当前的时间，单位转换为秒
        SetClockAt(pts, time);  // 调用 SetClockAt 更新时钟
    }

    // 获取当前时间的微秒值
    time_t GetMicroseconds() {
        system_clock::time_point time_point_new = system_clock::now();  // 获取当前时间点
        system_clock::duration duration = time_point_new.time_since_epoch();  // 计算从时间起点到当前时间点的时长
        time_t us = duration_cast<microseconds>(duration).count();  // 将时长转换为微秒
        return us;
    }

    double pts_ = 0;        // 显示时间戳（PTS）
    double pts_drift_ = 0;  // 时间漂移量（PTS 与当前时间的差值），用于同步调整
};

#endif // AVSYNC_H
