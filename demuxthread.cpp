#include "demuxthread.h"
#include "log.h"

// 构造函数，初始化音视频队列
DemuxThread::DemuxThread(AVPacketQueue *audio_queue, AVPacketQueue *video_queue)
    : audio_queue_(audio_queue), video_queue_(video_queue) 
{
    LogInfo("DemuxThread"); // 日志记录构造函数调用
}

// 析构函数，确保线程被正确停止
DemuxThread::~DemuxThread() 
{
    LogInfo("~DemuxThread"); // 日志记录析构函数调用
    if (thread_) { // 在析构之前确保线程被停止
        Stop();
    }
}

// 初始化解复用器
int DemuxThread::Init(const char *url) 
{
    LogInfo("url:%s", url); // 记录传入的 URL
    int ret = 0;
    url_ = url; // 保存 URL

    // 1. 分配输入格式（解复用器）上下文
    ifmt_ctx_ = avformat_alloc_context();

    // 2. 根据 URL 打开本地文件
    ret = avformat_open_input(&ifmt_ctx_, url_.c_str(), NULL, NULL);
    if (ret < 0) {
        av_strerror(ret, err2str, sizeof(err2str)); // 获取错误描述
        LogError("avformat_open_input failed, ret:%d, err2str:%s", ret, err2str); // 记录错误
        return -1; // 返回错误
    }

    // 3. 读取媒体的部分数据包以获取码流信息
    ret = avformat_find_stream_info(ifmt_ctx_, NULL);
    if (ret < 0) {
        av_strerror(ret, err2str, sizeof(err2str)); // 获取错误描述
        LogError("avformat_find_stream_info failed, ret:%d, err2str:%s", ret, err2str); // 记录错误
        return -1; // 返回错误
    }

    // 打印格式信息到日志
    av_dump_format(ifmt_ctx_, 0, url_.c_str(), 0);

    // 4. 找到音频和视频流的最佳索引
    audio_index_ = av_find_best_stream(ifmt_ctx_, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    video_index_ = av_find_best_stream(ifmt_ctx_, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    LogInfo("audio_index_:%d, video_index_:%d", audio_index_, video_index_);

    // 检查是否找到音频或视频流
    if (audio_index_ < 0 || video_index_ < 0) {
        LogError("no audio or no video"); // 记录没有找到音视频流的错误
        return -1; // 返回错误
    }

    LogInfo("Init leave"); // 日志记录初始化结束
    return 0; // 成功返回
}

// 启动解复用线程
int DemuxThread::Start() 
{
    // 启动新的线程来执行 Run() 方法
    thread_ = new std::thread(&DemuxThread::Run, this);
    if (!thread_) {
        LogError("new std::thread(&DemuxThread::Run, this) failed"); // 记录创建线程失败
        return -1; // 返回错误
    }
    return 0; // 成功
}

// 停止解复用线程
int DemuxThread::Stop() 
{
    // 停止线程并关闭输入格式上下文
    Thread::Stop(); // 调用基类的 Stop 函数

    // 6. 关闭解复用器，释放资源
    avformat_close_input(&ifmt_ctx_);
}

// 解复用主要逻辑
void DemuxThread::Run() 
{
    LogInfo("Run into"); // 日志记录线程开始执行

    int ret = 0;
    AVPacket pkt; // 创建 AVPacket 用于存储读取到的每个数据包

    // 终止之前，循环读取 AVPacket
    while (abort_ != 1) {
        // 限制音频和视频队列的大小，防止内存溢出
        if(audio_queue_->Size() > 100 || video_queue_->Size() > 100) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 如果队列已满，休眠
            continue; // 继续下次循环
        }

        // 5. 从输入格式上下文（解复用器上下文）中读取数据包
        ret = av_read_frame(ifmt_ctx_, &pkt);
        if (ret < 0) {
            av_strerror(ret, err2str, sizeof(err2str)); // 获取错误描述
            LogError("av_read_frame failed, ret:%d, err2str:%s", ret, err2str); // 记录读取失败的错误
            break; // 如果读取失败，则跳出循环
        }

        // 根据流索引判断是音频包还是视频包
        if (pkt.stream_index == audio_index_) {
            ret = audio_queue_->Push(&pkt); // 推送音频包到音频队列
            LogInfo("audio pkt queue size:%d", audio_queue_->Size()); // 打印音频队列大小
			// av_packet_unref(&pkt); // 释放不需要的包
        } else if (pkt.stream_index == video_index_) {
            ret = video_queue_->Push(&pkt); // 推送视频包到视频队列
            LogInfo("video pkt queue size:%d", video_queue_->Size()); // 打印视频队列大小
            // av_packet_unref(&pkt); // 释放不需要的包
        } else {
            av_packet_unref(&pkt); // 释放不需要的包
        }
    }

    LogInfo("Run finish"); // 日志记录线程运行结束
}

// 获取音频流的编解码参数
AVCodecParameters *DemuxThread::AudioCodecParameters() 
{
    if (audio_index_ != -1) {
        return ifmt_ctx_->streams[audio_index_]->codecpar; // 返回音频流的编解码参数
    } else {
        return NULL; // 如果没有音频流，返回 NULL
    }
}

// 获取视频流的编解码参数
AVCodecParameters *DemuxThread::VideoCodecParameters() 
{
    if (video_index_ != -1) {
        return ifmt_ctx_->streams[video_index_]->codecpar; // 返回视频流的编解码参数
    } else {
        return NULL; // 如果没有视频流，返回 NULL
    }
}

AVRational DemuxThread::AudioStreamTimebase()
{
    if (audio_index_ != -1) {
        return ifmt_ctx_->streams[audio_index_]->time_base; // 返回音频流的时间基准
    } else {
        return AVRational{0, 0}; // 如果没有音频流，返回(0, 0)
    }
}

AVRational DemuxThread::VideoStreamTimebase()
{
    if (video_index_ != -1) {
        return ifmt_ctx_->streams[video_index_]->time_base; // 返回视频流的时间基准
    } else {
        return AVRational{0, 0}; // 如果没有视频流，返回(0, 0)
    }
}
