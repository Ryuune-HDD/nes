#ifndef NES_ROM_H
#define NES_ROM_H
#include <stdint.h>

size_t rom_get_size(const char* file);
int rom_read(const char* file, uint8_t* romfile);

#endif //NES_ROM_H
