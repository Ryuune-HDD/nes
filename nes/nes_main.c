#include "nes_main.h"
#include "nes_ppu.h"
#include "nes_mapper.h"
#include "nes_apu.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// 以下库需要改
// 自己的库
#include <pcm.h>
#include <windows.h>

#include "display.h"
#include "nes_cpu.h"
#include "interface.h"
//////////////////////////////////////////////////////////////////////////////////
//本程序移植自网友ye781205的NES模拟器工程
//ALIENTEK STM32F407开发板
//NES主函数 代码
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2014/7/1
//版本：V1.0
//////////////////////////////////////////////////////////////////////////////////

extern uint8_t frame; //nes帧计数器
uint8_t nes_frame_cnt; //nes帧计数器
int MapperNo; //map编号
int NES_scanline; //nes扫描线
int VROM_1K_SIZE;
int VROM_8K_SIZE;

uint8_t PADdata0; //手柄1键值 [7:0]右7 左6 下5 上4 Start3 Select2 B1 A0
uint8_t PADdata1; //手柄2键值 [7:0]右7 左6 下5 上4 Start3 Select2 B1 A0

uint8_t cpunmi; //cpu中断标志
uint8_t cpuirq; //IRQ 标志
uint32_t clocks; //CPU 时钟 APU 专用

uint8_t* NES_RAM; //保持1024字节对齐
uint8_t* NES_SRAM;
NES_header* RomHeader; //rom文件头
MAPPER* NES_Mapper;
MapperCommRes* MAPx;

uint8_t* romfile; //nes文件指针,指向整个nes文件的起始地址.

uint8_t* spr_ram; //精灵RAM,256字节
ppu_data* ppu; //ppu指针
uint8_t* VROM_banks;
uint8_t* VROM_tiles;

apu_t* apu; //apu指针
uint16_t* wave_buffers;

uint16_t num = 0;
//uint8_t* romfile;			//nes文件指针,指向整个nes文件的起始地址.
//////////////////////////////////////////////////////////////////////////////////////

uint8_t system_task_return = 0; //任务强制返回标志.

//加载ROM
//返回值:0,成功
//       1,内存错误
//       3,map错误
uint8_t nes_load_rom(void)
{
	uint8_t i;
	uint8_t res = 0;
	uint8_t* p = (uint8_t*)romfile;

	if (strncmp((char*)p, "NES", 3) == 0)
	{
		RomHeader->ctrl_z = p[3];
		RomHeader->num_16k_rom_banks = p[4];
		RomHeader->num_8k_vrom_banks = p[5];
		RomHeader->flags_1 = p[6];
		RomHeader->flags_2 = p[7];
		if (RomHeader->flags_1 & 0x04)
		{
			p += 512; //有512字节的trainer:
		}
		if (RomHeader->num_8k_vrom_banks > 0) //存在VROM,进行预解码
		{
			VROM_banks = p + 16 + (RomHeader->num_16k_rom_banks * 0x4000);
#if	NES_RAM_SPEED==1	//1:内存占用小 0:速度快
			VROM_tiles = VROM_banks;
#else
			//VROM_tiles=mymalloc(SRAMIN,RomHeader->num_8k_vrom_banks*8*1024);//这里可能申请多达1MB内存!!!
			VROM_tiles = malloc(RomHeader->num_8k_vrom_banks * 8 * 1024); //这里可能申请多达1MB内存!!!///Caution
			if (VROM_tiles == 0)VROM_tiles = VROM_banks; //内存不够用的情况下,尝试VROM_titles与VROM_banks共用内存
			compile(RomHeader->num_8k_vrom_banks * 8 * 1024 / 16, VROM_banks, VROM_tiles);
#endif
		}
		else
		{
			//			VROM_banks=mymalloc(SRAMIN,8*1024);
			//			VROM_tiles=mymalloc(SRAMIN,8*1024);
			VROM_banks = malloc(8 * 1024); ///Caution
			VROM_tiles = malloc(8 * 1024); ///Caution
			if (!VROM_banks || !VROM_tiles)res = 1;
		}
		VROM_1K_SIZE = RomHeader->num_8k_vrom_banks * 8;
		VROM_8K_SIZE = RomHeader->num_8k_vrom_banks;
		MapperNo = (RomHeader->flags_1 >> 4) | (RomHeader->flags_2 & 0xf0);
		if (RomHeader->flags_2 & 0x0E) MapperNo = RomHeader->flags_1 >> 4; //忽略高四位，如果头看起来很糟糕
		LOG("Mapper: %d", MapperNo);
		for (i = 0; i < 255; i++) // 查找支持的Mapper号
		{
			if (MapTab[i] == MapperNo)break;
			if (MapTab[i] == -1)res = 3;
		}
		if (res == 0)
		{
			switch (MapperNo)
			{
			case 1:
				//MAP1=mymalloc(SRAMIN,sizeof(Mapper1Res));
				MAP1 = malloc(sizeof(Mapper1Res));
				if (!MAP1)res = 1;
				break;
			case 4:
			case 6:
			case 16:
			case 17:
			case 18:
			case 19:
			case 21:
			case 23:
			case 24:
			case 25:
			case 64:
			case 65:
			case 67:
			case 69:
			case 85:
			case 189:
				//MAPx=mymalloc(SRAMIN,sizeof(MapperCommRes));
				MAPx = malloc(sizeof(MapperCommRes));
				if (!MAPx)res = 1;
				break;
			default:
				break;
			}
		}
	}
	return res; //返回执行结果
}

//释放内存
void nes_sram_free(void)
{
	_aligned_free(NES_RAM);
	free(NES_SRAM);
	free(RomHeader);
	free(NES_Mapper);
	free(spr_ram);
	free(ppu);
	free(apu);
	free(wave_buffers);
	free(romfile);
	if ((VROM_tiles != VROM_banks) && VROM_banks && VROM_tiles) //如果分别为VROM_banks和VROM_tiles申请了内存,则释放
	{
		free(VROM_banks);
		free(VROM_tiles);
	}
	switch (MapperNo) //释放map内存
	{
	case 1: //释放内存
		free(MAP1);
		break;
	case 4:
	case 6:
	case 16:
	case 17:
	case 18:
	case 19:
	case 21:
	case 23:
	case 24:
	case 25:
	case 64:
	case 65:
	case 67:
	case 69:
	case 85:
	case 189:
		free(MAPx);
		break; //释放内存
	default: break;
	}
	NES_RAM = NULL;
	NES_SRAM = NULL;
	RomHeader = NULL;
	NES_Mapper = NULL;
	spr_ram = NULL;
	ppu = NULL;
	apu = NULL;
	wave_buffers = NULL;
	romfile = NULL;
	VROM_banks = NULL;
	VROM_tiles = NULL;
	MAP1 = NULL;
	MAPx = NULL;
}

//为NES运行申请内存
//romsize:nes文件大小
//返回值:0,申请成功
//       1,申请失败
uint8_t nes_sram_malloc(uint32_t romsize)
{
	NES_RAM = _aligned_malloc(0x800, 1024); //2KB 对齐到1024边界
	NES_SRAM = malloc(0X2000); //8KB
	RomHeader = malloc(sizeof(NES_header)); //不变
	NES_Mapper = malloc(sizeof(MAPPER)); //不变
	spr_ram = malloc(0X100); //0.25KB
	ppu = malloc(sizeof(ppu_data));
	apu = malloc(sizeof(apu_t)); //sizeof(apu_t)=  12588
	wave_buffers = malloc(APU_PCMBUF_SIZE * 2);
	romfile = malloc(romsize); //申请游戏rom空间,等于nes文件大小， 不变

	LOG("NES_SRAM    :%p\n", NES_SRAM);
	LOG("RomHeader   :%p\n", RomHeader);
	LOG("NES_Mapper  :%p\n", NES_Mapper);
	LOG("spr_ram     :%p\n", spr_ram);
	LOG("ppu         :%p\n", ppu);
	LOG("apu         :%p\n", apu);
	LOG("wave_buffers:%p\n", wave_buffers);
	LOG("romfile     :%p\n", romfile);
	LOG("romsize     :%d\n", romsize);

	while (romfile == NULL) //内存不够?释放主界面占用内存,再重新申请
	{
		LOG("内存不足\r\n");
		// HAL_Delay(2000);
		romfile = malloc(romsize); //重新申请
		LOG("romfile     :%p\n", romfile);
	} //
	if (!NES_RAM || !NES_SRAM || !RomHeader || !NES_Mapper || !spr_ram || !ppu || !apu || !wave_buffers || !romfile)
	{
		nes_sram_free();
		return 1;
	}
	//清零
	memset(NES_SRAM, 0, 0X2000);
	memset(RomHeader, 0, sizeof(NES_header));
	memset(NES_Mapper, 0, sizeof(MAPPER));
	memset(spr_ram, 0, 0X100);
	memset(ppu, 0, sizeof(ppu_data));
	memset(apu, 0, sizeof(apu_t));
	memset(wave_buffers, 0, APU_PCMBUF_SIZE * 2);
	memset(romfile, 0, romsize);
	return 0;
}


//开始nes游戏
//pname:nes游戏路径
//返回值:
//0,正常退出
//1,内存错误
//2,文件错误
//3,不支持的map
uint8_t nes_load(const char* pname)
{
	size_t size = nes_get_size(pname);

	if (size == 0)
	{
		return 6;
	}
	if (nes_sram_malloc(size) != 0)
	{
		LOG("内存分配失败\n");
		return 5; // 内存分配失败
	}
	LOG("内存分配成功\n");
	if (nes_read_rom(pname, romfile) < 0)
	{
		return 7;
	}
	if (nes_load_rom() != 0)
	{
		LOG("加载NES游戏ROM到内存中失败\n");
		return 8; // 加载NES游戏ROM到内存中失败
	}
	LOG("游戏文件加载成功\n");
	cpu6502_init(); //初始化6502,并复位
	Mapper_Init(); //map初始化
	PPU_reset(); //ppu复位
	apu_init(); //apu初始化
	LOG("模拟器初始化成功\n");
	nes_emulate_frame(); //进入NES模拟器主循环
	nes_sram_free(); //释放内存
	return 0;
}


//读取游戏手柄数据
void nes_get_gamepadval(void)
{
	PADdata0 = 0;

	if (GetAsyncKeyState('W') & 0x8000)
	{
		PADdata0 |= 0x10; // UP
	}
	if (GetAsyncKeyState('S') & 0x8000)
	{
		PADdata0 |= 0x20; // DOWN
	}
	if (GetAsyncKeyState('A') & 0x8000)
	{
		PADdata0 |= 0x40; // LEFT
	}
	if (GetAsyncKeyState('D') & 0x8000)
	{
		PADdata0 |= 0x80; // RIGHT
	}
	if (GetAsyncKeyState(VK_LEFT) & 0x8000)
	{
		PADdata0 |= 0x01; // A
	}
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
	{
		PADdata0 |= 0x02; // B
	}
	if (GetAsyncKeyState(VK_SPACE) & 0x8000)
	{
		PADdata0 |= 0x08; // Start
	}
	if (GetAsyncKeyState('H') & 0x8000)
	{
		PADdata0 |= 0x04; // Select
	}
}

//nes模拟器主循环
void nes_emulate_frame(void)
{
	uint8_t nes_frame = 0;
	while (1)
	{
		// Sleep(1);
		// LINES 0-239
		PPU_start_frame();
		for (NES_scanline = 0; NES_scanline < 240; NES_scanline++)
		{
			run6502(113);
			NES_Mapper->HSync(NES_scanline);
			//扫描一行
			if (nes_frame == 0)
				scanline_draw(NES_scanline);
			else
				do_scanline_and_dont_draw(NES_scanline);
		}
		NES_scanline = 240;
		nes_get_gamepadval();
		run6502(113); //运行1线
		NES_Mapper->HSync(NES_scanline);
		start_vblank();
		if (NMI_enabled())
		{
			cpunmi = 1;
			run6502(7); //运行中断
		}
		NES_Mapper->VSync();
		// LINES 242-261
		for (NES_scanline = 241; NES_scanline < 262; NES_scanline++)
		{
			run6502(113);
			NES_Mapper->HSync(NES_scanline);
		}
		end_vblank();
		apu_soundoutput(); //输出游戏声音
		nes_frame_cnt++;
		nes_frame++;
		if (nes_frame > NES_SKIP_FRAME)nes_frame = 0; //跳帧
		if (system_task_return) break; //TPAD返回
	}
}

//在6502.s里面被调用
void debug_6502(uint16_t reg0, uint8_t reg1)
{
	LOG("6502 error:%x,%d\r\n", reg0, reg1);
}
