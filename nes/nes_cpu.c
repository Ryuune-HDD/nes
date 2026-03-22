#include "nes_cpu.h"
#include "nes_main.h"
#include <string.h>

static uint8_t* aaaaaa;
static uint8_t* CPU_ROM_banks[4];

static uint8_t joy0_shift; // 手柄1移位寄存器
static uint8_t joy1_shift; // 手柄2移位寄存器

static uint8_t A, X, Y, S, P;
static uint16_t PC;

static uint8_t temp8;
static uint16_t temp16;

#define PAGE_CROSS(a, b) ((((a) ^ (b)) & 0x100) != 0)
#define READ_WORD(addr) (K6502_Read(addr) | (K6502_Read((addr) + 1) << 8))

#define setZ(v)    (P = (P & ~FLAG_Z) | ((v) == 0 ? FLAG_Z : 0))
#define setN(v)    (P = (P & ~FLAG_N) | ((v) & 0x80))
#define setNZ(v)   do { setZ(v); setN(v); } while(0)

static const uint8_t cycles_map[256] = {
    7, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6,
    2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 6, 7,
    6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6,
    2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 6, 7,
    6, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6,
    2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 6, 7,
    6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6,
    2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 6, 7,
    2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
    2, 6, 2, 6, 4, 4, 4, 4, 2, 5, 2, 5, 5, 5, 5, 5,
    2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
    2, 5, 2, 5, 4, 4, 4, 4, 2, 2, 2, 2, 4, 4, 4, 4,
    2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
    2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 6, 7,
    2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
    2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 6, 7
};

typedef void (*OpFunc)(void);

static OpFunc op_handlers[256];

uint8_t K6502_Read(uint16_t addr)
{
    if (addr < 0x2000)
    {
        return NES_RAM[addr & 0x07FF];
    }
    else if (addr < 0x4000)
    {
        return PPU_ReadFromPort(addr & 7);
    }
    else if (addr < 0x4020)
    {
        switch (addr & 0x1F)
        {
        case 0x15: return Apu_Read4015(addr);
        case 0x16:
            {
                uint8_t result = (joy0_shift & 1) | 0x60;
                joy0_shift >>= 1;
                return result;
            }
        case 0x17:
            {
                uint8_t result = (joy1_shift & 1) | 0x60;
                joy1_shift >>= 1;
                return result;
            }
        default: return 0;
        }
    }
    else if (addr >= 0x5000 && addr < 0x6000)
    {
        return asm_Mapper_ReadLow(addr);
    }
    else if (addr < 0x8000)
    {
        return NES_SRAM[addr - 0x6000];
    }
    else if (addr < 0xA000)
    {
        return CPU_ROM_banks[0] ? CPU_ROM_banks[0][addr & 0x1FFF] : 0;
    }
    else if (addr < 0xC000)
    {
        return CPU_ROM_banks[1] ? CPU_ROM_banks[1][addr & 0x1FFF] : 0;
    }
    else if (addr < 0xE000)
    {
        return CPU_ROM_banks[2] ? CPU_ROM_banks[2][addr & 0x1FFF] : 0;
    }
    else
    {
        return CPU_ROM_banks[3] ? CPU_ROM_banks[3][addr & 0x1FFF] : 0;
    }
}

void K6502_Write(uint16_t addr, uint8_t val)
{
    if (addr < 0x2000)
    {
        NES_RAM[addr & 0x07FF] = val;
    }
    else if (addr < 0x4000)
    {
        PPU_WriteToPort(val, addr & 7);
    }
    else if (addr < 0x4020)
    {
        switch (addr & 0x1F)
        {
        case 0x14:
            {
                uint16_t src_addr = (uint16_t)val << 8;
                for (int i = 0; i < 256; i++)
                {
                    spr_ram[i] = K6502_Read(src_addr + i);
                }
                clocks += 514;
                break;
            }
        case 0x15: Apu_Write4015(val, addr);
            break;
        case 0x16:
            if ((val & 1) == 0)
            {
                joy0_shift = PADdata0;
                joy1_shift = PADdata1;
            }
            break;
        case 0x17: Apu_Write4017(val, addr);
            break;
        default: Apu_Write(val, addr);
            break;
        }
    }
    else if (addr >= 0x5000 && addr < 0x6000)
    {
        asm_Mapper_WriteLow(val, addr);
    }
    else if (addr < 0x8000)
    {
        NES_SRAM[addr - 0x6000] = val;
    }
    else
    {
        asm_Mapper_Write(val, addr);
    }
}

static inline void push(uint8_t val)
{
    K6502_Write(0x100 + S, val);
    S = (S - 1) & 0xFF;
}

static inline uint8_t pop(void)
{
    S = (S + 1) & 0xFF;
    return K6502_Read(0x100 + S);
}

static inline void branch(uint8_t cond)
{
    if (cond)
    {
        int8_t offset = (int8_t)K6502_Read(PC++);
        uint16_t old_pc = PC;
        PC = (PC + offset) & 0xFFFF;
        clocks += ((old_pc ^ PC) & 0x100) ? 2 : 1;
    }
    else
    {
        PC++;
    }
}

static void ADC(uint8_t val)
{
    uint16_t sum = A + val + (P & FLAG_C);
    P = (P & ~(FLAG_V | FLAG_C)) | (((~(A ^ val) & (A ^ sum)) & 0x80) ? FLAG_V : 0) | ((sum > 0xFF) ? FLAG_C : 0);
    A = sum & 0xFF;
    setNZ(A);
}

static void SBC(uint8_t val)
{
    ADC(~val);
}

static void CMP(uint8_t a, uint8_t b)
{
    temp8 = a - b;
    P = (P & ~FLAG_C) | (a >= b ? FLAG_C : 0);
    setNZ(temp8);
}

static void ASL_ACC(void)
{
    P = (P & ~FLAG_C) | ((A & 0x80) ? FLAG_C : 0);
    A <<= 1;
    setNZ(A);
}

static void LSR_ACC(void)
{
    P = (P & ~FLAG_C) | (A & 1);
    A >>= 1;
    setNZ(A);
}

static void ROL_ACC(void)
{
    temp8 = (P & FLAG_C) ? 1 : 0;
    P = (P & ~FLAG_C) | ((A & 0x80) ? FLAG_C : 0);
    A = (A << 1) | temp8;
    setNZ(A);
}

static void ROR_ACC(void)
{
    temp8 = (P & FLAG_C) ? 0x80 : 0;
    P = (P & ~FLAG_C) | (A & 1);
    A = (A >> 1) | temp8;
    setNZ(A);
}

static void op_BRK(void)
{
    PC++;
    push((PC >> 8) & 0xFF);
    push(PC & 0xFF);
    push(P | FLAG_B | FLAG_R);
    P |= FLAG_I;
    PC = READ_WORD(IRQ_VECTOR);
}

static void op_ORA_IZX(void)
{
    temp16 = (K6502_Read(PC++) + X) & 0xFF;
    temp16 = K6502_Read(temp16) | (K6502_Read((temp16 + 1) & 0xFF) << 8);
    A |= K6502_Read(temp16);
    setNZ(A);
}

static void op_ORA_IZY(void)
{
    temp16 = K6502_Read(PC++);
    temp16 = K6502_Read(temp16) | (K6502_Read((temp16 + 1) & 0xFF) << 8);
    uint16_t old = temp16;
    temp16 = (temp16 + Y) & 0xFFFF;
    if (PAGE_CROSS(old, temp16)) clocks++;
    A |= K6502_Read(temp16);
    setNZ(A);
}

static void op_AND_IZX(void)
{
    temp16 = (K6502_Read(PC++) + X) & 0xFF;
    temp16 = K6502_Read(temp16) | (K6502_Read((temp16 + 1) & 0xFF) << 8);
    A &= K6502_Read(temp16);
    setNZ(A);
}

static void op_AND_IZY(void)
{
    temp16 = K6502_Read(PC++);
    temp16 = K6502_Read(temp16) | (K6502_Read((temp16 + 1) & 0xFF) << 8);
    uint16_t old = temp16;
    temp16 = (temp16 + Y) & 0xFFFF;
    if (PAGE_CROSS(old, temp16)) clocks++;
    A &= K6502_Read(temp16);
    setNZ(A);
}

static void op_EOR_IZX(void)
{
    temp16 = (K6502_Read(PC++) + X) & 0xFF;
    temp16 = K6502_Read(temp16) | (K6502_Read((temp16 + 1) & 0xFF) << 8);
    A ^= K6502_Read(temp16);
    setNZ(A);
}

static void op_EOR_IZY(void)
{
    temp16 = K6502_Read(PC++);
    temp16 = K6502_Read(temp16) | (K6502_Read((temp16 + 1) & 0xFF) << 8);
    uint16_t old = temp16;
    temp16 = (temp16 + Y) & 0xFFFF;
    if (PAGE_CROSS(old, temp16)) clocks++;
    A ^= K6502_Read(temp16);
    setNZ(A);
}

static void op_ADC_IZX(void)
{
    temp16 = (K6502_Read(PC++) + X) & 0xFF;
    temp16 = K6502_Read(temp16) | (K6502_Read((temp16 + 1) & 0xFF) << 8);
    ADC(K6502_Read(temp16));
}

static void op_ADC_IZY(void)
{
    temp16 = K6502_Read(PC++);
    temp16 = K6502_Read(temp16) | (K6502_Read((temp16 + 1) & 0xFF) << 8);
    uint16_t old = temp16;
    temp16 = (temp16 + Y) & 0xFFFF;
    if (PAGE_CROSS(old, temp16)) clocks++;
    ADC(K6502_Read(temp16));
}

static void op_STA_IZX(void)
{
    temp16 = (K6502_Read(PC++) + X) & 0xFF;
    temp16 = K6502_Read(temp16) | (K6502_Read((temp16 + 1) & 0xFF) << 8);
    K6502_Write(temp16, A);
}

static void op_STA_IZY(void)
{
    temp16 = K6502_Read(PC++);
    temp16 = K6502_Read(temp16) | (K6502_Read((temp16 + 1) & 0xFF) << 8);
    temp16 = (temp16 + Y) & 0xFFFF;
    K6502_Write(temp16, A);
}

static void op_LDA_IMM(void)
{
    A = K6502_Read(PC++);
    setNZ(A);
}

static void op_LDA_ZP(void)
{
    A = K6502_Read(K6502_Read(PC++));
    setNZ(A);
}

static void op_LDA_ZPX(void)
{
    A = K6502_Read((K6502_Read(PC++) + X) & 0xFF);
    setNZ(A);
}

static void op_LDA_IZX(void)
{
    temp16 = (K6502_Read(PC++) + X) & 0xFF;
    temp16 = K6502_Read(temp16) | (K6502_Read((temp16 + 1) & 0xFF) << 8);
    A = K6502_Read(temp16);
    setNZ(A);
}

static void op_LDA_IZY(void)
{
    temp16 = K6502_Read(PC++);
    temp16 = K6502_Read(temp16) | (K6502_Read((temp16 + 1) & 0xFF) << 8);
    uint16_t old = temp16;
    temp16 = (temp16 + Y) & 0xFFFF;
    if (PAGE_CROSS(old, temp16)) clocks++;
    A = K6502_Read(temp16);
    setNZ(A);
}

static void op_LDA_ABS(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    A = K6502_Read(temp16);
    setNZ(A);
}

static void op_LDA_ABX(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    uint16_t old = temp16;
    temp16 = (temp16 + X) & 0xFFFF;
    if (PAGE_CROSS(old, temp16)) clocks++;
    A = K6502_Read(temp16);
    setNZ(A);
}

static void op_LDA_ABY(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    uint16_t old = temp16;
    temp16 = (temp16 + Y) & 0xFFFF;
    if (PAGE_CROSS(old, temp16)) clocks++;
    A = K6502_Read(temp16);
    setNZ(A);
}

static void op_LDX_IMM(void)
{
    X = K6502_Read(PC++);
    setNZ(X);
}

static void op_LDX_ZP(void)
{
    X = K6502_Read(K6502_Read(PC++));
    setNZ(X);
}

static void op_LDX_ZPY(void)
{
    X = K6502_Read((K6502_Read(PC++) + Y) & 0xFF);
    setNZ(X);
}

static void op_LDX_ABS(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    X = K6502_Read(temp16);
    setNZ(X);
}

static void op_LDX_ABY(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    uint16_t old = temp16;
    temp16 = (temp16 + Y) & 0xFFFF;
    if (PAGE_CROSS(old, temp16)) clocks++;
    X = K6502_Read(temp16);
    setNZ(X);
}

static void op_LDY_IMM(void)
{
    Y = K6502_Read(PC++);
    setNZ(Y);
}

static void op_LDY_ZP(void)
{
    Y = K6502_Read(K6502_Read(PC++));
    setNZ(Y);
}

static void op_LDY_ZPX(void)
{
    Y = K6502_Read((K6502_Read(PC++) + X) & 0xFF);
    setNZ(Y);
}

static void op_LDY_ABS(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    Y = K6502_Read(temp16);
    setNZ(Y);
}

static void op_LDY_ABX(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    uint16_t old = temp16;
    temp16 = (temp16 + X) & 0xFFFF;
    if (PAGE_CROSS(old, temp16)) clocks++;
    Y = K6502_Read(temp16);
    setNZ(Y);
}

static void op_STA_ABS(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    K6502_Write(temp16, A);
}

static void op_STA_ABX(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    K6502_Write((temp16 + X) & 0xFFFF, A);
}

static void op_STA_ABY(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    K6502_Write((temp16 + Y) & 0xFFFF, A);
}

static void op_STX_ABS(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    K6502_Write(temp16, X);
}

static void op_STY_ABS(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    K6502_Write(temp16, Y);
}

static void op_JSR(void)
{
    temp16 = READ_WORD(PC);
    PC++;
    push((PC >> 8) & 0xFF);
    push(PC & 0xFF);
    PC = temp16;
}

static void op_RTS(void)
{
    temp16 = pop();
    temp16 |= pop() << 8;
    PC = temp16 + 1;
}

static void op_RTI(void)
{
    P = pop();
    temp16 = pop();
    temp16 |= pop() << 8;
    PC = temp16;
}

static void op_JMP_ABS(void)
{
    PC = READ_WORD(PC);
}

static void op_BPL(void) { branch(!(P & FLAG_N)); }
static void op_BMI(void) { branch(P & FLAG_N); }
static void op_BVC(void) { branch(!(P & FLAG_V)); }
static void op_BVS(void) { branch(P & FLAG_V); }
static void op_BCC(void) { branch(!(P & FLAG_C)); }
static void op_BCS(void) { branch(P & FLAG_C); }
static void op_BNE(void) { branch(!(P & FLAG_Z)); }
static void op_BEQ(void) { branch(P & FLAG_Z); }

static void op_PHP(void)
{
    push(P | FLAG_B | FLAG_R);
}

static void op_PLP(void)
{
    P = pop() | FLAG_R | FLAG_B;
}

static void op_PHA(void)
{
    push(A);
}

static void op_PLA(void)
{
    A = pop();
    setNZ(A);
}

static void op_TAX(void)
{
    X = A;
    setNZ(X);
}

static void op_TXA(void)
{
    A = X;
    setNZ(A);
}

static void op_TAY(void)
{
    Y = A;
    setNZ(Y);
}

static void op_TYA(void)
{
    A = Y;
    setNZ(A);
}

static void op_TSX(void)
{
    X = S;
    setNZ(X);
}

static void op_TXS(void)
{
    S = X;
}

static void op_INX(void)
{
    X = (X + 1) & 0xFF;
    setNZ(X);
}

static void op_DEX(void)
{
    X = (X - 1) & 0xFF;
    setNZ(X);
}

static void op_INY(void)
{
    Y = (Y + 1) & 0xFF;
    setNZ(Y);
}

static void op_DEY(void)
{
    Y = (Y - 1) & 0xFF;
    setNZ(Y);
}

static void op_CLC(void) { P &= ~FLAG_C; }
static void op_SEC(void) { P |= FLAG_C; }
static void op_CLI(void) { P &= ~FLAG_I; }
static void op_SEI(void) { P |= FLAG_I; }
static void op_CLV(void) { P &= ~FLAG_V; }
static void op_CLD(void) { P &= ~FLAG_D; }
static void op_SED(void) { P |= FLAG_D; }

static void op_NOP(void)
{
}

static void op_BIT_ZP(void)
{
    temp8 = K6502_Read(K6502_Read(PC++));
    P = (P & ~(FLAG_N | FLAG_V | FLAG_Z)) | (temp8 & (FLAG_N | FLAG_V)) | ((A & temp8) == 0 ? FLAG_Z : 0);
}

static void op_BIT_ABS(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    temp8 = K6502_Read(temp16);
    P = (P & ~(FLAG_N | FLAG_V | FLAG_Z)) | (temp8 & (FLAG_N | FLAG_V)) | ((A & temp8) == 0 ? FLAG_Z : 0);
}

static void op_ASL_ZP(void)
{
    temp16 = K6502_Read(PC++);
    temp8 = K6502_Read(temp16);
    P = (P & ~FLAG_C) | ((temp8 & 0x80) ? FLAG_C : 0);
    temp8 <<= 1;
    K6502_Write(temp16, temp8);
    setNZ(temp8);
}

static void op_ASL_ZPX(void)
{
    temp16 = (K6502_Read(PC++) + X) & 0xFF;
    temp8 = K6502_Read(temp16);
    P = (P & ~FLAG_C) | ((temp8 & 0x80) ? FLAG_C : 0);
    temp8 <<= 1;
    K6502_Write(temp16, temp8);
    setNZ(temp8);
}

static void op_ASL_ABS(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    temp8 = K6502_Read(temp16);
    P = (P & ~FLAG_C) | ((temp8 & 0x80) ? FLAG_C : 0);
    temp8 <<= 1;
    K6502_Write(temp16, temp8);
    setNZ(temp8);
}

static void op_ASL_ABX(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    temp16 = (temp16 + X) & 0xFFFF;
    temp8 = K6502_Read(temp16);
    P = (P & ~FLAG_C) | ((temp8 & 0x80) ? FLAG_C : 0);
    temp8 <<= 1;
    K6502_Write(temp16, temp8);
    setNZ(temp8);
}

static void op_LSR_ZP(void)
{
    temp16 = K6502_Read(PC++);
    temp8 = K6502_Read(temp16);
    P = (P & ~FLAG_C) | (temp8 & 1);
    temp8 >>= 1;
    K6502_Write(temp16, temp8);
    setNZ(temp8);
}

static void op_LSR_ZPX(void)
{
    temp16 = (K6502_Read(PC++) + X) & 0xFF;
    temp8 = K6502_Read(temp16);
    P = (P & ~FLAG_C) | (temp8 & 1);
    temp8 >>= 1;
    K6502_Write(temp16, temp8);
    setNZ(temp8);
}

static void op_LSR_ABS(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    temp8 = K6502_Read(temp16);
    P = (P & ~FLAG_C) | (temp8 & 1);
    temp8 >>= 1;
    K6502_Write(temp16, temp8);
    setNZ(temp8);
}

static void op_LSR_ABX(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    temp16 = (temp16 + X) & 0xFFFF;
    temp8 = K6502_Read(temp16);
    P = (P & ~FLAG_C) | (temp8 & 1);
    temp8 >>= 1;
    K6502_Write(temp16, temp8);
    setNZ(temp8);
}

static void op_ROL_ZP(void)
{
    temp16 = K6502_Read(PC++);
    temp8 = K6502_Read(temp16);
    temp8 = (temp8 << 1) | (P & FLAG_C ? 1 : 0);
    P = (P & ~FLAG_C) | ((temp8 & 0x100) ? FLAG_C : 0);
    temp8 &= 0xFF;
    K6502_Write(temp16, temp8);
    setNZ(temp8);
}

static void op_ROL_ZPX(void)
{
    temp16 = (K6502_Read(PC++) + X) & 0xFF;
    temp8 = K6502_Read(temp16);
    temp8 = (temp8 << 1) | (P & FLAG_C ? 1 : 0);
    P = (P & ~FLAG_C) | ((temp8 & 0x100) ? FLAG_C : 0);
    temp8 &= 0xFF;
    K6502_Write(temp16, temp8);
    setNZ(temp8);
}

static void op_ROL_ABS(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    temp8 = K6502_Read(temp16);
    temp8 = (temp8 << 1) | (P & FLAG_C ? 1 : 0);
    P = (P & ~FLAG_C) | ((temp8 & 0x100) ? FLAG_C : 0);
    temp8 &= 0xFF;
    K6502_Write(temp16, temp8);
    setNZ(temp8);
}

static void op_ROL_ABX(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    temp16 = (temp16 + X) & 0xFFFF;
    temp8 = K6502_Read(temp16);
    temp8 = (temp8 << 1) | (P & FLAG_C ? 1 : 0);
    P = (P & ~FLAG_C) | ((temp8 & 0x100) ? FLAG_C : 0);
    temp8 &= 0xFF;
    K6502_Write(temp16, temp8);
    setNZ(temp8);
}

static void op_ROR_ZP(void)
{
    temp16 = K6502_Read(PC++);
    temp8 = K6502_Read(temp16);
    temp8 = (temp8 >> 1) | (P & FLAG_C ? 0x80 : 0);
    P = (P & ~FLAG_C) | (temp8 & 1);
    K6502_Write(temp16, temp8);
    setNZ(temp8);
}

static void op_ROR_ZPX(void)
{
    temp16 = (K6502_Read(PC++) + X) & 0xFF;
    temp8 = K6502_Read(temp16);
    temp8 = (temp8 >> 1) | (P & FLAG_C ? 0x80 : 0);
    P = (P & ~FLAG_C) | (temp8 & 1);
    K6502_Write(temp16, temp8);
    setNZ(temp8);
}

static void op_ROR_ABS(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    temp8 = K6502_Read(temp16);
    temp8 = (temp8 >> 1) | (P & FLAG_C ? 0x80 : 0);
    P = (P & ~FLAG_C) | (temp8 & 1);
    K6502_Write(temp16, temp8);
    setNZ(temp8);
}

static void op_ROR_ABX(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    temp16 = (temp16 + X) & 0xFFFF;
    temp8 = K6502_Read(temp16);
    temp8 = (temp8 >> 1) | (P & FLAG_C ? 0x80 : 0);
    P = (P & ~FLAG_C) | (temp8 & 1);
    K6502_Write(temp16, temp8);
    setNZ(temp8);
}

static void op_STA_ZP(void)
{
    K6502_Write(K6502_Read(PC++), A);
}

static void op_STA_ZPX(void)
{
    K6502_Write((K6502_Read(PC++) + X) & 0xFF, A);
}

static void op_STX_ZP(void)
{
    K6502_Write(K6502_Read(PC++), X);
}

static void op_STX_ZPY(void)
{
    K6502_Write((K6502_Read(PC++) + Y) & 0xFF, X);
}

static void op_STY_ZP(void)
{
    K6502_Write(K6502_Read(PC++), Y);
}

static void op_STY_ZPX(void)
{
    K6502_Write((K6502_Read(PC++) + X) & 0xFF, Y);
}

static void op_DEC_ZP(void)
{
    temp16 = K6502_Read(PC++);
    temp8 = (K6502_Read(temp16) - 1) & 0xFF;
    K6502_Write(temp16, temp8);
    setNZ(temp8);
}

static void op_DEC_ZPX(void)
{
    temp16 = (K6502_Read(PC++) + X) & 0xFF;
    temp8 = (K6502_Read(temp16) - 1) & 0xFF;
    K6502_Write(temp16, temp8);
    setNZ(temp8);
}

static void op_DEC_ABS(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    temp8 = (K6502_Read(temp16) - 1) & 0xFF;
    K6502_Write(temp16, temp8);
    setNZ(temp8);
}

static void op_DEC_ABX(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    temp16 = (temp16 + X) & 0xFFFF;
    temp8 = (K6502_Read(temp16) - 1) & 0xFF;
    K6502_Write(temp16, temp8);
    setNZ(temp8);
}

static void op_INC_ZP(void)
{
    temp16 = K6502_Read(PC++);
    temp8 = (K6502_Read(temp16) + 1) & 0xFF;
    K6502_Write(temp16, temp8);
    setNZ(temp8);
}

static void op_INC_ZPX(void)
{
    temp16 = (K6502_Read(PC++) + X) & 0xFF;
    temp8 = (K6502_Read(temp16) + 1) & 0xFF;
    K6502_Write(temp16, temp8);
    setNZ(temp8);
}

static void op_INC_ABS(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    temp8 = (K6502_Read(temp16) + 1) & 0xFF;
    K6502_Write(temp16, temp8);
    setNZ(temp8);
}

static void op_INC_ABX(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    temp16 = (temp16 + X) & 0xFFFF;
    temp8 = (K6502_Read(temp16) + 1) & 0xFF;
    K6502_Write(temp16, temp8);
    setNZ(temp8);
}

static void op_CMP_IMM(void)
{
    CMP(A, K6502_Read(PC++));
}

static void op_CMP_ZP(void)
{
    CMP(A, K6502_Read(K6502_Read(PC++)));
}

static void op_CMP_ZPX(void)
{
    CMP(A, K6502_Read((K6502_Read(PC++) + X) & 0xFF));
}

static void op_CMP_ABS(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    CMP(A, K6502_Read(temp16));
}

static void op_CMP_ABX(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    uint16_t old = temp16;
    temp16 = (temp16 + X) & 0xFFFF;
    if (PAGE_CROSS(old, temp16)) clocks++;
    CMP(A, K6502_Read(temp16));
}

static void op_CMP_ABY(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    uint16_t old = temp16;
    temp16 = (temp16 + Y) & 0xFFFF;
    if (PAGE_CROSS(old, temp16)) clocks++;
    CMP(A, K6502_Read(temp16));
}

static void op_CPX_IMM(void)
{
    CMP(X, K6502_Read(PC++));
}

static void op_CPX_ZP(void)
{
    CMP(X, K6502_Read(K6502_Read(PC++)));
}

static void op_CPX_ABS(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    CMP(X, K6502_Read(temp16));
}

static void op_CPY_IMM(void)
{
    CMP(Y, K6502_Read(PC++));
}

static void op_CPY_ZP(void)
{
    CMP(Y, K6502_Read(K6502_Read(PC++)));
}

static void op_CPY_ABS(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    CMP(Y, K6502_Read(temp16));
}

static void op_ORA_ZP(void)
{
    A |= K6502_Read(K6502_Read(PC++));
    setNZ(A);
}

static void op_ORA_ZPX(void)
{
    A |= K6502_Read((K6502_Read(PC++) + X) & 0xFF);
    setNZ(A);
}

static void op_ORA_ABS(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    A |= K6502_Read(temp16);
    setNZ(A);
}

static void op_ORA_ABX(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    uint16_t old = temp16;
    temp16 = (temp16 + X) & 0xFFFF;
    if (PAGE_CROSS(old, temp16)) clocks++;
    A |= K6502_Read(temp16);
    setNZ(A);
}

static void op_ORA_ABY(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    uint16_t old = temp16;
    temp16 = (temp16 + Y) & 0xFFFF;
    if (PAGE_CROSS(old, temp16)) clocks++;
    A |= K6502_Read(temp16);
    setNZ(A);
}

static void op_AND_ZP(void)
{
    A &= K6502_Read(K6502_Read(PC++));
    setNZ(A);
}

static void op_AND_ZPX(void)
{
    A &= K6502_Read((K6502_Read(PC++) + X) & 0xFF);
    setNZ(A);
}

static void op_AND_ABS(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    A &= K6502_Read(temp16);
    setNZ(A);
}

static void op_AND_ABX(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    uint16_t old = temp16;
    temp16 = (temp16 + X) & 0xFFFF;
    if (PAGE_CROSS(old, temp16)) clocks++;
    A &= K6502_Read(temp16);
    setNZ(A);
}

static void op_AND_ABY(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    uint16_t old = temp16;
    temp16 = (temp16 + Y) & 0xFFFF;
    if (PAGE_CROSS(old, temp16)) clocks++;
    A &= K6502_Read(temp16);
    setNZ(A);
}

static void op_EOR_ZP(void)
{
    A ^= K6502_Read(K6502_Read(PC++));
    setNZ(A);
}

static void op_EOR_ZPX(void)
{
    A ^= K6502_Read((K6502_Read(PC++) + X) & 0xFF);
    setNZ(A);
}

static void op_EOR_ABS(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    A ^= K6502_Read(temp16);
    setNZ(A);
}

static void op_EOR_ABX(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    uint16_t old = temp16;
    temp16 = (temp16 + X) & 0xFFFF;
    if (PAGE_CROSS(old, temp16)) clocks++;
    A ^= K6502_Read(temp16);
    setNZ(A);
}

static void op_EOR_ABY(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    uint16_t old = temp16;
    temp16 = (temp16 + Y) & 0xFFFF;
    if (PAGE_CROSS(old, temp16)) clocks++;
    A ^= K6502_Read(temp16);
    setNZ(A);
}

static void op_ADC_ZP(void)
{
    ADC(K6502_Read(K6502_Read(PC++)));
}

static void op_ADC_ZPX(void)
{
    ADC(K6502_Read((K6502_Read(PC++) + X) & 0xFF));
}

static void op_ADC_ABS(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    ADC(K6502_Read(temp16));
}

static void op_ADC_ABX(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    uint16_t old = temp16;
    temp16 = (temp16 + X) & 0xFFFF;
    if (PAGE_CROSS(old, temp16)) clocks++;
    ADC(K6502_Read(temp16));
}

static void op_ADC_ABY(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    uint16_t old = temp16;
    temp16 = (temp16 + Y) & 0xFFFF;
    if (PAGE_CROSS(old, temp16)) clocks++;
    ADC(K6502_Read(temp16));
}

static void op_SBC_ZP(void)
{
    SBC(K6502_Read(K6502_Read(PC++)));
}

static void op_SBC_ZPX(void)
{
    SBC(K6502_Read((K6502_Read(PC++) + X) & 0xFF));
}

static void op_SBC_ABS(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    SBC(K6502_Read(temp16));
}

static void op_SBC_ABX(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    uint16_t old = temp16;
    temp16 = (temp16 + X) & 0xFFFF;
    if (PAGE_CROSS(old, temp16)) clocks++;
    SBC(K6502_Read(temp16));
}

static void op_SBC_ABY(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    uint16_t old = temp16;
    temp16 = (temp16 + Y) & 0xFFFF;
    if (PAGE_CROSS(old, temp16)) clocks++;
    SBC(K6502_Read(temp16));
}

static void op_SBC_IMM(void)
{
    SBC(K6502_Read(PC++));
}

static void op_LAX_ZP(void)
{
    A = X = K6502_Read(K6502_Read(PC++));
    setNZ(A);
}

static void op_LAX_ZPY(void)
{
    temp16 = (K6502_Read(PC++) + Y) & 0xFF;
    temp8 = K6502_Read(temp16);
    A = X = temp8;
    setNZ(A);
}

static void op_LAX_ABS(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    A = X = K6502_Read(temp16);
    setNZ(A);
}

static void op_LAX_IZX(void)
{
    temp16 = (K6502_Read(PC++) + X) & 0xFF;
    temp16 = K6502_Read(temp16) | (K6502_Read((temp16 + 1) & 0xFF) << 8);
    A = X = K6502_Read(temp16);
    setNZ(A);
}

static void op_LAX_IZY(void)
{
    temp16 = K6502_Read(PC++);
    temp16 = K6502_Read(temp16) | (K6502_Read((temp16 + 1) & 0xFF) << 8);
    uint16_t old = temp16;
    temp16 = (temp16 + Y) & 0xFFFF;
    if (PAGE_CROSS(old, temp16)) clocks++;
    A = X = K6502_Read(temp16);
    setNZ(A);
}

static void op_LAX_ABY(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    uint16_t old = temp16;
    temp16 = (temp16 + Y) & 0xFFFF;
    if (PAGE_CROSS(old, temp16)) clocks++;
    A = X = K6502_Read(temp16);
    setNZ(A);
}

static void op_SAX_ZP(void)
{
    temp8 = A & X;
    K6502_Write(K6502_Read(PC++), temp8);
}

static void op_SAX_ZPY(void)
{
    temp8 = A & X;
    K6502_Write((K6502_Read(PC++) + Y) & 0xFF, temp8);
}

static void op_SAX_ABS(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    temp8 = A & X;
    K6502_Write(temp16, temp8);
}

static void op_SAX_IZX(void)
{
    temp16 = (K6502_Read(PC++) + X) & 0xFF;
    temp16 = K6502_Read(temp16) | (K6502_Read((temp16 + 1) & 0xFF) << 8);
    temp8 = A & X;
    K6502_Write(temp16, temp8);
}

static void op_DCP_ZP(void)
{
    temp16 = K6502_Read(PC++);
    temp8 = (K6502_Read(temp16) - 1) & 0xFF;
    K6502_Write(temp16, temp8);
    temp8 = A - temp8;
    P = (P & ~FLAG_C) | (A >= temp8 ? FLAG_C : 0);
    setNZ(temp8);
}

static void op_DCP_ZPX(void)
{
    temp16 = (K6502_Read(PC++) + X) & 0xFF;
    temp8 = (K6502_Read(temp16) - 1) & 0xFF;
    K6502_Write(temp16, temp8);
    temp8 = A - temp8;
    P = (P & ~FLAG_C) | (A >= temp8 ? FLAG_C : 0);
    setNZ(temp8);
}

static void op_DCP_ABS(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    temp8 = (K6502_Read(temp16) - 1) & 0xFF;
    K6502_Write(temp16, temp8);
    temp8 = A - temp8;
    P = (P & ~FLAG_C) | (A >= temp8 ? FLAG_C : 0);
    setNZ(temp8);
}

static void op_DCP_ABX(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    temp16 = (temp16 + X) & 0xFFFF;
    temp8 = (K6502_Read(temp16) - 1) & 0xFF;
    K6502_Write(temp16, temp8);
    temp8 = A - temp8;
    P = (P & ~FLAG_C) | (A >= temp8 ? FLAG_C : 0);
    setNZ(temp8);
}

static void op_DCP_ABY(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    temp16 = (temp16 + Y) & 0xFFFF;
    temp8 = (K6502_Read(temp16) - 1) & 0xFF;
    K6502_Write(temp16, temp8);
    temp8 = A - temp8;
    P = (P & ~FLAG_C) | (A >= temp8 ? FLAG_C : 0);
    setNZ(temp8);
}

static void op_DCP_IZX(void)
{
    temp16 = (K6502_Read(PC++) + X) & 0xFF;
    temp16 = K6502_Read(temp16) | (K6502_Read((temp16 + 1) & 0xFF) << 8);
    temp8 = (K6502_Read(temp16) - 1) & 0xFF;
    K6502_Write(temp16, temp8);
    temp8 = A - temp8;
    P = (P & ~FLAG_C) | (A >= temp8 ? FLAG_C : 0);
    setNZ(temp8);
}

static void op_DCP_IZY(void)
{
    temp16 = K6502_Read(PC++);
    temp16 = K6502_Read(temp16) | (K6502_Read((temp16 + 1) & 0xFF) << 8);
    uint16_t old = temp16;
    temp16 = (temp16 + Y) & 0xFFFF;
    if (PAGE_CROSS(old, temp16)) clocks++;
    temp8 = (K6502_Read(temp16) - 1) & 0xFF;
    K6502_Write(temp16, temp8);
    temp8 = A - temp8;
    P = (P & ~FLAG_C) | (A >= temp8 ? FLAG_C : 0);
    setNZ(temp8);
}

static void op_ISB_ZP(void)
{
    temp16 = K6502_Read(PC++);
    temp8 = (K6502_Read(temp16) + 1) & 0xFF;
    K6502_Write(temp16, temp8);
    SBC(~temp8);
}

static void op_ISB_ZPX(void)
{
    temp16 = (K6502_Read(PC++) + X) & 0xFF;
    temp8 = (K6502_Read(temp16) + 1) & 0xFF;
    K6502_Write(temp16, temp8);
    SBC(~temp8);
}

static void op_ISB_ABS(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    temp8 = (K6502_Read(temp16) + 1) & 0xFF;
    K6502_Write(temp16, temp8);
    SBC(~temp8);
}

static void op_ISB_ABX(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    temp16 = (temp16 + X) & 0xFFFF;
    temp8 = (K6502_Read(temp16) + 1) & 0xFF;
    K6502_Write(temp16, temp8);
    SBC(~temp8);
}

static void op_ISB_ABY(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    temp16 = (temp16 + Y) & 0xFFFF;
    temp8 = (K6502_Read(temp16) + 1) & 0xFF;
    K6502_Write(temp16, temp8);
    SBC(~temp8);
}

static void op_ISB_IZX(void)
{
    temp16 = (K6502_Read(PC++) + X) & 0xFF;
    temp16 = K6502_Read(temp16) | (K6502_Read((temp16 + 1) & 0xFF) << 8);
    temp8 = (K6502_Read(temp16) + 1) & 0xFF;
    K6502_Write(temp16, temp8);
    SBC(~temp8);
}

static void op_ISB_IZY(void)
{
    temp16 = K6502_Read(PC++);
    temp16 = K6502_Read(temp16) | (K6502_Read((temp16 + 1) & 0xFF) << 8);
    uint16_t old = temp16;
    temp16 = (temp16 + Y) & 0xFFFF;
    if (PAGE_CROSS(old, temp16)) clocks++;
    temp8 = (K6502_Read(temp16) + 1) & 0xFF;
    K6502_Write(temp16, temp8);
    SBC(~temp8);
}

static void op_SLO_ZP(void)
{
    temp16 = K6502_Read(PC++);
    temp8 = K6502_Read(temp16);
    P = (P & ~FLAG_C) | ((temp8 & 0x80) ? FLAG_C : 0);
    temp8 <<= 1;
    K6502_Write(temp16, temp8);
    A |= temp8;
    setNZ(A);
}

static void op_SLO_ZPX(void)
{
    temp16 = (K6502_Read(PC++) + X) & 0xFF;
    temp8 = K6502_Read(temp16);
    P = (P & ~FLAG_C) | ((temp8 & 0x80) ? FLAG_C : 0);
    temp8 <<= 1;
    K6502_Write(temp16, temp8);
    A |= temp8;
    setNZ(A);
}

static void op_SLO_ABS(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    temp8 = K6502_Read(temp16);
    P = (P & ~FLAG_C) | ((temp8 & 0x80) ? FLAG_C : 0);
    temp8 <<= 1;
    K6502_Write(temp16, temp8);
    A |= temp8;
    setNZ(A);
}

static void op_SLO_ABX(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    temp16 = (temp16 + X) & 0xFFFF;
    temp8 = K6502_Read(temp16);
    P = (P & ~FLAG_C) | ((temp8 & 0x80) ? FLAG_C : 0);
    temp8 <<= 1;
    K6502_Write(temp16, temp8);
    A |= temp8;
    setNZ(A);
}

static void op_SLO_ABY(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    temp16 = (temp16 + Y) & 0xFFFF;
    temp8 = K6502_Read(temp16);
    P = (P & ~FLAG_C) | ((temp8 & 0x80) ? FLAG_C : 0);
    temp8 <<= 1;
    K6502_Write(temp16, temp8);
    A |= temp8;
    setNZ(A);
}

static void op_SLO_IZX(void)
{
    temp16 = (K6502_Read(PC++) + X) & 0xFF;
    temp16 = K6502_Read(temp16) | (K6502_Read((temp16 + 1) & 0xFF) << 8);
    temp8 = K6502_Read(temp16);
    P = (P & ~FLAG_C) | ((temp8 & 0x80) ? FLAG_C : 0);
    temp8 <<= 1;
    K6502_Write(temp16, temp8);
    A |= temp8;
    setNZ(A);
}

static void op_SLO_IZY(void)
{
    temp16 = K6502_Read(PC++);
    temp16 = K6502_Read(temp16) | (K6502_Read((temp16 + 1) & 0xFF) << 8);
    uint16_t old = temp16;
    temp16 = (temp16 + Y) & 0xFFFF;
    if (PAGE_CROSS(old, temp16)) clocks++;
    temp8 = K6502_Read(temp16);
    P = (P & ~FLAG_C) | ((temp8 & 0x80) ? FLAG_C : 0);
    temp8 <<= 1;
    K6502_Write(temp16, temp8);
    A |= temp8;
    setNZ(A);
}

static void op_RLA_ZP(void)
{
    uint8_t old_c;
    temp16 = K6502_Read(PC++);
    temp8 = K6502_Read(temp16);
    old_c = (P & FLAG_C) ? 1 : 0;
    P = (P & ~FLAG_C) | ((temp8 & 0x80) ? FLAG_C : 0);
    temp8 = (temp8 << 1) | old_c;
    K6502_Write(temp16, temp8);
    A &= temp8;
    setNZ(A);
}

static void op_RLA_ZPX(void)
{
    uint8_t old_c;
    temp16 = (K6502_Read(PC++) + X) & 0xFF;
    temp8 = K6502_Read(temp16);
    old_c = (P & FLAG_C) ? 1 : 0;
    P = (P & ~FLAG_C) | ((temp8 & 0x80) ? FLAG_C : 0);
    temp8 = (temp8 << 1) | old_c;
    K6502_Write(temp16, temp8);
    A &= temp8;
    setNZ(A);
}

static void op_RLA_ABS(void)
{
    uint8_t old_c;
    temp16 = READ_WORD(PC);
    PC += 2;
    temp8 = K6502_Read(temp16);
    old_c = (P & FLAG_C) ? 1 : 0;
    P = (P & ~FLAG_C) | ((temp8 & 0x80) ? FLAG_C : 0);
    temp8 = (temp8 << 1) | old_c;
    K6502_Write(temp16, temp8);
    A &= temp8;
    setNZ(A);
}

static void op_RLA_ABX(void)
{
    uint8_t old_c;
    temp16 = READ_WORD(PC);
    PC += 2;
    temp16 = (temp16 + X) & 0xFFFF;
    temp8 = K6502_Read(temp16);
    old_c = (P & FLAG_C) ? 1 : 0;
    P = (P & ~FLAG_C) | ((temp8 & 0x80) ? FLAG_C : 0);
    temp8 = (temp8 << 1) | old_c;
    K6502_Write(temp16, temp8);
    A &= temp8;
    setNZ(A);
}

static void op_RLA_ABY(void)
{
    uint8_t old_c;
    temp16 = READ_WORD(PC);
    PC += 2;
    temp16 = (temp16 + Y) & 0xFFFF;
    temp8 = K6502_Read(temp16);
    old_c = (P & FLAG_C) ? 1 : 0;
    P = (P & ~FLAG_C) | ((temp8 & 0x80) ? FLAG_C : 0);
    temp8 = (temp8 << 1) | old_c;
    K6502_Write(temp16, temp8);
    A &= temp8;
    setNZ(A);
}

static void op_RLA_IZX(void)
{
    uint8_t old_c;
    temp16 = (K6502_Read(PC++) + X) & 0xFF;
    temp16 = K6502_Read(temp16) | (K6502_Read((temp16 + 1) & 0xFF) << 8);
    temp8 = K6502_Read(temp16);
    old_c = (P & FLAG_C) ? 1 : 0;
    P = (P & ~FLAG_C) | ((temp8 & 0x80) ? FLAG_C : 0);
    temp8 = (temp8 << 1) | old_c;
    K6502_Write(temp16, temp8);
    A &= temp8;
    setNZ(A);
}

static void op_RLA_IZY(void)
{
    uint8_t old_c;
    temp16 = K6502_Read(PC++);
    temp16 = K6502_Read(temp16) | (K6502_Read((temp16 + 1) & 0xFF) << 8);
    uint16_t old = temp16;
    temp16 = (temp16 + Y) & 0xFFFF;
    if (PAGE_CROSS(old, temp16)) clocks++;
    temp8 = K6502_Read(temp16);
    old_c = (P & FLAG_C) ? 1 : 0;
    P = (P & ~FLAG_C) | ((temp8 & 0x80) ? FLAG_C : 0);
    temp8 = (temp8 << 1) | old_c;
    K6502_Write(temp16, temp8);
    A &= temp8;
    setNZ(A);
}

static void op_SRE_ZP(void)
{
    temp16 = K6502_Read(PC++);
    temp8 = K6502_Read(temp16);
    P = (P & ~FLAG_C) | (temp8 & 1);
    temp8 >>= 1;
    K6502_Write(temp16, temp8);
    A ^= temp8;
    setNZ(A);
}

static void op_SRE_ZPX(void)
{
    temp16 = (K6502_Read(PC++) + X) & 0xFF;
    temp8 = K6502_Read(temp16);
    P = (P & ~FLAG_C) | (temp8 & 1);
    temp8 >>= 1;
    K6502_Write(temp16, temp8);
    A ^= temp8;
    setNZ(A);
}

static void op_SRE_ABS(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    temp8 = K6502_Read(temp16);
    P = (P & ~FLAG_C) | (temp8 & 1);
    temp8 >>= 1;
    K6502_Write(temp16, temp8);
    A ^= temp8;
    setNZ(A);
}

static void op_SRE_ABX(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    temp16 = (temp16 + X) & 0xFFFF;
    temp8 = K6502_Read(temp16);
    P = (P & ~FLAG_C) | (temp8 & 1);
    temp8 >>= 1;
    K6502_Write(temp16, temp8);
    A ^= temp8;
    setNZ(A);
}

static void op_SRE_ABY(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    temp16 = (temp16 + Y) & 0xFFFF;
    temp8 = K6502_Read(temp16);
    P = (P & ~FLAG_C) | (temp8 & 1);
    temp8 >>= 1;
    K6502_Write(temp16, temp8);
    A ^= temp8;
    setNZ(A);
}

static void op_SRE_IZX(void)
{
    temp16 = (K6502_Read(PC++) + X) & 0xFF;
    temp16 = K6502_Read(temp16) | (K6502_Read((temp16 + 1) & 0xFF) << 8);
    temp8 = K6502_Read(temp16);
    P = (P & ~FLAG_C) | (temp8 & 1);
    temp8 >>= 1;
    K6502_Write(temp16, temp8);
    A ^= temp8;
    setNZ(A);
}

static void op_SRE_IZY(void)
{
    temp16 = K6502_Read(PC++);
    temp16 = K6502_Read(temp16) | (K6502_Read((temp16 + 1) & 0xFF) << 8);
    uint16_t old = temp16;
    temp16 = (temp16 + Y) & 0xFFFF;
    if (PAGE_CROSS(old, temp16)) clocks++;
    temp8 = K6502_Read(temp16);
    P = (P & ~FLAG_C) | (temp8 & 1);
    temp8 >>= 1;
    K6502_Write(temp16, temp8);
    A ^= temp8;
    setNZ(A);
}

static void op_RRA_ZP(void)
{
    uint8_t old_c;
    temp16 = K6502_Read(PC++);
    temp8 = K6502_Read(temp16);
    old_c = (P & FLAG_C) ? 0x80 : 0;
    P = (P & ~FLAG_C) | (temp8 & 1);
    temp8 = (temp8 >> 1) | old_c;
    K6502_Write(temp16, temp8);
    ADC(temp8);
}

static void op_RRA_ZPX(void)
{
    uint8_t old_c;
    temp16 = (K6502_Read(PC++) + X) & 0xFF;
    temp8 = K6502_Read(temp16);
    old_c = (P & FLAG_C) ? 0x80 : 0;
    P = (P & ~FLAG_C) | (temp8 & 1);
    temp8 = (temp8 >> 1) | old_c;
    K6502_Write(temp16, temp8);
    ADC(temp8);
}

static void op_RRA_ABS(void)
{
    uint8_t old_c;
    temp16 = READ_WORD(PC);
    PC += 2;
    temp8 = K6502_Read(temp16);
    old_c = (P & FLAG_C) ? 0x80 : 0;
    P = (P & ~FLAG_C) | (temp8 & 1);
    temp8 = (temp8 >> 1) | old_c;
    K6502_Write(temp16, temp8);
    ADC(temp8);
}

static void op_RRA_ABX(void)
{
    uint8_t old_c;
    temp16 = READ_WORD(PC);
    PC += 2;
    temp16 = (temp16 + X) & 0xFFFF;
    temp8 = K6502_Read(temp16);
    old_c = (P & FLAG_C) ? 0x80 : 0;
    P = (P & ~FLAG_C) | (temp8 & 1);
    temp8 = (temp8 >> 1) | old_c;
    K6502_Write(temp16, temp8);
    ADC(temp8);
}

static void op_RRA_ABY(void)
{
    uint8_t old_c;
    temp16 = READ_WORD(PC);
    PC += 2;
    temp16 = (temp16 + Y) & 0xFFFF;
    temp8 = K6502_Read(temp16);
    old_c = (P & FLAG_C) ? 0x80 : 0;
    P = (P & ~FLAG_C) | (temp8 & 1);
    temp8 = (temp8 >> 1) | old_c;
    K6502_Write(temp16, temp8);
    ADC(temp8);
}

static void op_RRA_IZX(void)
{
    temp16 = (K6502_Read(PC++) + X) & 0xFF;
    temp16 = K6502_Read(temp16) | (K6502_Read((temp16 + 1) & 0xFF) << 8);
    temp8 = K6502_Read(temp16);
    uint8_t old_c = (P & FLAG_C) ? 0x80 : 0;
    P = (P & ~FLAG_C) | (temp8 & 1);
    temp8 = (temp8 >> 1) | old_c;
    K6502_Write(temp16, temp8);
    ADC(temp8);
}

static void op_RRA_IZY(void)
{
    temp16 = K6502_Read(PC++);
    temp16 = K6502_Read(temp16) | (K6502_Read((temp16 + 1) & 0xFF) << 8);
    uint16_t old = temp16;
    temp16 = (temp16 + Y) & 0xFFFF;
    if (PAGE_CROSS(old, temp16)) clocks++;
    temp8 = K6502_Read(temp16);
    uint8_t old_c = (P & FLAG_C) ? 0x80 : 0;
    P = (P & ~FLAG_C) | (temp8 & 1);
    temp8 = (temp8 >> 1) | old_c;
    K6502_Write(temp16, temp8);
    ADC(temp8);
}

static void op_ANC_IMM(void)
{
    A &= K6502_Read(PC++);
    setNZ(A);
    P = (P & ~FLAG_C) | ((A & 0x80) ? FLAG_C : 0);
}

static void op_ANE_IMM(void)
{
    A = (A | 0xEE) & X & K6502_Read(PC++);
    setNZ(A);
}

static void op_ASR_IMM(void)
{
    temp8 = A & K6502_Read(PC++);
    P = (P & ~FLAG_C) | (temp8 & 1);
    temp8 >>= 1;
    A = temp8;
    setNZ(A);
}

static void op_ARR_IMM(void)
{
    temp8 = A & K6502_Read(PC++);
    temp8 = (temp8 >> 1) | ((P & FLAG_C) ? 0x80 : 0);
    A = temp8;
    setNZ(A);
    P = (P & ~FLAG_V) | ((A >> 6) ^ (A >> 5)) & 1;
    P = (P & ~FLAG_C) | (A & 0x40);
}

static void op_SBX_IMM(void)
{
    temp8 = K6502_Read(PC++);
    temp16 = (A & X) - temp8;
    P = (P & ~FLAG_C) | ((temp16 & 0x100) ? 0 : FLAG_C);
    X = temp16 & 0xFF;
    setNZ(X);
}

static void op_LAS_ABY(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    temp16 = (temp16 + Y) & 0xFFFF;
    temp8 = K6502_Read(temp16);
    A = X = S = S & temp8;
    setNZ(A);
}

static void op_LXA_IMM(void)
{
    A = X = ((A | 0xEE) & K6502_Read(PC++));
    setNZ(A);
}

static void op_SHA_IZY(void)
{
    temp16 = K6502_Read(PC++);
    temp16 = K6502_Read(temp16) | (K6502_Read((temp16 + 1) & 0xFF) << 8);
    temp16 = (temp16 + Y) & 0xFFFF;
    temp8 = A & X & ((uint8_t)((temp16 >> 8) + 1));
    K6502_Write(temp16, temp8);
}

static void op_SHA_ABY(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    temp16 = (temp16 + Y) & 0xFFFF;
    temp8 = A & X & ((uint8_t)((temp16 >> 8) + 1));
    K6502_Write(temp16, temp8);
}

static void op_SHS_ABY(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    temp16 = (temp16 + Y) & 0xFFFF;
    S = A & X;
    temp8 = S & ((uint8_t)((temp16 >> 8) + 1));
    K6502_Write(temp16, temp8);
}

static void op_SHX_ABY(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    temp16 = (temp16 + Y) & 0xFFFF;
    temp8 = X & ((uint8_t)((temp16 >> 8) + 1));
    K6502_Write(temp16, temp8);
}

static void op_SHY_ABX(void)
{
    temp16 = READ_WORD(PC);
    PC += 2;
    temp16 = (temp16 + X) & 0xFFFF;
    temp8 = Y & ((uint8_t)((temp16 >> 8) + 1));
    K6502_Write(temp16, temp8);
}

static void op_ADC_IMM(void)
{
    ADC(K6502_Read(PC++));
}

static void op_AND_IMM(void)
{
    A &= K6502_Read(PC++);
    setNZ(A);
}

static void op_EOR_IMM(void)
{
    A ^= K6502_Read(PC++);
    setNZ(A);
}

static void op_ORA_IMM(void)
{
    A |= K6502_Read(PC++);
    setNZ(A);
}

static void op_JMP_IND_0xFF(void)
{
    temp16 = READ_WORD(PC);
    uint8_t low = K6502_Read(temp16);
    uint8_t high;
    if ((temp16 & 0x00FF) == 0x00FF)
    {
        high = K6502_Read(temp16 & 0xFF00);
    }
    else
    {
        high = K6502_Read(temp16 + 1);
    }
    PC = (high << 8) | low;
}


static void op_CMP_IZX(void)
{
    temp16 = (K6502_Read(PC++) + X) & 0xFF;
    temp16 = K6502_Read(temp16) | (K6502_Read((temp16 + 1) & 0xFF) << 8);
    CMP(A, K6502_Read(temp16));
}

static void op_CMP_IZY(void)
{
    temp16 = K6502_Read(PC++);
    temp16 = K6502_Read(temp16) | (K6502_Read((temp16 + 1) & 0xFF) << 8);
    uint16_t old = temp16;
    temp16 = (temp16 + Y) & 0xFFFF;
    if (PAGE_CROSS(old, temp16)) clocks++;
    CMP(A, K6502_Read(temp16));
}

static void op_SBC_IZX(void)
{
    temp16 = (K6502_Read(PC++) + X) & 0xFF;
    temp16 = K6502_Read(temp16) | (K6502_Read((temp16 + 1) & 0xFF) << 8);
    SBC(K6502_Read(temp16));
}

static void op_SBC_IZY(void)
{
    temp16 = K6502_Read(PC++);
    temp16 = K6502_Read(temp16) | (K6502_Read((temp16 + 1) & 0xFF) << 8);
    uint16_t old = temp16;
    temp16 = (temp16 + Y) & 0xFFFF;
    if (PAGE_CROSS(old, temp16)) clocks++;
    SBC(K6502_Read(temp16));
}

static void op_JAM(void)
{
    // Treat illegal JAM opcodes (0x02, 0x12, 0x22, ... in the NES 6502) as no-ops.
    // The 6502 reference ROMs and many mapper ROMs do not rely on these to halt execution.
    // By making JAM a benign instruction, we avoid infinite loops that occur when a mapper
    // or game writes these values by mistake or uses them as data in memory regions.
    // Note: The caller already advanced PC past the opcode, so we don't modify PC here.
}

static void op_ILL(void)
{
    PC++;
}

static void init_op_handlers(void)
{
    int i;
    for (i = 0; i < 256; i++) op_handlers[i] = op_ILL;

    op_handlers[0x00] = op_BRK;
    op_handlers[0x01] = op_ORA_IZX;
    op_handlers[0x05] = op_ORA_ZP;
    op_handlers[0x06] = op_ASL_ZP;
    op_handlers[0x08] = op_PHP;
    op_handlers[0x09] = op_ORA_IMM;
    op_handlers[0x0A] = ASL_ACC;
    op_handlers[0x0D] = op_ORA_ABS;
    op_handlers[0x0E] = op_ASL_ABS;

    op_handlers[0x10] = op_BPL;
    op_handlers[0x11] = op_ORA_IZY;
    op_handlers[0x15] = op_ORA_ZPX;
    op_handlers[0x16] = op_ASL_ZPX;
    op_handlers[0x18] = op_CLC;
    op_handlers[0x19] = op_ORA_ABY;
    op_handlers[0x1D] = op_ORA_ABX;
    op_handlers[0x1E] = op_ASL_ABX;

    op_handlers[0x20] = op_JSR;
    op_handlers[0x21] = op_AND_IZX;
    op_handlers[0x24] = op_BIT_ZP;
    op_handlers[0x25] = op_AND_ZP;
    op_handlers[0x26] = op_ROL_ZP;
    op_handlers[0x28] = op_PLP;
    op_handlers[0x29] = op_AND_IMM;
    op_handlers[0x2A] = ROL_ACC;
    op_handlers[0x2C] = op_BIT_ABS;
    op_handlers[0x2D] = op_AND_ABS;
    op_handlers[0x2E] = op_ROL_ABS;

    op_handlers[0x30] = op_BMI;
    op_handlers[0x31] = op_AND_IZY;
    op_handlers[0x35] = op_AND_ZPX;
    op_handlers[0x36] = op_ROL_ZPX;
    op_handlers[0x38] = op_SEC;
    op_handlers[0x39] = op_AND_ABY;
    op_handlers[0x3D] = op_AND_ABX;
    op_handlers[0x3E] = op_ROL_ABX;

    op_handlers[0x40] = op_RTI;
    op_handlers[0x41] = op_EOR_IZX;
    op_handlers[0x45] = op_EOR_ZP;
    op_handlers[0x46] = op_LSR_ZP;
    op_handlers[0x48] = op_PHA;
    op_handlers[0x49] = op_EOR_IMM;
    op_handlers[0x4A] = LSR_ACC;
    op_handlers[0x4C] = op_JMP_ABS;
    op_handlers[0x4D] = op_EOR_ABS;
    op_handlers[0x4E] = op_LSR_ABS;

    op_handlers[0x50] = op_BVC;
    op_handlers[0x51] = op_EOR_IZY;
    op_handlers[0x55] = op_EOR_ZPX;
    op_handlers[0x56] = op_LSR_ZPX;
    op_handlers[0x58] = op_CLI;
    op_handlers[0x59] = op_EOR_ABY;
    op_handlers[0x5D] = op_EOR_ABX;
    op_handlers[0x5E] = op_LSR_ABX;

    op_handlers[0x60] = op_RTS;
    op_handlers[0x61] = op_ADC_IZX;
    op_handlers[0x65] = op_ADC_ZP;
    op_handlers[0x66] = op_ROR_ZP;
    op_handlers[0x68] = op_PLA;
    op_handlers[0x69] = op_ADC_IMM;
    op_handlers[0x6A] = ROR_ACC;
    op_handlers[0x6C] = op_JMP_IND_0xFF;
    op_handlers[0x6D] = op_ADC_ABS;
    op_handlers[0x6E] = op_ROR_ABS;

    op_handlers[0x70] = op_BVS;
    op_handlers[0x71] = op_ADC_IZY;
    op_handlers[0x75] = op_ADC_ZPX;
    op_handlers[0x76] = op_ROR_ZPX;
    op_handlers[0x78] = op_SEI;
    op_handlers[0x79] = op_ADC_ABY;
    op_handlers[0x7D] = op_ADC_ABX;
    op_handlers[0x7E] = op_ROR_ABX;

    op_handlers[0x80] = op_NOP;
    op_handlers[0x81] = op_STA_IZX;
    op_handlers[0x84] = op_STY_ZP;
    op_handlers[0x85] = op_STA_ZP;
    op_handlers[0x86] = op_STX_ZP;
    op_handlers[0x88] = op_DEY;
    op_handlers[0x8A] = op_TXA;
    op_handlers[0x8C] = op_STY_ABS;
    op_handlers[0x8D] = op_STA_ABS;
    op_handlers[0x8E] = op_STX_ABS;

    op_handlers[0x90] = op_BCC;
    op_handlers[0x91] = op_STA_IZY;
    op_handlers[0x94] = op_STY_ZPX;
    op_handlers[0x95] = op_STA_ZPX;
    op_handlers[0x96] = op_STX_ZPY;
    op_handlers[0x98] = op_TYA;
    op_handlers[0x99] = op_STA_ABY;
    op_handlers[0x9A] = op_TXS;
    op_handlers[0x9D] = op_STA_ABX;

    op_handlers[0xA0] = op_LDY_IMM;
    op_handlers[0xA1] = op_LDA_IZX;
    op_handlers[0xA2] = op_LDX_IMM;
    op_handlers[0xA4] = op_LDY_ZP;
    op_handlers[0xA5] = op_LDA_ZP;
    op_handlers[0xA6] = op_LDX_ZP;
    op_handlers[0xA8] = op_TAY;
    op_handlers[0xA9] = op_LDA_IMM;
    op_handlers[0xAA] = op_TAX;
    op_handlers[0xAC] = op_LDY_ABS;
    op_handlers[0xAD] = op_LDA_ABS;
    op_handlers[0xAE] = op_LDX_ABS;

    op_handlers[0xB0] = op_BCS;
    op_handlers[0xB1] = op_LDA_IZY;
    op_handlers[0xB4] = op_LDY_ZPX;
    op_handlers[0xB5] = op_LDA_ZPX;
    op_handlers[0xB6] = op_LDX_ZPY;
    op_handlers[0xB8] = op_CLV;
    op_handlers[0xB9] = op_LDA_ABY;
    op_handlers[0xBA] = op_TSX;
    op_handlers[0xBC] = op_LDY_ABX;
    op_handlers[0xBD] = op_LDA_ABX;
    op_handlers[0xBE] = op_LDX_ABY;

    op_handlers[0xC0] = op_CPY_IMM;
    op_handlers[0xC1] = op_CMP_IZX;
    op_handlers[0xC4] = op_CPY_ZP;
    op_handlers[0xC5] = op_CMP_ZP;
    op_handlers[0xC6] = op_DEC_ZP;
    op_handlers[0xC8] = op_INY;
    op_handlers[0xC9] = op_CMP_IMM;
    op_handlers[0xCA] = op_DEX;
    op_handlers[0xCC] = op_CPY_ABS;
    op_handlers[0xCD] = op_CMP_ABS;
    op_handlers[0xCE] = op_DEC_ABS;

    op_handlers[0xD0] = op_BNE;
    op_handlers[0xD1] = op_CMP_IZY;
    op_handlers[0xD5] = op_CMP_ZPX;
    op_handlers[0xD6] = op_DEC_ZPX;
    op_handlers[0xD8] = op_CLD;
    op_handlers[0xD9] = op_CMP_ABY;
    op_handlers[0xDD] = op_CMP_ABX;
    op_handlers[0xDE] = op_DEC_ABX;

    op_handlers[0xE0] = op_CPX_IMM;
    op_handlers[0xE1] = op_SBC_IZX;
    op_handlers[0xE4] = op_CPX_ZP;
    op_handlers[0xE5] = op_SBC_ZP;
    op_handlers[0xE6] = op_INC_ZP;
    op_handlers[0xE8] = op_INX;
    op_handlers[0xE9] = op_SBC_IMM;
    op_handlers[0xEA] = op_NOP;
    op_handlers[0xEC] = op_CPX_ABS;
    op_handlers[0xED] = op_SBC_ABS;
    op_handlers[0xEE] = op_INC_ABS;

    op_handlers[0xF0] = op_BEQ;
    op_handlers[0xF1] = op_SBC_IZY;
    op_handlers[0xF5] = op_SBC_ZPX;
    op_handlers[0xF6] = op_INC_ZPX;
    op_handlers[0xF8] = op_SED;
    op_handlers[0xF9] = op_SBC_ABY;
    op_handlers[0xFD] = op_SBC_ABX;
    op_handlers[0xFE] = op_INC_ABX;

    op_handlers[0x02] = op_JAM;
    op_handlers[0x12] = op_JAM;
    op_handlers[0x22] = op_JAM;
    op_handlers[0x32] = op_JAM;
    op_handlers[0x42] = op_JAM;
    op_handlers[0x52] = op_JAM;
    op_handlers[0x62] = op_JAM;
    op_handlers[0x72] = op_JAM;
    op_handlers[0x92] = op_JAM;
    op_handlers[0xB2] = op_JAM;
    op_handlers[0xD2] = op_JAM;
    op_handlers[0xF2] = op_JAM;

    op_handlers[0x03] = op_SLO_IZX;
    op_handlers[0x07] = op_SLO_ZP;
    op_handlers[0x0F] = op_SLO_ABS;
    op_handlers[0x13] = op_SLO_IZY;
    op_handlers[0x17] = op_SLO_ZPX;
    op_handlers[0x1B] = op_SLO_ABY;
    op_handlers[0x1F] = op_SLO_ABX;

    op_handlers[0x0B] = op_ANC_IMM;
    op_handlers[0x2B] = op_ANC_IMM;

    op_handlers[0x23] = op_RLA_IZX;
    op_handlers[0x27] = op_RLA_ZP;
    op_handlers[0x2F] = op_RLA_ABS;
    op_handlers[0x33] = op_RLA_IZY;
    op_handlers[0x37] = op_RLA_ZPX;
    op_handlers[0x3B] = op_RLA_ABY;
    op_handlers[0x3F] = op_RLA_ABX;

    op_handlers[0x43] = op_SRE_IZX;
    op_handlers[0x47] = op_SRE_ZP;
    op_handlers[0x4F] = op_SRE_ABS;
    op_handlers[0x53] = op_SRE_IZY;
    op_handlers[0x57] = op_SRE_ZPX;
    op_handlers[0x5B] = op_SRE_ABY;
    op_handlers[0x5F] = op_SRE_ABX;

    op_handlers[0x4B] = op_ASR_IMM;

    op_handlers[0x63] = op_RRA_IZX;
    op_handlers[0x67] = op_RRA_ZP;
    op_handlers[0x6F] = op_RRA_ABS;
    op_handlers[0x73] = op_RRA_IZY;
    op_handlers[0x77] = op_RRA_ZPX;
    op_handlers[0x7B] = op_RRA_ABY;
    op_handlers[0x7F] = op_RRA_ABX;

    op_handlers[0x6B] = op_ARR_IMM;

    op_handlers[0x82] = op_NOP;
    op_handlers[0x89] = op_NOP;
    op_handlers[0xC2] = op_NOP;
    op_handlers[0xE2] = op_NOP;

    op_handlers[0x83] = op_SAX_IZX;
    op_handlers[0x87] = op_SAX_ZP;
    op_handlers[0x8F] = op_SAX_ABS;
    op_handlers[0x93] = op_SHA_IZY;
    op_handlers[0x97] = op_SAX_ZPY;

    op_handlers[0x8B] = op_ANE_IMM;

    op_handlers[0x9B] = op_SHS_ABY;
    op_handlers[0x9C] = op_SHY_ABX;
    op_handlers[0x9E] = op_SHX_ABY;
    op_handlers[0x9F] = op_SHA_ABY;

    op_handlers[0xA3] = op_LAX_IZX;
    op_handlers[0xA7] = op_LAX_ZP;
    op_handlers[0xAF] = op_LAX_ABS;
    op_handlers[0xAB] = op_LXA_IMM;
    op_handlers[0xB3] = op_LAX_IZY;
    op_handlers[0xB7] = op_LAX_ZPY;
    op_handlers[0xBB] = op_LAS_ABY;
    op_handlers[0xBF] = op_LAX_ABY;

    op_handlers[0xC3] = op_DCP_IZX;
    op_handlers[0xC7] = op_DCP_ZP;
    op_handlers[0xCF] = op_DCP_ABS;
    op_handlers[0xCB] = op_SBX_IMM;
    op_handlers[0xD3] = op_DCP_IZY;
    op_handlers[0xD7] = op_DCP_ZPX;
    op_handlers[0xDB] = op_DCP_ABY;
    op_handlers[0xDF] = op_DCP_ABX;

    op_handlers[0xE3] = op_ISB_IZX;
    op_handlers[0xE7] = op_ISB_ZP;
    op_handlers[0xEF] = op_ISB_ABS;
    op_handlers[0xEB] = op_SBC_IMM;
    op_handlers[0xF3] = op_ISB_IZY;
    op_handlers[0xF7] = op_ISB_ZPX;
    op_handlers[0xFB] = op_ISB_ABY;
    op_handlers[0xFF] = op_ISB_ABX;

    op_handlers[0x04] = op_NOP;
    op_handlers[0x14] = op_NOP;
    op_handlers[0x1A] = op_NOP;
    op_handlers[0x34] = op_NOP;
    op_handlers[0x3A] = op_NOP;
    op_handlers[0x44] = op_NOP;
    op_handlers[0x54] = op_NOP;
    op_handlers[0x5A] = op_NOP;
    op_handlers[0x64] = op_NOP;
    op_handlers[0x74] = op_NOP;
    op_handlers[0x7A] = op_NOP;
    op_handlers[0x0C] = op_NOP;
    op_handlers[0x1C] = op_NOP;
    op_handlers[0x3C] = op_NOP;
    op_handlers[0x5C] = op_NOP;
    op_handlers[0x7C] = op_NOP;
    op_handlers[0xD4] = op_NOP;
    op_handlers[0xDC] = op_NOP;
    op_handlers[0xF4] = op_NOP;
    op_handlers[0xFA] = op_NOP;
}

static void map_bank(int8_t page, uint8_t bank_idx)
{
    uint32_t num_banks = RomHeader ? RomHeader->num_16k_rom_banks : 2;
    uint32_t rommask = (num_banks * 0x4000) - 1;

    if (page < 0)
    {
        page = (int8_t)((num_banks * 2 + page) % (num_banks * 2));
    }

    uint32_t page_idx = ((uint32_t)page << 13) & rommask;

    uint32_t rom_offset = 16;
    if (RomHeader && (RomHeader->flags_1 & 0x04))
    {
        rom_offset += 512;
    }

    CPU_ROM_banks[bank_idx] = romfile + rom_offset + page_idx;
}

void map67_(int8_t page)
{
    map_bank(page, 4);
}

void map89_(int8_t page)
{
    map_bank(page, 0);
}

void mapAB_(int8_t page)
{
    map_bank(page, 1);
}

void mapCD_(int8_t page)
{
    map_bank(page, 2);
}

void mapEF_(int8_t page)
{
    map_bank(page, 3);
}

void cpu6502_init(void)
{
    memset(NES_RAM, 0, 0x800);

    uint32_t rom_offset = 16;
    if (RomHeader && (RomHeader->flags_1 & 0x04)) rom_offset += 512;

    // 计算 8KB Bank 数量
    uint32_t num_16k_banks = RomHeader ? RomHeader->num_16k_rom_banks : 2;
    uint32_t num_8k_banks = num_16k_banks * 2;

    // 1. $8000-$BFFF: 映射前 16KB (Bank 0, 1)
    CPU_ROM_banks[0] = romfile + rom_offset;
    CPU_ROM_banks[1] = romfile + rom_offset + 0x2000;
    // 2. $C000-$FFFF: 映射最后 16KB (倒数第二和最后一个 Bank)
    uint32_t last_bank_idx = num_8k_banks - 1;
    uint32_t prev_bank_idx = (num_8k_banks > 1) ? (num_8k_banks - 2) : 0;

    CPU_ROM_banks[2] = romfile + rom_offset + (prev_bank_idx * 0x2000);
    CPU_ROM_banks[3] = romfile + rom_offset + (last_bank_idx * 0x2000);

    CPU_reset();
    init_op_handlers();
    LOG("CPU初始化成功！！！");
}

void CPU_reset(void)
{
    A = X = Y = 0;
    S = 0xFD;
    P = FLAG_R | FLAG_I;
    cpunmi = 0;
    cpuirq = 0;
    clocks = 0;

    PC = READ_WORD(RES_VECTOR);
}

void ISR6502(uint16_t VECTOR)
{
    push((PC >> 8) & 0xFF);
    push(PC & 0xFF);
    push(P & ~FLAG_B);
    P |= FLAG_I;
    PC = READ_WORD(VECTOR);
    clocks += 7;
}

void run6502(uint32_t cycles)
{
    uint32_t total_cycles = cycles / 256;
    uint32_t target_cycles = clocks + total_cycles;

    while (clocks < target_cycles)
    {
        if (cpunmi)
        {
            cpunmi = 0;
            ISR6502(NMI_VECTOR);
        }
        if (cpuirq && !(P & FLAG_I))
        {
            cpuirq = 0;
            ISR6502(IRQ_VECTOR);
        }
        uint8_t opcode = K6502_Read(PC++);
        op_handlers[opcode]();
        clocks += cycles_map[opcode];
    }
}
