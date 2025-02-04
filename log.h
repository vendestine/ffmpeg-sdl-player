#ifndef LOG_H
#define LOG_H

#include <string>
#include <iostream>

void LogInit();
void Serialize(const char* fmt, ...);

// 获取文件名（不带路径）
#define __FILENAME__ (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1) : __FILE__)

// 创建日志前缀
#define makePrefix(fmt) std::string(__FUNCTION__).append("() - ").append(fmt).c_str()
#define makeClassPrefix(fmt) std::string(__PRETTY_FUNCTION__).append(" - ").append(fmt).c_str() // 使用 __PRETTY_FUNCTION__ 捕获完整信息

// 日志宏
#define LogDebug(fmt, ...)    Serialize(makePrefix(fmt), ##__VA_ARGS__)
#define LogInfo(fmt, ...)     Serialize(makeClassPrefix(fmt), ##__VA_ARGS__)
#define LogNotice(fmt, ...)   Serialize(makePrefix(fmt), ##__VA_ARGS__)
#define LogError(fmt, ...)    Serialize(makePrefix(fmt), ##__VA_ARGS__)

// 修改后的成员函数调用
#define LogClassInfo(fmt, ...)    Serialize(makeClassPrefix(fmt), ##__VA_ARGS__)

// 函数入口和出口日志
#define FunEntry(...) LogDebug(" Entry... " ##__VA_ARGS__)
#define FunExit(...)  LogDebug(" Exit... " ##__VA_ARGS__)

#endif // LOG_H
