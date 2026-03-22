#ifndef __NES_MAIN_H
#define __NES_MAIN_H

#include <stdint.h>

//////////////////////////////////////////////////////////////////////////////////	 
//本程序移植自网友ye781205的NES模拟器工程
//ALIENTEK STM32F407开发板
//NES主函数 代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2014/7/1
//版本：V1.0  			  
////////////////////////////////////////////////////////////////////////////////// 	 

#include <stdint.h>
#include <stdbool.h>

#define NES_SKIP_FRAME 	2		//定义模拟器跳帧数,默认跳2帧

// #define INLINE 	static inline
// #define int8_t 	char
// #define int16_t 	short
// #define int32_t 	int
// #define uint8_t 	unsigned char
// #define uint16_t 	unsigned short
// #define uint32_t 	unsigned int
// // #define boolean bool
//
// typedef uint8_t uint8_t;
// typedef uint16_t uint16_t;
// typedef uint32_t uint32_t;

//nes信息头结构体
typedef struct
{
	uint8_t id[3]; // 'NES'
	uint8_t ctrl_z; // control-z
	uint8_t num_16k_rom_banks;
	uint8_t num_8k_vrom_banks;
	uint8_t flags_1;
	uint8_t flags_2;
	uint8_t reserved[8];
} NES_header;

extern uint8_t nes_frame_cnt; //nes帧计数器
extern int MapperNo; //map编号
extern int NES_scanline; //扫描线
extern NES_header* RomHeader; //rom文件头
extern int VROM_1K_SIZE;
extern int VROM_8K_SIZE;
extern uint8_t cpunmi; //cpu中断标志  在 6502.s里面
extern uint8_t cpuirq;
extern uint8_t PADdata0; //手柄0键值
extern uint8_t PADdata1; //手柄1键值
extern uint8_t lianan_biao; //连按标志
#define  CPU_NMI  cpunmi=1;
#define  CPU_IRQ  cpuirq=1;
#define  NES_RAM_SPEED	0 	 	//1:内存占用小  0:速度快


void cpu6502_init(void); //在 cart.s
void run6502(uint32_t); //在 6502.s
uint8_t nes_load_rom(void);
void nes_sram_free(void);
uint8_t nes_sram_malloc(uint32_t romsize);
uint8_t nes_load(const char* pname);
void nes_set_window(void);
void nes_get_gamepadval(void);
void nes_emulate_frame(void);
void debug_6502(uint16_t reg0, uint8_t reg1);

void nes_i2s_dma_tx_callback(void);
int nes_sound_open(int samples_per_sync, int sample_rate);
void nes_sound_close(void);
void nes_apu_fill_buffer(int samples, uint16_t* wavebuf);

extern uint32_t get_crc32(uint8_t* buf, uint32_t len);
#endif







