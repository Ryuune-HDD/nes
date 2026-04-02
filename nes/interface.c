#include "interface.h"
#include <stdint.h>
#include "nes_apu.h"
#include "../devices/display.h"
#include "../devices/pcm.h"
#include "../devices/key.h"
#include "../devices/rom.h"

/********************** 设备初始化和关闭 ************************/
void devices_init()
{
#if !ENABLE_LOG
    DisplayWindowInit();
#endif
    pcm_init(APU_SAMPLE_RATE);
}

void devices_uninit()
{
    pcm_cleanup();
}


/********************** 文件读取 ************************/
// 获取ROM文件大小（单位：字节）
size_t nes_rom_get_size(const char* file)
{
    return rom_get_size(file);
}

// 从文件读取 ROM 数据到指定缓冲区 romfile
int nes_rom_read(const char* file, uint8_t* romfile)
{
    return rom_read(file, romfile);
}

/********************** 屏幕显示 ************************/
void nes_display_write(uint16_t* rgb565_data, uint8_t line)
{
    display_write(rgb565_data, line);
}

/********************** 音频播放 ************************/
void nes_audio_write(uint16_t* buffer, size_t sample_count)
{
    pcm_submit_buffer(buffer, sample_count);
}

/********************** 按键监听 ************************/
void nes_key_read_state(uint8_t* pad0, uint8_t* pad1)
{
    key_read_state(pad0, pad1);
}
