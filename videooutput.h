#ifndef VIDEOOUTPUT_H
#define VIDEOOUTPUT_H

#ifdef __cplusplus
extern "C" {
// 包含SDL头文件
#include "SDL.h"  // SDL库，用于图形渲染和事件处理
#include "libavutil/time.h"  // FFmpeg时间工具库
}
#endif

#include "avframequeue.h"  // 包含自定义的AVFrameQueue类头文件，用于存储视频帧
#include "avsync.h"  // 包含音视频同步模块的头文件

class VideoOutput {
public:
    // 构造函数，初始化视频输出的宽度和高度，以及帧队列
    VideoOutput(AVSync *avsync, AVRational time_base, AVFrameQueue *frame_queue, int video_width, int video_height);

    // 初始化SDL窗口、渲染器和纹理
    int Init();

    // 主循环，处理事件和渲染
    int MainLoop();

    // 等待事件并刷新视频画面
    void RefreshLoopWaitEvent(SDL_Event *event);

private:
    // 刷新视频画面
    void videoRefresh(double *remaining_time);

    AVFrameQueue *frame_queue_ = NULL;  // 帧队列指针
    SDL_Event event_;  // SDL事件对象
    SDL_Rect rect_;  // 渲染矩形区域
    SDL_Window *win_ = NULL;  // SDL窗口对象
    SDL_Renderer *renderer_ = NULL;  // SDL渲染器对象
    SDL_Texture *texture_ = NULL;  // SDL纹理对象

    AVSync *avsync_ = NULL;  // 音视频同步模块指针
    AVRational time_base_;  // 时间基准
    int video_width_ = 0;  // 视频宽度
    int video_height_ = 0;  // 视频高度
    uint8_t *yuv_buf_ = NULL;  // YUV缓冲区指针
    int yuv_buf_size_ = 0;  // YUV缓冲区大小
};

#endif // VIDEOOUTPUT_H
