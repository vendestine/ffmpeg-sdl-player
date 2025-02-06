#include "videooutput.h"
#include "log.h" // 日志记录功能
#include <thread> // 线程库

// 构造函数实现
VideoOutput::VideoOutput(AVSync *avsync, AVRational time_base, AVFrameQueue *frame_queue, int video_width, int video_height)
    : avsync_(avsync), time_base_(time_base),
      frame_queue_(frame_queue), video_width_(video_width), video_height_(video_height)
{
}

// 初始化SDL窗口、渲染器和纹理
int VideoOutput::Init()
{
    // 初始化SDL视频子系统
    if (SDL_Init(SDL_INIT_VIDEO)) {
        LogError("SDL_Init failed");  // 日志记录错误
        return -1;
    }

    // 创建SDL窗口
    win_ = SDL_CreateWindow("player", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                            video_width_, video_height_, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!win_) {
        LogError("SDL_CreateWindow failed");  // 日志记录错误
        return -1;
    }

    // 创建SDL渲染器
    renderer_ = SDL_CreateRenderer(win_, -1, 0);
    if (!renderer_) {
        LogError("SDL_CreateRenderer failed");  // 日志记录错误
        return -1;
    }

    // 创建SDL纹理（YUV格式）
    texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, video_width_, video_height_);
    if (!texture_) {
        LogError("SDL_CreateTexture failed");  // 日志记录错误
        return -1;
    }

    // 计算YUV缓冲区大小并分配内存
    yuv_buf_size_ = video_width_ * video_height_ * 1.5;  // YUV420格式的大小
    yuv_buf_ = (uint8_t *)malloc(yuv_buf_size_);  // 分配内存

    return 0;  // 初始化成功
}

// 主循环：处理SDL事件
int VideoOutput::MainLoop()
{
    SDL_Event event;
    while (true) {
        // 等待事件并刷新画面
        RefreshLoopWaitEvent(&event);

        // 处理事件
        switch (event.type) {
        case SDL_KEYDOWN:  // 按键事件
            if (event.key.keysym.sym == SDLK_ESCAPE) {
                LogInfo("esc key down");  // 日志记录ESC键按下
                return 0;  // 退出主循环
            }
            break;
        case SDL_QUIT:  // 退出事件
            LogInfo("SDL_QUIT");  // 日志记录退出事件
            return 0;  // 退出主循环
            break;
        default:
            break;
        }
    }
}

// 定义刷新率（秒）
#define REFRESH_RATE 0.01

// 等待事件并刷新画面
void VideoOutput::RefreshLoopWaitEvent(SDL_Event *event)
{
    double remaining_time = 0.0;
    SDL_PumpEvents();  // 更新事件队列
    while (!SDL_PeepEvents(event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT)) {
        if (remaining_time > 0.0)
            std::this_thread::sleep_for(std::chrono::milliseconds(int64_t(remaining_time * 1000.0)));  // 休眠
        remaining_time = REFRESH_RATE;
        videoRefresh(&remaining_time);  // 刷新画面
        SDL_PumpEvents();  // 更新事件队列
    }
}

// 刷新视频画面
void VideoOutput::videoRefresh(double *remaining_time)
{
    AVFrame *frame = NULL;
    frame = frame_queue_->Front();  // 从帧队列中获取当前帧
    if (frame) {
        double pts = frame->pts * av_q2d(time_base_);  // 计算当前帧的显示时间
        LogInfo("video pts:%0.3lf\n", pts);
        double diff = pts - avsync_->GetClock();  // 计算当前帧与音频时钟的差值（其实也就是audio_clock和video_clock的差值）
        if (diff > 0) {
            *remaining_time = FFMIN(*remaining_time, diff);  // 调整剩余时间
            return;
        }
        // 设置渲染区域
        rect_.x = 0;
        rect_.y = 0;
        rect_.w = video_width_;
        rect_.h = video_height_;

        // 更新纹理数据(rect表示更新rect范围内的纹理）
        SDL_UpdateYUVTexture(texture_, &rect_, frame->data[0], frame->linesize[0],
                             frame->data[1], frame->linesize[1],
                             frame->data[2], frame->linesize[2]);

        // 渲染纹理
        SDL_RenderClear(renderer_);  // 清空渲染器
        SDL_RenderCopy(renderer_, texture_, NULL, &rect_);  // 渲染纹理
        SDL_RenderPresent(renderer_);  // 显示渲染结果

        // 从队列中移除当前帧并释放资源
        frame = frame_queue_->Pop(1);
        av_frame_free(&frame);
    }
}







