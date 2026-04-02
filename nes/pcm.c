#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ============== 播放器上下文 ============== */
typedef struct
{
    ma_device device;
    ma_pcm_rb rb; /* 环形缓冲区 */
    ma_bool32 is_initialized;
} pcm_context_t;

static pcm_context_t g_ctx = {0};

/* ============== 数据回调函数 (消费者) ============== */
static void audio_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    (void)pInput;
    pcm_context_t* ctx = (pcm_context_t*)pDevice->pUserData;

    /* 从环形缓冲区读取数据 */
    void* pReadBuffer;
    ma_uint32 framesToRead = frameCount;

    /* 1. 申请读取 */
    ma_pcm_rb_acquire_read(&ctx->rb, &framesToRead, &pReadBuffer);

    /* 2. 拷贝数据到输出 */
    if (framesToRead > 0)
    {
        memcpy(pOutput, pReadBuffer, framesToRead * 2 * 2); /* 16bit立体声=4字节/帧 */
        ma_pcm_rb_commit_read(&ctx->rb, framesToRead);
    }

    /* 如果读不够，剩余部分保持静音 (miniaudio默认已清零) */
}

/* ============== 初始化 PCM 系统 ============== */
int pcm_init(uint32_t sample_rate)
{
    if (g_ctx.is_initialized) return 0;

    /* 1. 设备配置：固定 16位，立体声 */
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_s16;
    config.playback.channels = 2;
    config.sampleRate = sample_rate;
    config.dataCallback = audio_callback;
    config.pUserData = &g_ctx;

    if (ma_device_init(NULL, &config, &g_ctx.device) != MA_SUCCESS)
    {
        return -1;
    }

    /* 2. 初始化环形缓冲区
       【关键修改】：缓冲区大小设置为大约 2~3 帧的数据量 (约30ms~50ms)
       NES 一帧产生 sample_rate/60 个样本。
       这里设置为 sample_rate/20 (50ms)，既能平滑播放，又能有效限制模拟器速度
    */
    ma_uint32 buffer_frames = sample_rate / 20;
    if (buffer_frames < 1024) buffer_frames = 1024; /* 安全下限 */

    if (ma_pcm_rb_init(ma_format_s16, 2, buffer_frames, NULL, NULL, &g_ctx.rb) != MA_SUCCESS)
    {
        ma_device_uninit(&g_ctx.device);
        return -1;
    }

    g_ctx.is_initialized = MA_TRUE;

    /* 启动设备 */
    ma_device_start(&g_ctx.device);
 return 0;
}

/* ============== 提交数据 (生产者，带阻塞) ============== */
int pcm_submit_buffer(const uint16_t* buffer, size_t sample_count)
{
    if (!g_ctx.is_initialized) return -1;

    /* 样本数转帧数 (16位立体声: 2样本=1帧) */
    ma_uint32 total_frames = (ma_uint32)(sample_count / 2);
    ma_uint32 frames_written = 0;

    /* 循环写入，直到所有数据都写入缓冲区 */
    while (frames_written < total_frames)
    {
        /* 检查可用空间 */
        ma_uint32 available = ma_pcm_rb_available_write(&g_ctx.rb);

        if (available == 0)
        {
            /* 【关键修改】：缓冲区满了，说明模拟器跑得太快
               休眠 1ms 等待音频设备消费数据
               这替代了 nes_emulate_frame 中的 Sleep(1)，实现自动同步 */
            ma_sleep(1);
            continue;
        }

        /* 计算本次写入量 */
        ma_uint32 frames_to_write = total_frames - frames_written;
        if (frames_to_write > available)
        {
            frames_to_write = available;
        }

        void* pWriteBuffer;
        /* ma_pcm_rb_acquire_write 可能会返回比请求更小的连续内存块，
           所以需要用 frames_to_write 传入并获取实际可写指针 */
        ma_pcm_rb_acquire_write(&g_ctx.rb, &frames_to_write, &pWriteBuffer);

        if (frames_to_write > 0)
        {
            /* 拷贝数据：源地址需要根据已写入帧数偏移 */
            memcpy(pWriteBuffer, (uint8_t*)buffer + (frames_written * 4), frames_to_write * 4);
            ma_pcm_rb_commit_write(&g_ctx.rb, frames_to_write);
            frames_written += frames_to_write;
        }
    }

    return 0;
}

/* ============== 停止播放 ============== */
void pcm_stop(void)
{
    if (g_ctx.is_initialized)
    {
        ma_device_stop(&g_ctx.device);
    }
}

/* ============== 清理资源 ============== */
void pcm_cleanup(void)
{
    if (!g_ctx.is_initialized) return;

    ma_device_stop(&g_ctx.device);
    ma_device_uninit(&g_ctx.device);
    ma_pcm_rb_uninit(&g_ctx.rb);

    memset(&g_ctx, 0, sizeof(g_ctx));
}
