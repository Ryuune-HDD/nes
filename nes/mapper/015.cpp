#include "nes_mapper.h"


// Mapper 15
void MAP15_Reset()
{
    //  // set CPU bank pointers
    //  set_CPU_banks(0,1,2,3);
}

void MAP15_MemoryWrite(uint16_t addr, uint8_t data)
{
    switch (addr)
    {
    case 0x8000:
        {
            if (data & 0x80)
            {
                set_CPU_bank4((data & 0x3F) * 2 + 1);
                set_CPU_bank5((data & 0x3F) * 2 + 0);
                set_CPU_bank6((data & 0x3F) * 2 + 3);
                set_CPU_bank7((data & 0x3F) * 2 + 2);
            }
            else
            {
                set_CPU_bank4((data & 0x3F) * 2 + 0);
                set_CPU_bank5((data & 0x3F) * 2 + 1);
                set_CPU_bank6((data & 0x3F) * 2 + 2);
                set_CPU_bank7((data & 0x3F) * 2 + 3);
            }
            if (data & 0x40)
            {
                set_mirroring(0, 0, 1, 1); //ˮƽ����InfoNES_Mirroring( 0 );   //0011
                //set_mirroring(NES_PPU::MIRROR_HORIZ);
            }
            else
            {
                set_mirroring(0, 1, 0, 1); //��ֱ����InfoNES_Mirroring( 1 );   //0101
                //set_mirroring(NES_PPU::MIRROR_VERT);
            }
        }
        break;

    case 0x8001:
        {
            if (data & 0x80)
            {
                set_CPU_bank6((data & 0x3F) * 2 + 1);
                set_CPU_bank7((data & 0x3F) * 2 + 0);
            }
            else
            {
                set_CPU_bank6((data & 0x3F) * 2 + 0);
                set_CPU_bank7((data & 0x3F) * 2 + 1);
            }
        }
        break;

    case 0x8002:
        {
            if (data & 0x80)
            {
                set_CPU_bank4((data & 0x3F) * 2 + 1);
                set_CPU_bank5((data & 0x3F) * 2 + 1);
                set_CPU_bank6((data & 0x3F) * 2 + 1);
                set_CPU_bank7((data & 0x3F) * 2 + 1);
            }
            else
            {
                set_CPU_bank4((data & 0x3F) * 2);
                set_CPU_bank5((data & 0x3F) * 2);
                set_CPU_bank6((data & 0x3F) * 2);
                set_CPU_bank7((data & 0x3F) * 2);
            }
        }
        break;

    case 0x8003:
        {
            if (data & 0x80)
            {
                set_CPU_bank6((data & 0x3F) * 2 + 1);
                set_CPU_bank7((data & 0x3F) * 2 + 0);
            }
            else
            {
                set_CPU_bank6((data & 0x3F) * 2 + 0);
                set_CPU_bank7((data & 0x3F) * 2 + 1);
            }
            if (data & 0x40)
            {
                set_mirroring(0, 0, 1, 1); //ˮƽ����InfoNES_Mirroring( 0 );   //0011
                //set_mirroring(NES_PPU::MIRROR_HORIZ);
            }
            else
            {
                set_mirroring(0, 1, 0, 1); //��ֱ����InfoNES_Mirroring( 1 );   //0101
                //set_mirroring(NES_PPU::MIRROR_VERT);
            }
        }
        break;
    }
}

void MAP15_Init()
{
    NES_Mapper->Reset = MAP15_Reset;
    NES_Mapper->Write = MAP15_MemoryWrite;
}