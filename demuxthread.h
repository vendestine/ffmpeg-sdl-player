#ifndef DEMUXTHREAD_H
#define DEMUXTHREAD_H

#include "thread.h"

#ifdef __cplusplus  ///
extern "C"
{
// 包含ffmpeg头文件
#include "libavutil/avutil.h"
#include "libavformat/avformat.h"
}
#endif


class DemuxThread : public Thread
{
public:
    DemuxThread();               // 构造函数
    ~DemuxThread();              // 析构函数
    int Init(const char *url);   // 初始化解复用器
    int Start();                 // 启动解复用线程
    int Stop();                  // 停止解复用线程
    void Run();                  // 实际的解复用线程函数
private:
    char err2str[256] = {0};     // 用于存储错误信息的字符串
    std::string url_;            // 媒体文件的 URL

    AVFormatContext *ifmt_ctx_ = NULL; // FFmpeg 格式上下文，存储媒体文件信息
    int audio_index_ = -1;      // 音频流索引
    int video_index_ = -1;      // 视频流索引
};

#endif // DEMUXTHREAD_H
