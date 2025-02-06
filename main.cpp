#include <iostream>
#include "log.h"           // 日志功能的定义
#include "demuxthread.h"   // 解复用线程的定义
#include "avframequeue.h"  // 帧队列的定义
#include "decodethread.h"  // 解码线程的定义
#include "audiooutput.h"
using namespace std;

// 主程序
#undef main
int main(int argc, char *argv[]) {
    LogInit(); // 初始化日志功能

    int ret = 0; // 定义返回值，初始为 0

    // 创建音频和视频 包队列
    AVPacketQueue audio_packet_queue; // 用于存储音频数据包的队列
    AVPacketQueue video_packet_queue; // 用于存储视频数据包的队列

    // 创建音频和视频 帧队列
    AVFrameQueue audio_frame_queue;  // 用于存储解码后的音频帧的队列
    AVFrameQueue video_frame_queue;  // 用于存储解码后的视频帧的队列

    // 1. 解复用（Demuxing）
    // 创建解复用线程对象，传入音频和视频队列的指针
    DemuxThread *demux_thread = new DemuxThread(&audio_packet_queue, &video_packet_queue);
    // 初始化解复用器，传入待处理的媒体文件
    ret = demux_thread->Init("time.mp4");
    if(ret < 0) {
        LogError("demux_thread.Init failed");
        return -1;
    }
    // 启动解复用线程
    ret = demux_thread->Start();
    if(ret < 0) {
        LogError("demux_thread.Start() failed");
        return -1;
    }

    // 2. 解码（Decoding)
    /*
     * 注意解复用线程启动后，这里就创建了解码线程，所以有可能解复用线程数据还没有解复用完毕的时候，
     * 解码器就已经初始化完，也开始解码了
     */
    // 2.1 创建音频解码线程
    DecodeThread *audio_decode_thread = new DecodeThread(&audio_packet_queue, &audio_frame_queue);
    // 初始化音频解码器
    ret = audio_decode_thread->Init(demux_thread->AudioCodecParameters());
    if(ret < 0) {
        LogError("audio_decode_thread->Init() failed");
        return -1;
    }
    // 启动音频解码线程
    ret = audio_decode_thread->Start();
    if(ret < 0) {
        LogError("audio_decode_thread->Start() failed");
        return -1;
    }

    // 2.2 创建视频解码线程
    DecodeThread *video_decode_thread = new DecodeThread(&video_packet_queue, &video_frame_queue);
    // 初始化视频解码器
    ret = video_decode_thread->Init(demux_thread->VideoCodecParameters());
    if(ret < 0) {
        LogError("video_decode_thread->Init() failed");
        return -1;
    }
    // 启动视频解码线程
    ret = video_decode_thread->Start();
    if(ret < 0) {
        LogError("video_decode_thread->Start() failed");
        return -1;
    }

    // 3. 音频输出
    AudioParams audio_params = {0}; // 初始化音频参数
    memset(&audio_params, 0, sizeof(AudioParams)); // 将音频参数结构清零
    audio_params.channels = demux_thread->AudioCodecParameters()->channels; // 设置声道数
    audio_params.channel_layout = demux_thread->AudioCodecParameters()->channel_layout; // 设置声道布局
    audio_params.fmt = (enum AVSampleFormat) demux_thread->AudioCodecParameters()->format; // 设置样本格式
    audio_params.freq = demux_thread->AudioCodecParameters()->sample_rate; // 设置采样率
    audio_params.frame_size = demux_thread->AudioCodecParameters()->frame_size; // 设置帧大小
    AudioOutput *audio_output = new AudioOutput(audio_params, &audio_frame_queue); // 创建音频输出对象
    ret = audio_output->Init(); // 初始化音频输出
    if (ret < 0) {
        LogError("audio_output->Init() failed"); // 初始化失败，记录错误信息
        return -1; // 返回错误
    }

    // 休眠2秒
    std::this_thread::sleep_for(std::chrono::milliseconds(120* 1000));

    // 停止解复用线程，并释放资源
    LogInfo("demux_thread->Stop");
    demux_thread->Stop(); // 停止解复用线程
    LogInfo("delete demux_thread");
    delete demux_thread; // 释放解复用线程对象的内存

    // 停止音频解码线程，并释放资源
    LogInfo("audio_decode_thread->Stop()");
    audio_decode_thread->Stop(); // 停止音频解码线程
    LogInfo("delete audio_decode_thread");
    delete audio_decode_thread; // 释放音频解码线程对象的内存

    // 停止视频解码线程，并释放资源
    LogInfo("video_decode_thread->Stop()");
    video_decode_thread->Stop(); // 停止视频解码线程
    LogInfo("delete video_decode_thread");
    delete video_decode_thread; // 释放视频解码线程对象的内存

    LogInfo("main finish");
    return 0;
}
