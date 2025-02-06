#include "audiooutput.h"
#include "log.h"

// 构造函数：初始化成员变量
AudioOutput::AudioOutput(AVSync *avsync, AVRational time_base, const AudioParams &audio_params, AVFrameQueue *frame_queue)
    :avsync_(avsync), time_base_(time_base), src_tgt_(audio_params), frame_queue_(frame_queue)
{
}

// 析构函数：清理资源
AudioOutput::~AudioOutput()
{
}

// PCM 数据转储文件（用于调试）
FILE *dump_pcm = NULL;

// SDL 音频回调函数：填充音频数据到 SDL 音频缓冲区
void fill_audio_pcm(void *udata, Uint8 *stream, int len) {

    // 1. 从frame queue读取解码后的PCM的数据，填充到stream
    // 2. 举例：len = 4000字节， 一个frame有6000字节， 第一次读取了4000 len1 = 4000， 这个frame剩了2000字节
    //         第二次调用该函数的时候，audio_buf直接从index开始读，此时audio_buf_size = 2000，先读2000字节，把上一个frame读完
    //         然后再次进入while循环的时候，audio_buf读完了，要读取新的frame了
    // 所以其实 len和audio_buf是变化的，len初始是一次回调填充的数据长度，auido_buf指向的就是一帧的音频数据，
    // 而audio_buf_1和audio_size其实是一个定值，audio_buf_1它一定指向重采样/未重采样的一帧音频数据的数据缓冲区（可能比真正的音频数据大小audio_size要大）
    AudioOutput *is = (AudioOutput *)udata;
    int len1 = 0;        // 每次循环可写入音频数据的长度
    int audio_size = 0;  // 一帧音频数据的大小
    if(!dump_pcm) {
           dump_pcm = fopen("dump.pcm", "wb");
        }
    LogInfo("fill pcm len:%d", len);

    // 从 FrameQueue 循环读取数据，不断写进入stream，直到写入了len大小的字节为止
    while (len > 0) {
        // 如果当前音频缓冲区已读完，则从队列中取出新的音频帧
        if(is->audio_buf_index == is->audio_buf_size) {
            is->audio_buf_index = 0;   // 更新index，从头开始读
            AVFrame *frame = is->frame_queue_->Pop(10);
            if(frame) {  // 读到解码后的数据
                // 更新pts
                is->pts_ = frame->pts;

                // 判断是否需要进行重采样
                if( ((frame->format != is->dst_tgt_.fmt)
                        || (frame->sample_rate != is->dst_tgt_.freq)
                        ||  (frame->channel_layout != is->dst_tgt_.channel_layout))
                        && (!is->swr_ctx_)) {
                    // 初始化重采样上下文
                    is->swr_ctx_ = swr_alloc_set_opts(NULL,
                                                      is->dst_tgt_.channel_layout,
                                                      (enum AVSampleFormat)is->dst_tgt_.fmt,
                                                      is->dst_tgt_.freq,
                                                      frame->channel_layout,
                                                      (enum AVSampleFormat)frame->format,
                                                      frame->sample_rate,
                                                      0, NULL);
                    // 检查重采样上下文是否成功分配并初始化
                    if (!is->swr_ctx_ || swr_init(is->swr_ctx_) < 0) {
                        LogError(
                               "Cannot create sample rate converter for conversion of %d Hz %s %d channels to %d Hz %s %d channels!\n",
                               frame->sample_rate,
                               av_get_sample_fmt_name((enum AVSampleFormat)frame->format),
                               frame->channels,
                               is->dst_tgt_.freq,
                               av_get_sample_fmt_name((enum AVSampleFormat)is->dst_tgt_.fmt),
                               is->dst_tgt_.channels);
                        swr_free((SwrContext **)(&is->swr_ctx_));
                        return;
                    }
                }
                // 执行重采样
                if(is->swr_ctx_) { // 重采样
                    const uint8_t **in = (const uint8_t **)frame->extended_data;
                    uint8_t **out = &is->audio_buf1_;
                    int out_samples = frame->nb_samples * is->dst_tgt_.freq/frame->sample_rate + 256;
                    int out_bytes = av_samples_get_buffer_size(NULL, is->dst_tgt_.channels, out_samples, is->dst_tgt_.fmt, 0);
                    if(out_bytes <0) {
                        LogError("av_samples_get_buffer_size failed");
                        return;
                    }
                    av_fast_malloc(&is->audio_buf1_, &is->audio_buf1_size, out_bytes);

                    int len2 = swr_convert(is->swr_ctx_, out, out_samples, in, frame->nb_samples); // 返回重采样后的样本数量
                    if(len2 <0) {
                        LogError("swr_convert failed");
                        return;
                    }
                    is->audio_buf_ = is->audio_buf1_;
                    is->audio_buf_size = av_samples_get_buffer_size(NULL, is->dst_tgt_.channels, len2, is->dst_tgt_.fmt, 1);
                } else { // 没有重采样
                    audio_size = av_samples_get_buffer_size(NULL, frame->channels, frame->nb_samples, (enum AVSampleFormat)frame->format, 1);
                    av_fast_malloc(&is->audio_buf1_, &is->audio_buf1_size, audio_size);
                    is->audio_buf_ = is->audio_buf1_;
                    is->audio_buf_size = audio_size;
                    memcpy(is->audio_buf_, frame->data[0], audio_size);
                }
                av_frame_free(&frame);
            }
            else { // 没有读到解码后的数据（设置，将来填充静音数据）
                is->audio_buf_ = NULL;
                is->audio_buf_size = 512;
            }
        }
        // 计算当前（本次循环）可写入的长度len1（audio_buf中剩余的字节数，一般为audio_buf全部数据，也有可能是上次回调函数调用完剩下的buffer容量）
        // audio_buf size < len的时候，是audio_buf size，比len大的时候，发剩余的len就行
        len1 = is->audio_buf_size - is->audio_buf_index;
        if (len1 > len)  // 如果len1长度大于目前剩下的len长度
            len1 = len;  // 只需要传len长度

        if(!is->audio_buf_) {
            // 填充静音数据
            memset(stream, 0, len1);
        } else {
            // 真正拷贝有效的数据
            memcpy(stream, is->audio_buf_ + is->audio_buf_index, len1);
            SDL_MixAudio(stream, is->audio_buf_ + is->audio_buf_index, len1, SDL_MIX_MAXVOLUME/8 );
            fwrite((uint8_t *)is->audio_buf_ + is->audio_buf_index, 1, len1, dump_pcm);
                        fflush(dump_pcm);
        }
        // 更新剩余长度和索引
        len -= len1;   // 减少 len 的值，表示剩余需要写入的字节数
        stream += len1; // 移动输出流指针 stream，指向下一个写入位置
        is->audio_buf_index += len1;  // 更新is->audio_buf_index，指向audio_buf中未被拷⻉到stream的剩余数据的起始位置
    }
	// 设置时钟
    if(is->pts_ != AV_NOPTS_VALUE) {
        double pts = is->pts_ * av_q2d(is->time_base_);  // pts单位从timebase转换成秒
        LogInfo("audio pts:%0.3lf\n", pts);
        is->avsync_->SetClock(pts);  // 更新音频时钟，设置audio clock（对时）
	}
}


int AudioOutput::Init()
{
    // 尝试初始化 SDL 音频子系统
    if(SDL_Init(SDL_INIT_AUDIO) != 0) {
        LogError("SDL_Init failed"); // 如果初始化失败，记录错误日志
        return -1; // 返回-1表示初始化失败
    }

    // 期望音频参数，实际音频参数 （SDL音频参数的结构体）
    SDL_AudioSpec wanted_spec, spec; // wanted_spec会变化，spec是固定值，最终它俩会一致
    // 这里省略wanted_spec的变化尝试，直接给出合法的wanted_spec
    wanted_spec.channels = 2; // 只支持双声道输出（立体声）
    wanted_spec.freq = src_tgt_.freq; // 从源目标获取的采样频率
    wanted_spec.format = AUDIO_S16SYS; // 使用系统默认的 16 位 PCM 格式
    wanted_spec.silence = 0; // 设置静声音量为 0
    wanted_spec.callback = fill_audio_pcm; // 设置音频回调函数，负责音频的填充
    wanted_spec.userdata = this; // 将当前 AudioOutput 实例指针设置为用户数据，以便在回调中访问
    wanted_spec.samples = 1024; // 设置一次性请求的样本数（可能的缓冲区大小）

    // 尝试打开音频设备，实际sdl音频参数会存入wanted_spec中
    int ret = SDL_OpenAudio(&wanted_spec, NULL);  // 根据设置的参数打开音频设备
    if(ret != 0) {
        LogError("SDL_OpenAudio failed"); // 如果打开失败，记录错误日志
        return -1; // 返回-1表示打开失败
    }

    // 根据SDL实际返回的音频参数来配置ffmepg的目标音频格式
    dst_tgt_.channels = wanted_spec.channels; // 目标声道数 spec.channels
    dst_tgt_.fmt = AV_SAMPLE_FMT_S16; // 目标采样格式，设置为 16 位 PCM
    dst_tgt_.freq = wanted_spec.freq; // 目标采样频率 spec.freq
    dst_tgt_.channel_layout = av_get_default_channel_layout(2); // 获取默认声道布局（立体声）
    dst_tgt_.frame_size = 1024; // 设置每帧大小为 1024（样本数）src_tgt_.frame_size

    SDL_PauseAudio(0); // 开始播放音频，设置为不暂停状态（0 表示播放）

    LogInfo("AudioOutput::Init() leave"); // 记录初始化完成信息
}

int AudioOutput::DeInit()
{
    SDL_PauseAudio(1); // 暂停音频播放（1 表示暂停）
    SDL_CloseAudio(); // 关闭音频设备
    LogInfo("AudioOutput::DeInit() leave"); // 记录反初始化完成信息
}
