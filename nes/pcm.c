#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* 上下文结构体 */
typedef struct
{
    ma_device device;
    ma_pcm_rb rb; /* 环形缓冲区 */
    ma_decoder decoder; /* 解码器 */
    ma_bool32 is_initialized;
} pcm_context_t;

static pcm_context_t g_ctx = {0};

/* 音频回调函数：从环形缓冲区取数据给声卡 */
static void audio_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    (void)pInput;
    pcm_context_t* ctx = (pcm_context_t*)pDevice->pUserData;

    /* 1. 申请读取 */
    void* pReadBuffer;
    ma_uint32 framesToRead = frameCount;
    ma_pcm_rb_acquire_read(&ctx->rb, &framesToRead, &pReadBuffer);

    /* 2. 拷贝数据到输出 (pOutput) */
    if (framesToRead > 0)
    {
        /* 计算字节大小：帧数 * 2通道 * 2字节(16bit) */
        memcpy(pOutput, pReadBuffer, framesToRead * 2 * 2);

        /* 3. 提交读取，移动读指针 */
        ma_pcm_rb_commit_read(&ctx->rb, framesToRead);
    }

    /* 如果读不够，剩余部分不管 (miniaudio 默认会填零) */
}

/* 初始化：只接受采样率 */
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
       参数: 格式, 通道数, 缓冲区帧数, 可选内存分配器, 结构体指针
       缓冲区设为采样率的2倍(约2秒)，防止卡顿 */
    if (ma_pcm_rb_init(ma_format_s16, 2, sample_rate * 2, NULL, NULL, &g_ctx.rb) != MA_SUCCESS)
    {
        ma_device_uninit(&g_ctx.device);
        return -1;
    }

    g_ctx.is_initialized = MA_TRUE;

    /* 启动设备 */
    ma_device_start(&g_ctx.device);

    return 0;
}

/* 提交数据：将 NES 的数据写入环形缓冲区 */
int pcm_submit_buffer(const uint16_t* buffer, size_t sample_count)
{
    if (!g_ctx.is_initialized) return -1;

    /* 样本数转帧数 (16位立体声: 2样本=1帧) */
    ma_uint32 frame_count = (ma_uint32)(sample_count / 2);

    /* 1. 申请写入空间 */
    void* pWriteBuffer;
    ma_pcm_rb_acquire_write(&g_ctx.rb, &frame_count, &pWriteBuffer);

    /* 2. 拷贝数据进去 */
    if (frame_count > 0)
    {
        memcpy(pWriteBuffer, buffer, frame_count * 2 * 2);

        /* 3. 提交写入，移动写指针 */
        ma_pcm_rb_commit_write(&g_ctx.rb, frame_count);
    }

    return 0;
}

/* 播放文件：解码后写入缓冲区 */
int pcm_play_file(const char* filename)
{
    if (!g_ctx.is_initialized) return -1;

    ma_device_stop(&g_ctx.device);

    ma_decoder_config dec_config = ma_decoder_config_init(ma_format_s16, 2, g_ctx.device.sampleRate);
    if (ma_decoder_init_file(filename, &dec_config, &g_ctx.decoder) != MA_SUCCESS)
    {
        return -1;
    }

    /* 临时缓冲区，用于读取解码数据 */
    uint8_t temp_buffer[4096];
    while (1)
    {
        ma_uint64 frames_read;
        ma_decoder_read_pcm_frames(&g_ctx.decoder, temp_buffer, sizeof(temp_buffer) / 4, &frames_read);

        if (frames_read == 0) break;
 /* 再次提交给环形缓冲区 */
        pcm_submit_buffer((const uint16_t*)temp_buffer, frames_read * 2);

        /* 等待缓冲区有空间 (简单防溢出) */
        while (ma_pcm_rb_available_write(&g_ctx.rb) < frames_read)
        {
            /* 可以加个 Sleep(1) 避免死循环占用CPU */
        }
    }

    ma_decoder_uninit(&g_ctx.decoder);
    return 0;
}

void pcm_stop(void)
{
    if (g_ctx.is_initialized)
    {
        ma_device_stop(&g_ctx.device);
    }
}

void pcm_cleanup(void)
{
    if (!g_ctx.is_initialized) return;

    ma_device_uninit(&g_ctx.device);
    ma_pcm_rb_uninit(&g_ctx.rb);
    memset(&g_ctx, 0, sizeof(g_ctx));
}
