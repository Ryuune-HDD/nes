#ifndef __NES_MAPPER_H
#define __NES_MAPPER_H  
#include "nes_main.h"
#include "nes_ppu.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序移植自网友ye781205的NES模拟器工程
//ALIENTEK STM32F407开发板
//NES MAP 驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2014/7/1
//版本：V1.0  			  
////////////////////////////////////////////////////////////////////////////////// 	
#define  num_8k_ROM_banks	VROM_8K_SIZE
#define  num_1k_VROM_banks	VROM_1K_SIZE

////////////////////////////////////////////////////////////////////////////////// 	
typedef struct
{
    void (*Reset)();
    void (*Write)(uint16_t addr, uint8_t data);
    void (*Read)(uint8_t data, uint16_t addr);
    uint8_t (*ReadLow)(uint16_t addr);
    void (*WriteLow)(uint16_t addr, uint8_t data);
    void (*HSync)(int scanline);
    void (*VSync)(void);
} MAPPER;

/////////////////////////////////////////////////////////////////////////////////
typedef enum
{
    MMC1_SMALL,
    MMC1_512K,
    MMC1_1024K
} MMC1_Size_t;

typedef struct
{
    uint32_t write_count;
    uint8_t bits;
    uint8_t regs[4];
    uint32_t last_write_addr;

    MMC1_Size_t MMC1_Size;
    uint32_t MMC1_256K_base;
    uint32_t MMC1_swap;

    // these are the 4 ROM banks currently selected
    uint32_t MMC1_bank1;
    uint32_t MMC1_bank2;
    uint32_t MMC1_bank3;
    uint32_t MMC1_bank4;

    uint32_t MMC1_HI1;
    uint32_t MMC1_HI2;
} Mapper1Res;

//通用map变量结构体
typedef struct
{
    uint8_t patch;
    uint8_t regs[11];

    uint32_t prg0, prg1;
    uint32_t chr01, chr23, chr4, chr5, chr6, chr7;

    uint8_t irq_enabled; // IRQs enabled
    uint32_t irq_counter; // IRQ scanline counter, decreasing
    uint32_t irq_latch; // IRQ scanline counter latch
} MapperCommRes;

extern uint32_t ROM_mask;
extern uint32_t VROM_mask;
extern const int MapTab[];
extern MAPPER* NES_Mapper;
extern uint32_t VROM_mask;

extern Mapper1Res* MAP1;
extern MapperCommRes* MAPx;


////////////////////////////////////////////////////////////////////////////////// 	
void Mapper_Init(void);
void asm_Mapper_Write(uint8_t byData, uint16_t wAddr);
uint8_t asm_Mapper_ReadLow(uint16_t wAddr);
void asm_Mapper_WriteLow(uint8_t byData, uint16_t wAddr);

void map67_(signed char page); //6502.s
void map89_(signed char page);
void mapAB_(signed char page);
void mapCD_(signed char page);
void mapEF_(signed char page); //有符号字符

void set_CPU_bank3(signed char page);
void set_CPU_bank4(signed char page);
void set_CPU_bank5(signed char page);
void set_CPU_bank6(signed char page);
void set_CPU_bank7(signed char page);
void set_CPU_banks(int bank0_num, int bank1_num, int bank2_num, int bank3_num);

void set_PPU_banks(uint32_t bank0_num, uint32_t bank1_num,
                   uint32_t bank2_num, uint32_t bank3_num,
                   uint32_t bank4_num, uint32_t bank5_num,
                   uint32_t bank6_num, uint32_t bank7_num);
void set_PPU_bank0(uint32_t bank_num);
void set_PPU_bank1(uint32_t bank_num);
void set_PPU_bank2(uint32_t bank_num);
void set_PPU_bank3(uint32_t bank_num);
void set_PPU_bank4(uint32_t bank_num);
void set_PPU_bank5(uint32_t bank_num);
void set_PPU_bank6(uint32_t bank_num);
void set_PPU_bank7(uint32_t bank_num);
void set_PPU_bank8(uint32_t bank_num);
void set_PPU_bank9(uint32_t bank_num);
void set_PPU_bank10(uint32_t bank_num);
void set_PPU_bank11(uint32_t bank_num);
void set_VRAM_bank(uint8_t bank, uint32_t bank_num);


#endif
