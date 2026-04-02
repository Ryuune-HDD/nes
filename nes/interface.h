#ifndef NES_INTERFACE_H
#define NES_INTERFACE_H

#include "stdint.h"

// 控制是否启用打印（0=关闭，1=开启）
#define ENABLE_LOG 0
#if ENABLE_LOG
#define LOG(fmt, ...) do { \
printf("[LOG] " fmt "\n", ##__VA_ARGS__); \
fflush(stdout); \
} while(0)
#else
#define LOG(fmt, ...)  // 空操作
#endif
void devices_init();
void devices_uninit();
size_t nes_rom_get_size(const char* file);
int nes_rom_read(const char* file, uint8_t* romfile);
void nes_display_write(uint16_t* rgb565_data, uint8_t line);
void nes_audio_write(uint16_t* buffer, size_t sample_count);
void nes_key_read_state(uint8_t* pad0, uint8_t* pad1);
#endif //NES_INTERFACE_H
