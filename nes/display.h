// display.h
#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {



#endif

void StartDisplayWindow(void);
void display_write(uint16_t* data, uint8_t line); // 输入一行 256 宽的 RGB565 数据及目标行号

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_H
