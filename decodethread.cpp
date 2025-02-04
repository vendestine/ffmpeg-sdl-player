#include "decodethread.h"
#include "log.h"

// 构造函数，初始化数据包和帧队列
DecodeThread::DecodeThread(AVPacketQueue *packet_queue, AVFrameQueue *frame_queue)
    : packet_queue_(packet_queue), frame_queue_(frame_queue) {
    // 不需要特别的初始化逻辑
}

// 析构函数，停止线程并关闭解码器
DecodeThread::~DecodeThread() {
    if (thread_) {
        Stop(); // 确保线程被停止
    }

    // 6. 关闭解码器和释放解码器上下文
    if (codec_ctx_) {
        avcodec_free_context(&codec_ctx_); // 释放解码上下文
    }
}

// 初始化解码器
int DecodeThread::Init(AVCodecParameters *par) {
    if (!par) { // 检查参数是否为空
        LogError("Init par is null");
        return -1; // 返回错误
    }

    // 1. 分配AVCodecContex（编解码器上下文）
    codec_ctx_ = avcodec_alloc_context3(NULL);

    // 2. 将码流中的编解码信息拷贝到AVCodecContex（编解码器上下文）
    int ret = avcodec_parameters_to_context(codec_ctx_, par);
    if (ret < 0) {
        av_strerror(ret, err2str, sizeof(err2str));
        LogError("avcodec_parameters_to_context failed, ret:%d, err2str:%s", ret, err2str);
        return -1; // 返回错误
    }

    // 3. 根据编解码信息查找相应的解码器
    AVCodec *codec = avcodec_find_decoder(codec_ctx_->codec_id);
    if (!codec) {
        LogError("avcodec_find_decoder failed");
        return -1; // 返回错误
    }

    // 4. 打开编解码器并关联到AVCodecContex（编解码器上下文）
    ret = avcodec_open2(codec_ctx_, codec, NULL);
    if (ret < 0) {
        av_strerror(ret, err2str, sizeof(err2str));
        LogError("avcodec_open2 failed, ret:%d, err2str:%s", ret, err2str);
        return -1; // 返回错误
    }
    LogInfo("Init finish"); // 日志记录初始化完成
    return 0; // 成功返回
}

// 启动解码线程
int DecodeThread::Start() {
    thread_ = new std::thread(&DecodeThread::Run, this); // 启动新线程执行 Run 函数
    if (!thread_) {
        LogError("new std::thread(&DecodeThread::Run, this) failed"); // 记录错误
        return -1; // 返回错误
    }
    return 0; // 成功返回
}

// 停止解码线程
int DecodeThread::Stop() {
    Thread::Stop(); // 调用基类的 Stop 方法
}

// 解码主循环
void DecodeThread::Run() {
    AVFrame *frame = av_frame_alloc(); // 分配解码后的帧

    LogInfo("Run into");
    while (abort_ != 1) { // 循环直到 abort 被设置
        // 如果帧队列大小大于 10，休眠 10 毫秒
        if (frame_queue_->Size() > 10) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 线程休眠
            continue; // 继续下一轮循环
        }

        // 从包队列中弹出 AVPacket
        AVPacket *pkt = packet_queue_->Pop(10); // 设置超时为 10 毫秒
        if (pkt) { // 如果成功获取到包

            // 5.1 向解码器发送数据包
            int ret = avcodec_send_packet(codec_ctx_, pkt);
            av_packet_free(&pkt); // 释放用完的包

            if (ret < 0) { // 发送包失败处理
                av_strerror(ret, err2str, sizeof(err2str));
                LogError("avcodec_send_packet failed, ret:%d, err2str:%s", ret, err2str);
                break; // 跳出循环
            }

            // 5.2 从解码器接收解码后的帧
            while (true) {
                ret = avcodec_receive_frame(codec_ctx_, frame); // 接收解码后的帧
                if (ret == 0) { // 接收成功
                    frame_queue_->Push(frame); // 将解码后的帧推入帧队列
                    LogInfo("%s frame queue size %d", codec_ctx_->codec->name, frame_queue_->Size()); // 日志记录队列大小
                    continue; // 继续读取下一帧
                } else if (AVERROR(EAGAIN) == ret) {
                    break; // 如果返回 EAGAIN，表示当前没有更多的帧可读，退出循环
                } else {
                    abort_ = 1; // 设置 abort 标志
                    av_strerror(ret, err2str, sizeof(err2str));
                    LogError("avcodec_receive_frame failed, ret:%d, err2str:%s", ret, err2str);
                    break; // 跳出循环
                }
            }
        } else {
            LogInfo("not got packet"); // 如果未获取到包，记录日志
        }
    }
    LogInfo("Run Finish"); // 日志记录解码线程结束
}
