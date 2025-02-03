#include "demuxthread.h"
#include "log.h"
DemuxThread::DemuxThread()
{
    LogInfo("DemuxThread");   // 日志记录构造函数调用
}

DemuxThread::~DemuxThread()
{
    LogInfo("~DemuxThread");  // 日志记录析构函数调用
    if(thread_) {  // 在析构之前确保线程被停止
        Stop();
    }
}

int DemuxThread::Init(const char *url)
{
    LogInfo("url:%s", url); // 日志记录传入的 URL
    int ret = 0;
    url_ = url; // 保存 URL

    // 解复用 1. 分配输入格式（解复用器）上下文
    ifmt_ctx_ = avformat_alloc_context();

    // 解复用 2. 根据url打开本地文件
    ret = avformat_open_input(&ifmt_ctx_, url_.c_str(), NULL, NULL);
    if (ret < 0) {
        av_strerror(ret, err2str, sizeof(err2str));
        LogError("avformat_open_input failed, ret:%d, err2str:%s", ret, err2str);
        return -1; // 返回错误
    }

    // 解复用 3. 读取媒体的部分数据包以获取码流信息
    ret = avformat_find_stream_info(ifmt_ctx_, NULL);
    if (ret < 0) {
        av_strerror(ret, err2str, sizeof(err2str));
        LogError("avformat_find_stream_info failed, ret:%d, err2str:%s", ret, err2str);
        return -1; // 返回错误
    }

    // 打印格式信息到日志
    av_dump_format(ifmt_ctx_, 0, url_.c_str(), 0);

    // 解复用 4. 找到音频和视频流的最佳索引
    audio_index_ = av_find_best_stream(ifmt_ctx_, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    video_index_ = av_find_best_stream(ifmt_ctx_, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    LogInfo("audio_index_:%d, video_index_:%d", audio_index_, video_index_);

    // 检查是否找到音频或视频流
    if (audio_index_ < 0 || video_index_ < 0) {
        LogError("no audio or no video");
        return -1; // 返回错误
    }

    LogInfo("Init leave"); // 日志记录初始化结束
    return 0; // 成功返回
}

int DemuxThread::Start()
{
    // 启动一个新的线程来运行 Run() 方法
    thread_ = new std::thread(&DemuxThread::Run, this);
    if (!thread_) {
        LogError("new std::thread(&DemuxThread::Run, this) failed");
        return -1;
    }
    return 0;
}

int DemuxThread::Stop()
{
    // 停止线程并关闭输入格式上下文
    Thread::Stop(); // 调用基类的 Stop 函数

    // 解复用 6. 关闭解复用器，释放资源
    avformat_close_input(&ifmt_ctx_);
}

void DemuxThread::Run()
{
    // 在子线程中执行解复用的主要逻辑，读取视频和音频帧
    LogInfo("Run into"); // 日志记录线程开始执行

    int ret = 0;
    AVPacket pkt; // 创建 AVPacket 用于存储读取到的每个数据包

    // 终止之前，循环读取AVPacket
    while (abort_ != 1) {
        // 解复用 5. 从输入格式上下文（解复用器上下文）中读取数据包
        ret = av_read_frame(ifmt_ctx_, &pkt);
        if (ret < 0) {
            av_strerror(ret, err2str, sizeof(err2str));
            LogError("av_read_frame failed, ret:%d, err2str:%s", ret, err2str);
            break; // 如果读取失败，则跳出循环
        }
        // 根据流索引判断是音频包还是视频包
        if (pkt.stream_index == audio_index_) {
            LogInfo("audio pkt"); // 日志记录音频包
        } else if (pkt.stream_index == video_index_) {
            LogInfo("video pkt"); // 日志记录视频包
        }

        av_packet_unref(&pkt); // 释放数据包内存，准备读取下一个包
    }

    LogInfo("Run finish"); // 日志记录线程运行结束
}
