#include "nes_cpu.h"
#include "nes_main.h"
#include <string.h>
static uint8_t* CPU_ROM_banks[5] = {0};
static uint8_t joy0_shift; // 手柄1移位寄存器
static uint8_t joy1_shift; // 手柄2移位寄存器
static uint8_t A, X, Y, S, P;
static uint16_t PC;
static uint8_t temp8;
static uint16_t temp16; // Used for effective address (EA)
static int g_page_crossed = 0;
static uint16_t g_bad_addr = 0;
static int cpu_jam = 0; // CPU Jam state for KIL instructions
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
static OpFunc op_handlers[256][2];

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
		return CPU_ROM_banks[0] ? CPU_ROM_banks[0][addr & 0x1FFF] : 0;
	}
	else if (addr < 0xA000)
	{
		return CPU_ROM_banks[1] ? CPU_ROM_banks[1][addr & 0x1FFF] : 0;
	}
	else if (addr < 0xC000)
	{
		return CPU_ROM_banks[2] ? CPU_ROM_banks[2][addr & 0x1FFF] : 0;
	}
	else if (addr < 0xE000)
	{
		return CPU_ROM_banks[3] ? CPU_ROM_banks[3][addr & 0x1FFF] : 0;
	}
	else
	{
		return CPU_ROM_banks[4] ? CPU_ROM_banks[4][addr & 0x1FFF] : 0;
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

// ---------------- Addressing Modes ----------------
static void am_IMP(void)
{
}

static void am_IMM(void)
{
	temp16 = PC;
	PC++;
}

static void am_ZP(void)
{
	temp16 = K6502_Read(PC++);
}

static void am_ZPX(void)
{
	temp16 = (K6502_Read(PC++) + X) & 0xFF;
}

static void am_ZPY(void)
{
	temp16 = (K6502_Read(PC++) + Y) & 0xFF;
}

static void am_ABS(void)
{
	temp16 = READ_WORD(PC);
	PC += 2;
}

static void am_ABX(void)
{
	uint16_t base = READ_WORD(PC);
	PC += 2;
	temp16 = (base + X) & 0xFFFF;
	if (PAGE_CROSS(base, temp16))
	{
		g_page_crossed = 1;
		g_bad_addr = (base & 0xFF00) | ((base + X) & 0xFF);
	}
}

static void am_ABY(void)
{
	uint16_t base = READ_WORD(PC);
	PC += 2;
	temp16 = (base + Y) & 0xFFFF;
	if (PAGE_CROSS(base, temp16))
	{
		g_page_crossed = 1;
		g_bad_addr = (base & 0xFF00) | ((base + Y) & 0xFF);
	}
}

static void am_IZX(void)
{
	uint8_t ptr = (K6502_Read(PC++) + X) & 0xFF;
	temp16 = K6502_Read(ptr) | (K6502_Read((ptr + 1) & 0xFF) << 8);
}

static void am_IZY(void)
{
	uint8_t ptr = K6502_Read(PC++);
	uint16_t base = K6502_Read(ptr) | (K6502_Read((ptr + 1) & 0xFF) << 8);
	temp16 = (base + Y) & 0xFFFF;
	if (PAGE_CROSS(base, temp16))
	{
		g_page_crossed = 1;
		g_bad_addr = (base & 0xFF00) | ((base + Y) & 0xFF);
	}
}

static void am_REL(void)
{
	int8_t offset = (int8_t)K6502_Read(PC);
	uint16_t target = (PC + 1 + offset) & 0xFFFF;
	if (PAGE_CROSS(PC + 1, target))
	{
		g_page_crossed = 1;
	}
	PC++;
	temp16 = target;
}

static void am_ACC(void)
{
	temp16 = 0xDEAD;
}

static void am_IND(void)
{
	uint16_t ptr = READ_WORD(PC);
	PC += 2;
	uint8_t low = K6502_Read(ptr);
	uint8_t high;
	if ((ptr & 0x00FF) == 0x00FF)
	{
		high = K6502_Read(ptr & 0xFF00);
	}
	else
	{
		high = K6502_Read(ptr + 1);
	}
	temp16 = (high << 8) | low;
}

// ---------------- Operations ----------------
static void op_ADC(void)
{
	if (g_page_crossed)
	{
		K6502_Read(g_bad_addr);
		clocks++;
	}
	ADC(K6502_Read(temp16));
}

static void op_AND(void)
{
	if (g_page_crossed)
	{
		K6502_Read(g_bad_addr);
		clocks++;
	}
	A &= K6502_Read(temp16);
	setNZ(A);
}

static void op_ASL(void)
{
	if (temp16 == 0xDEAD)
	{
		P = (P & ~FLAG_C) | ((A & 0x80) ? FLAG_C : 0);
		A <<= 1;
		setNZ(A);
	}
	else
	{
		temp8 = K6502_Read(temp16);
		P = (P & ~FLAG_C) | ((temp8 & 0x80) ? FLAG_C : 0);
		temp8 <<= 1;
		K6502_Write(temp16, temp8);
		setNZ(temp8);
	}
}

static void op_BCC(void)
{
	if (!(P & FLAG_C))
	{
		PC = temp16;
		clocks += 1 + g_page_crossed;
	}
}

static void op_BCS(void)
{
	if (P & FLAG_C)
	{
		PC = temp16;
		clocks += 1 + g_page_crossed;
	}
}

static void op_BEQ(void)
{
	if (P & FLAG_Z)
	{
		PC = temp16;
		clocks += 1 + g_page_crossed;
	}
}

static void op_BIT(void)
{
	temp8 = K6502_Read(temp16);
	P = (P & ~(FLAG_N | FLAG_V | FLAG_Z)) | (temp8 & (FLAG_N | FLAG_V)) | ((A & temp8) == 0 ? FLAG_Z : 0);
}

static void op_BMI(void)
{
	if (P & FLAG_N)
	{
		PC = temp16;
		clocks += 1 + g_page_crossed;
	}
}

static void op_BNE(void)
{
	if (!(P & FLAG_Z))
	{
		PC = temp16;
		clocks += 1 + g_page_crossed;
	}
}

static void op_BPL(void)
{
	if (!(P & FLAG_N))
	{
		PC = temp16;
		clocks += 1 + g_page_crossed;
	}
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

static void op_BVC(void)
{
	if (!(P & FLAG_V))
	{
		PC = temp16;
		clocks += 1 + g_page_crossed;
	}
}

static void op_BVS(void)
{
	if (P & FLAG_V)
	{
		PC = temp16;
		clocks += 1 + g_page_crossed;
	}
}

static void op_CLC(void) { P &= ~FLAG_C; }
static void op_CLD(void) { P &= ~FLAG_D; }
static void op_CLI(void) { P &= ~FLAG_I; }
static void op_CLV(void) { P &= ~FLAG_V; }

static void op_CMP(void)
{
	if (g_page_crossed)
	{
		K6502_Read(g_bad_addr);
		clocks++;
	}
	CMP(A, K6502_Read(temp16));
}

static void op_CPX(void)
{
	CMP(X, K6502_Read(temp16));
}

static void op_CPY(void)
{
	CMP(Y, K6502_Read(temp16));
}

static void op_DEC(void)
{
	temp8 = (K6502_Read(temp16) - 1) & 0xFF;
	K6502_Write(temp16, temp8);
	setNZ(temp8);
}

static void op_DEX(void)
{
	X = (X - 1) & 0xFF;
	setNZ(X);
}

static void op_DEY(void)
{
	Y = (Y - 1) & 0xFF;
	setNZ(Y);
}

static void op_EOR(void)
{
	if (g_page_crossed)
	{
		K6502_Read(g_bad_addr);
		clocks++;
	}
	A ^= K6502_Read(temp16);
	setNZ(A);
}

static void op_INC(void)
{
	temp8 = (K6502_Read(temp16) + 1) & 0xFF;
	K6502_Write(temp16, temp8);
	setNZ(temp8);
}

static void op_INX(void)
{
	X = (X + 1) & 0xFF;
	setNZ(X);
}

static void op_INY(void)
{
	Y = (Y + 1) & 0xFF;
	setNZ(Y);
}

static void op_JMP(void)
{
	PC = temp16;
}

static void op_JSR(void)
{
	PC--;
	push((PC >> 8) & 0xFF);
	push(PC & 0xFF);
	PC = temp16;
}

static void op_LDA(void)
{
	if (g_page_crossed)
	{
		K6502_Read(g_bad_addr);
		clocks++;
	}
	A = K6502_Read(temp16);
	setNZ(A);
}

static void op_LDX(void)
{
	if (g_page_crossed)
	{
		K6502_Read(g_bad_addr);
		clocks++;
	}
	X = K6502_Read(temp16);
	setNZ(X);
}

static void op_LDY(void)
{
	if (g_page_crossed)
	{
		K6502_Read(g_bad_addr);
		clocks++;
	}
	Y = K6502_Read(temp16);
	setNZ(Y);
}

static void op_LSR(void)
{
	if (temp16 == 0xDEAD)
	{
		P = (P & ~FLAG_C) | (A & 1);
		A >>= 1;
		setNZ(A);
	}
	else
	{
		temp8 = K6502_Read(temp16);
		P = (P & ~FLAG_C) | (temp8 & 1);
		temp8 >>= 1;
		K6502_Write(temp16, temp8);
		setNZ(temp8);
	}
}

static void op_NOP(void)
{
}

static void op_ORA(void)
{
	if (g_page_crossed)
	{
		K6502_Read(g_bad_addr);
		clocks++;
	}
	A |= K6502_Read(temp16);
	setNZ(A);
}

static void op_PHA(void) { push(A); }
static void op_PHP(void) { push(P | FLAG_B | FLAG_R); }

static void op_PLA(void)
{
	A = pop();
	setNZ(A);
}

static void op_PLP(void) { P = pop() | FLAG_R | FLAG_B; }

static void op_ROL(void)
{
	uint8_t carry_in = (P & FLAG_C) ? 1 : 0;
	if (temp16 == 0xDEAD)
	{
		P = (P & ~FLAG_C) | ((A & 0x80) ? FLAG_C : 0);
		A = (A << 1) | carry_in;
		setNZ(A);
	}
	else
	{
		temp8 = K6502_Read(temp16);
		P = (P & ~FLAG_C) | ((temp8 & 0x80) ? FLAG_C : 0);
		temp8 = (temp8 << 1) | carry_in;
		K6502_Write(temp16, temp8);
		setNZ(temp8);
	}
}

static void op_ROR(void)
{
	uint8_t carry_in = (P & FLAG_C) ? 0x80 : 0;
	if (temp16 == 0xDEAD)
	{
		P = (P & ~FLAG_C) | (A & 1);
		A = (A >> 1) | carry_in;
		setNZ(A);
	}
	else
	{
		temp8 = K6502_Read(temp16);
		P = (P & ~FLAG_C) | (temp8 & 1);
		temp8 = (temp8 >> 1) | carry_in;
		K6502_Write(temp16, temp8);
		setNZ(temp8);
	}
}

static void op_RTI(void)
{
	P = pop();
	temp16 = pop();
	temp16 |= pop() << 8;
	PC = temp16;
}

static void op_RTS(void)
{
	temp16 = pop();
	temp16 |= pop() << 8;
	PC = temp16 + 1;
}

static void op_SBC(void)
{
	if (g_page_crossed)
	{
		K6502_Read(g_bad_addr);
		clocks++;
	}
	SBC(K6502_Read(temp16));
}

static void op_SEC(void) { P |= FLAG_C; }
static void op_SED(void) { P |= FLAG_D; }
static void op_SEI(void) { P |= FLAG_I; }
static void op_STA(void) { K6502_Write(temp16, A); }
static void op_STX(void) { K6502_Write(temp16, X); }
static void op_STY(void) { K6502_Write(temp16, Y); }

static void op_TAX(void)
{
	X = A;
	setNZ(X);
}

static void op_TAY(void)
{
	Y = A;
	setNZ(Y);
}

static void op_TSX(void)
{
	X = S;
	setNZ(X);
}

static void op_TXA(void)
{
	A = X;
	setNZ(A);
}

static void op_TXS(void) { S = X; }

static void op_TYA(void)
{
	A = Y;
	setNZ(A);
}

// ---------------- Illegal Operations ----------------
#define ENABLE_ILLEGAL_OPCODE 1
#if ENABLE_ILLEGAL_OPCODE
// NOP Read: NOPs that read memory (side effects)
static void op_NOP_RD(void)
{
	if (g_page_crossed)
	{
		K6502_Read(g_bad_addr);
		clocks++;
	}
	K6502_Read(temp16);
}

// SLO: ASL memory then ORA with A
static void op_SLO(void)
{
	temp8 = K6502_Read(temp16);
	P = (P & ~FLAG_C) | ((temp8 & 0x80) ? FLAG_C : 0);
	temp8 <<= 1;
	K6502_Write(temp16, temp8);
	A |= temp8;
	setNZ(A);
}

// RLA: ROL memory then AND with A
static void op_RLA(void)
{
	uint8_t carry_in = (P & FLAG_C) ? 1 : 0;
	temp8 = K6502_Read(temp16);
	P = (P & ~FLAG_C) | ((temp8 & 0x80) ? FLAG_C : 0);
	temp8 = (temp8 << 1) | carry_in;
	K6502_Write(temp16, temp8);
	A &= temp8;
	setNZ(A);
}

// SRE: LSR memory then EOR with A
static void op_SRE(void)
{
	temp8 = K6502_Read(temp16);
	P = (P & ~FLAG_C) | (temp8 & 1);
	temp8 >>= 1;
	K6502_Write(temp16, temp8);
	A ^= temp8;
	setNZ(A);
}

// RRA: ROR memory then ADC with A
static void op_RRA(void)
{
	uint8_t carry_in = (P & FLAG_C) ? 0x80 : 0;
	temp8 = K6502_Read(temp16);
	P = (P & ~FLAG_C) | (temp8 & 1);
	temp8 = (temp8 >> 1) | carry_in;
	K6502_Write(temp16, temp8);
	ADC(temp8);
}

// SAX: Store A & X
static void op_SAX(void)
{
	K6502_Write(temp16, A & X);
}

// LAX: Load A and X
static void op_LAX(void)
{
	if (g_page_crossed)
	{
		K6502_Read(g_bad_addr);
		clocks++;
	}
	A = K6502_Read(temp16);
	X = A;
	setNZ(A);
}

// DCP: DEC memory then CMP with A
static void op_DCP(void)
{
	temp8 = (K6502_Read(temp16) - 1) & 0xFF;
	K6502_Write(temp16, temp8);
	CMP(A, temp8);
}

// ISB: INC memory then SBC with A
static void op_ISB(void)
{
	temp8 = (K6502_Read(temp16) + 1) & 0xFF;
	K6502_Write(temp16, temp8);
	SBC(temp8);
}

// ANC: AND immediate, set carry from bit 7
static void op_ANC(void)
{
	A &= K6502_Read(temp16);
	setNZ(A);
	if (A & 0x80) P |= FLAG_C;
	else P &= ~FLAG_C;
}

// ALR: AND immediate then LSR A
static void op_ALR(void)
{
	A &= K6502_Read(temp16);
	P = (P & ~FLAG_C) | (A & 1);
	A >>= 1;
	setNZ(A);
}

// ARR: AND immediate then ROR A
// Complex flag behavior
static void op_ARR(void)
{
	A &= K6502_Read(temp16);
	uint8_t carry_in = (P & FLAG_C) ? 0x80 : 0;
	A = (A >> 1) | carry_in;
	setNZ(A);
	// Set C based on bit 5 and 6 (overflow logic)
	P = (P & ~FLAG_C) | ((A & 0x40) ? FLAG_C : 0);
	// Set V based on bit 5 XOR bit 6
	if (((A >> 5) ^ (A >> 6)) & 1) P |= FLAG_V;
	else P &= ~FLAG_V;
}

// XAA: TXA then AND immediate (A = X & Imm)
// Behavior varies by CPU revision, this is a common stable implementation.
static void op_XAA(void)
{
	A = X & K6502_Read(temp16);
	setNZ(A);
}

// AXS: Store X & A, then subtract (X = (A & X) - Imm)
static void op_AXS(void)
{
	uint8_t val = K6502_Read(temp16);
	uint16_t res = (A & X) - val;
	X = res & 0xFF;
	setNZ(X);
	P = (P & ~FLAG_C) | (res < 0x100 ? FLAG_C : 0);
}

// AHX: Store A & X & HighByte
static void op_AHX(void)
{
	uint8_t val = A & X & (uint8_t)(temp16 >> 8);
	K6502_Write(temp16, val);
}

// SHY: Store Y & (BaseAddrHi + 1)
static void op_SHY(void)
{
	uint16_t addr;
	uint8_t val;
	if (g_page_crossed)
	{
		addr = g_bad_addr;
		val = Y & ((g_bad_addr >> 8) + 1);
	}
	else
	{
		addr = temp16;
		val = Y & ((temp16 >> 8) + 1);
	}
	K6502_Write(addr, val);
}

// SHX: Store X & (BaseAddrHi + 1)
static void op_SHX(void)
{
	uint16_t addr;
	uint8_t val;
	if (g_page_crossed)
	{
		addr = g_bad_addr;
		val = X & ((g_bad_addr >> 8) + 1);
	}
	else
	{
		addr = temp16;
		val = X & ((temp16 >> 8) + 1);
	}
	K6502_Write(addr, val);
}

// TAS: Store A & X to S, then Store S & HighByte
static void op_TAS(void)
{
	S = A & X;
	uint8_t val = S & (uint8_t)(temp16 >> 8);
	K6502_Write(temp16, val);
}

// LAS: Load A, X, S with Mem & S
static void op_LAS(void)
{
	if (g_page_crossed)
	{
		K6502_Read(g_bad_addr);
		clocks++;
	}
	A = X = S = (K6502_Read(temp16) & S);
	setNZ(A);
}

// KIL: Jam the CPU
static void op_KIL(void)
{
	cpu_jam = 1;
	PC--; // Stay on current instruction
}
#endif
static void init_op_handlers(void)
{
	int i;
	for (i = 0; i < 256; i++)
	{
		op_handlers[i][0] = am_IMP;
		op_handlers[i][1] = op_NOP;
	}
	// Legal Opcodes
	op_handlers[0x00][0] = am_IMP;
	op_handlers[0x00][1] = op_BRK;
	op_handlers[0x01][0] = am_IZX;
	op_handlers[0x01][1] = op_ORA;
	op_handlers[0x05][0] = am_ZP;
	op_handlers[0x05][1] = op_ORA;
	op_handlers[0x06][0] = am_ZP;
	op_handlers[0x06][1] = op_ASL;
	op_handlers[0x08][0] = am_IMP;
	op_handlers[0x08][1] = op_PHP;
	op_handlers[0x09][0] = am_IMM;
	op_handlers[0x09][1] = op_ORA;
	op_handlers[0x0A][0] = am_ACC;
	op_handlers[0x0A][1] = op_ASL;
	op_handlers[0x0D][0] = am_ABS;
	op_handlers[0x0D][1] = op_ORA;
	op_handlers[0x0E][0] = am_ABS;
	op_handlers[0x0E][1] = op_ASL;
	op_handlers[0x10][0] = am_REL;
	op_handlers[0x10][1] = op_BPL;
	op_handlers[0x11][0] = am_IZY;
	op_handlers[0x11][1] = op_ORA;
	op_handlers[0x15][0] = am_ZPX;
	op_handlers[0x15][1] = op_ORA;
	op_handlers[0x16][0] = am_ZPX;
	op_handlers[0x16][1] = op_ASL;
	op_handlers[0x18][0] = am_IMP;
	op_handlers[0x18][1] = op_CLC;
	op_handlers[0x19][0] = am_ABY;
	op_handlers[0x19][1] = op_ORA;
	op_handlers[0x1D][0] = am_ABX;
	op_handlers[0x1D][1] = op_ORA;
	op_handlers[0x1E][0] = am_ABX;
	op_handlers[0x1E][1] = op_ASL;
	op_handlers[0x20][0] = am_ABS;
	op_handlers[0x20][1] = op_JSR;
	op_handlers[0x21][0] = am_IZX;
	op_handlers[0x21][1] = op_AND;
	op_handlers[0x24][0] = am_ZP;
	op_handlers[0x24][1] = op_BIT;
	op_handlers[0x25][0] = am_ZP;
	op_handlers[0x25][1] = op_AND;
	op_handlers[0x26][0] = am_ZP;
	op_handlers[0x26][1] = op_ROL;
	op_handlers[0x28][0] = am_IMP;
	op_handlers[0x28][1] = op_PLP;
	op_handlers[0x29][0] = am_IMM;
	op_handlers[0x29][1] = op_AND;
	op_handlers[0x2A][0] = am_ACC;
	op_handlers[0x2A][1] = op_ROL;
	op_handlers[0x2C][0] = am_ABS;
	op_handlers[0x2C][1] = op_BIT;
	op_handlers[0x2D][0] = am_ABS;
	op_handlers[0x2D][1] = op_AND;
	op_handlers[0x2E][0] = am_ABS;
	op_handlers[0x2E][1] = op_ROL;
	op_handlers[0x30][0] = am_REL;
	op_handlers[0x30][1] = op_BMI;
	op_handlers[0x31][0] = am_IZY;
	op_handlers[0x31][1] = op_AND;
	op_handlers[0x35][0] = am_ZPX;
	op_handlers[0x35][1] = op_AND;
	op_handlers[0x36][0] = am_ZPX;
	op_handlers[0x36][1] = op_ROL;
	op_handlers[0x38][0] = am_IMP;
	op_handlers[0x38][1] = op_SEC;
	op_handlers[0x39][0] = am_ABY;
	op_handlers[0x39][1] = op_AND;
	op_handlers[0x3D][0] = am_ABX;
	op_handlers[0x3D][1] = op_AND;
	op_handlers[0x3E][0] = am_ABX;
	op_handlers[0x3E][1] = op_ROL;
	op_handlers[0x40][0] = am_IMP;
	op_handlers[0x40][1] = op_RTI;
	op_handlers[0x41][0] = am_IZX;
	op_handlers[0x41][1] = op_EOR;
	op_handlers[0x45][0] = am_ZP;
	op_handlers[0x45][1] = op_EOR;
	op_handlers[0x46][0] = am_ZP;
	op_handlers[0x46][1] = op_LSR;
	op_handlers[0x48][0] = am_IMP;
	op_handlers[0x48][1] = op_PHA;
	op_handlers[0x49][0] = am_IMM;
	op_handlers[0x49][1] = op_EOR;
	op_handlers[0x4A][0] = am_ACC;
	op_handlers[0x4A][1] = op_LSR;
	op_handlers[0x4C][0] = am_ABS;
	op_handlers[0x4C][1] = op_JMP;
	op_handlers[0x4D][0] = am_ABS;
	op_handlers[0x4D][1] = op_EOR;
	op_handlers[0x4E][0] = am_ABS;
	op_handlers[0x4E][1] = op_LSR;
	op_handlers[0x50][0] = am_REL;
	op_handlers[0x50][1] = op_BVC;
	op_handlers[0x51][0] = am_IZY;
	op_handlers[0x51][1] = op_EOR;
	op_handlers[0x55][0] = am_ZPX;
	op_handlers[0x55][1] = op_EOR;
	op_handlers[0x56][0] = am_ZPX;
	op_handlers[0x56][1] = op_LSR;
	op_handlers[0x58][0] = am_IMP;
	op_handlers[0x58][1] = op_CLI;
	op_handlers[0x59][0] = am_ABY;
	op_handlers[0x59][1] = op_EOR;
	op_handlers[0x5D][0] = am_ABX;
	op_handlers[0x5D][1] = op_EOR;
	op_handlers[0x5E][0] = am_ABX;
	op_handlers[0x5E][1] = op_LSR;
	op_handlers[0x60][0] = am_IMP;
	op_handlers[0x60][1] = op_RTS;
	op_handlers[0x61][0] = am_IZX;
	op_handlers[0x61][1] = op_ADC;
	op_handlers[0x65][0] = am_ZP;
	op_handlers[0x65][1] = op_ADC;
	op_handlers[0x66][0] = am_ZP;
	op_handlers[0x66][1] = op_ROR;
	op_handlers[0x68][0] = am_IMP;
	op_handlers[0x68][1] = op_PLA;
	op_handlers[0x69][0] = am_IMM;
	op_handlers[0x69][1] = op_ADC;
	op_handlers[0x6A][0] = am_ACC;
	op_handlers[0x6A][1] = op_ROR;
	op_handlers[0x6C][0] = am_IND;
	op_handlers[0x6C][1] = op_JMP;
	op_handlers[0x6D][0] = am_ABS;
	op_handlers[0x6D][1] = op_ADC;
	op_handlers[0x6E][0] = am_ABS;
	op_handlers[0x6E][1] = op_ROR;
	op_handlers[0x70][0] = am_REL;
	op_handlers[0x70][1] = op_BVS;
	op_handlers[0x71][0] = am_IZY;
	op_handlers[0x71][1] = op_ADC;
	op_handlers[0x75][0] = am_ZPX;
	op_handlers[0x75][1] = op_ADC;
	op_handlers[0x76][0] = am_ZPX;
	op_handlers[0x76][1] = op_ROR;
	op_handlers[0x78][0] = am_IMP;
	op_handlers[0x78][1] = op_SEI;
	op_handlers[0x79][0] = am_ABY;
	op_handlers[0x79][1] = op_ADC;
	op_handlers[0x7D][0] = am_ABX;
	op_handlers[0x7D][1] = op_ADC;
	op_handlers[0x7E][0] = am_ABX;
	op_handlers[0x7E][1] = op_ROR;
	op_handlers[0x81][0] = am_IZX;
	op_handlers[0x81][1] = op_STA;
	op_handlers[0x84][0] = am_ZP;
	op_handlers[0x84][1] = op_STY;
	op_handlers[0x85][0] = am_ZP;
	op_handlers[0x85][1] = op_STA;
	op_handlers[0x86][0] = am_ZP;
	op_handlers[0x86][1] = op_STX;
	op_handlers[0x88][0] = am_IMP;
	op_handlers[0x88][1] = op_DEY;
	op_handlers[0x8A][0] = am_IMP;
	op_handlers[0x8A][1] = op_TXA;
	op_handlers[0x8C][0] = am_ABS;
	op_handlers[0x8C][1] = op_STY;
	op_handlers[0x8D][0] = am_ABS;
	op_handlers[0x8D][1] = op_STA;
	op_handlers[0x8E][0] = am_ABS;
	op_handlers[0x8E][1] = op_STX;
	op_handlers[0x90][0] = am_REL;
	op_handlers[0x90][1] = op_BCC;
	op_handlers[0x91][0] = am_IZY;
	op_handlers[0x91][1] = op_STA;
	op_handlers[0x94][0] = am_ZPX;
	op_handlers[0x94][1] = op_STY;
	op_handlers[0x95][0] = am_ZPX;
	op_handlers[0x95][1] = op_STA;
	op_handlers[0x96][0] = am_ZPY;
	op_handlers[0x96][1] = op_STX;
	op_handlers[0x98][0] = am_IMP;
	op_handlers[0x98][1] = op_TYA;
	op_handlers[0x99][0] = am_ABY;
	op_handlers[0x99][1] = op_STA;
	op_handlers[0x9A][0] = am_IMP;
	op_handlers[0x9A][1] = op_TXS;
	op_handlers[0x9D][0] = am_ABX;
	op_handlers[0x9D][1] = op_STA;
	op_handlers[0xA0][0] = am_IMM;
	op_handlers[0xA0][1] = op_LDY;
	op_handlers[0xA1][0] = am_IZX;
	op_handlers[0xA1][1] = op_LDA;
	op_handlers[0xA2][0] = am_IMM;
	op_handlers[0xA2][1] = op_LDX;
	op_handlers[0xA4][0] = am_ZP;
	op_handlers[0xA4][1] = op_LDY;
	op_handlers[0xA5][0] = am_ZP;
	op_handlers[0xA5][1] = op_LDA;
	op_handlers[0xA6][0] = am_ZP;
	op_handlers[0xA6][1] = op_LDX;
	op_handlers[0xA8][0] = am_IMP;
	op_handlers[0xA8][1] = op_TAY;
	op_handlers[0xA9][0] = am_IMM;
	op_handlers[0xA9][1] = op_LDA;
	op_handlers[0xAA][0] = am_IMP;
	op_handlers[0xAA][1] = op_TAX;
	op_handlers[0xAC][0] = am_ABS;
	op_handlers[0xAC][1] = op_LDY;
	op_handlers[0xAD][0] = am_ABS;
	op_handlers[0xAD][1] = op_LDA;
	op_handlers[0xAE][0] = am_ABS;
	op_handlers[0xAE][1] = op_LDX;
	op_handlers[0xB0][0] = am_REL;
	op_handlers[0xB0][1] = op_BCS;
	op_handlers[0xB1][0] = am_IZY;
	op_handlers[0xB1][1] = op_LDA;
	op_handlers[0xB4][0] = am_ZPX;
	op_handlers[0xB4][1] = op_LDY;
	op_handlers[0xB5][0] = am_ZPX;
	op_handlers[0xB5][1] = op_LDA;
	op_handlers[0xB6][0] = am_ZPY;
	op_handlers[0xB6][1] = op_LDX;
	op_handlers[0xB8][0] = am_IMP;
	op_handlers[0xB8][1] = op_CLV;
	op_handlers[0xB9][0] = am_ABY;
	op_handlers[0xB9][1] = op_LDA;
	op_handlers[0xBA][0] = am_IMP;
	op_handlers[0xBA][1] = op_TSX;
	op_handlers[0xBC][0] = am_ABX;
	op_handlers[0xBC][1] = op_LDY;
	op_handlers[0xBD][0] = am_ABX;
	op_handlers[0xBD][1] = op_LDA;
	op_handlers[0xBE][0] = am_ABY;
	op_handlers[0xBE][1] = op_LDX;
	op_handlers[0xC0][0] = am_IMM;
	op_handlers[0xC0][1] = op_CPY;
	op_handlers[0xC1][0] = am_IZX;
	op_handlers[0xC1][1] = op_CMP;
	op_handlers[0xC4][0] = am_ZP;
	op_handlers[0xC4][1] = op_CPY;
	op_handlers[0xC5][0] = am_ZP;
	op_handlers[0xC5][1] = op_CMP;
	op_handlers[0xC6][0] = am_ZP;
	op_handlers[0xC6][1] = op_DEC;
	op_handlers[0xC8][0] = am_IMP;
	op_handlers[0xC8][1] = op_INY;
	op_handlers[0xC9][0] = am_IMM;
	op_handlers[0xC9][1] = op_CMP;
	op_handlers[0xCA][0] = am_IMP;
	op_handlers[0xCA][1] = op_DEX;
	op_handlers[0xCC][0] = am_ABS;
	op_handlers[0xCC][1] = op_CPY;
	op_handlers[0xCD][0] = am_ABS;
	op_handlers[0xCD][1] = op_CMP;
	op_handlers[0xCE][0] = am_ABS;
	op_handlers[0xCE][1] = op_DEC;
	op_handlers[0xD0][0] = am_REL;
	op_handlers[0xD0][1] = op_BNE;
	op_handlers[0xD1][0] = am_IZY;
	op_handlers[0xD1][1] = op_CMP;
	op_handlers[0xD5][0] = am_ZPX;
	op_handlers[0xD5][1] = op_CMP;
	op_handlers[0xD6][0] = am_ZPX;
	op_handlers[0xD6][1] = op_DEC;
	op_handlers[0xD8][0] = am_IMP;
	op_handlers[0xD8][1] = op_CLD;
	op_handlers[0xD9][0] = am_ABY;
	op_handlers[0xD9][1] = op_CMP;
	op_handlers[0xDD][0] = am_ABX;
	op_handlers[0xDD][1] = op_CMP;
	op_handlers[0xDE][0] = am_ABX;
	op_handlers[0xDE][1] = op_DEC;
	op_handlers[0xE0][0] = am_IMM;
	op_handlers[0xE0][1] = op_CPX;
	op_handlers[0xE1][0] = am_IZX;
	op_handlers[0xE1][1] = op_SBC;
	op_handlers[0xE4][0] = am_ZP;
	op_handlers[0xE4][1] = op_CPX;
	op_handlers[0xE5][0] = am_ZP;
	op_handlers[0xE5][1] = op_SBC;
	op_handlers[0xE6][0] = am_ZP;
	op_handlers[0xE6][1] = op_INC;
	op_handlers[0xE8][0] = am_IMP;
	op_handlers[0xE8][1] = op_INX;
	op_handlers[0xE9][0] = am_IMM;
	op_handlers[0xE9][1] = op_SBC;
	op_handlers[0xEB][0] = am_IMM;
	op_handlers[0xEB][1] = op_SBC; // Official SBC alias
	op_handlers[0xEC][0] = am_ABS;
	op_handlers[0xEC][1] = op_CPX;
	op_handlers[0xED][0] = am_ABS;
	op_handlers[0xED][1] = op_SBC;
	op_handlers[0xEE][0] = am_ABS;
	op_handlers[0xEE][1] = op_INC;
	op_handlers[0xF0][0] = am_REL;
	op_handlers[0xF0][1] = op_BEQ;
	op_handlers[0xF1][0] = am_IZY;
	op_handlers[0xF1][1] = op_SBC;
	op_handlers[0xF5][0] = am_ZPX;
	op_handlers[0xF5][1] = op_SBC;
	op_handlers[0xF6][0] = am_ZPX;
	op_handlers[0xF6][1] = op_INC;
	op_handlers[0xF8][0] = am_IMP;
	op_handlers[0xF8][1] = op_SED;
	op_handlers[0xF9][0] = am_ABY;
	op_handlers[0xF9][1] = op_SBC;
	op_handlers[0xFD][0] = am_ABX;
	op_handlers[0xFD][1] = op_SBC;
	op_handlers[0xFE][0] = am_ABX;
	op_handlers[0xFE][1] = op_INC;
#if ENABLE_ILLEGAL_OPCODE
	// NOP variants (Reads memory, 2/3/4 cycles)
	// $02, $12, $22, $32, $42, $52, $62, $72, $92, $B2, $D2, $F2 are KIL (Jam)
	// Let's map NOPs first
	// NOP Immediate (2 cycles)
	op_handlers[0x80][0] = am_IMM;
	op_handlers[0x80][1] = op_NOP;
	op_handlers[0x82][0] = am_IMM;
	op_handlers[0x82][1] = op_NOP;
	op_handlers[0x89][0] = am_IMM;
	op_handlers[0x89][1] = op_NOP;
	op_handlers[0xC2][0] = am_IMM;
	op_handlers[0xC2][1] = op_NOP;
	op_handlers[0xE2][0] = am_IMM;
	op_handlers[0xE2][1] = op_NOP;
	// NOP Zero Page (3 cycles)
	op_handlers[0x04][0] = am_ZP;
	op_handlers[0x04][1] = op_NOP_RD;
	op_handlers[0x44][0] = am_ZP;
	op_handlers[0x44][1] = op_NOP_RD;
	op_handlers[0x64][0] = am_ZP;
	op_handlers[0x64][1] = op_NOP_RD;
	// NOP Zero Page X (4 cycles)
	op_handlers[0x14][0] = am_ZPX;
	op_handlers[0x14][1] = op_NOP_RD;
	op_handlers[0x34][0] = am_ZPX;
	op_handlers[0x34][1] = op_NOP_RD;
	op_handlers[0x54][0] = am_ZPX;
	op_handlers[0x54][1] = op_NOP_RD;
	op_handlers[0x74][0] = am_ZPX;
	op_handlers[0x74][1] = op_NOP_RD;
	op_handlers[0xD4][0] = am_ZPX;
	op_handlers[0xD4][1] = op_NOP_RD;
	op_handlers[0xF4][0] = am_ZPX;
	op_handlers[0xF4][1] = op_NOP_RD;
	// NOP Absolute (4 cycles)
	op_handlers[0x0C][0] = am_ABS;
	op_handlers[0x0C][1] = op_NOP_RD;
	// NOP Absolute X (4/5 cycles)
	op_handlers[0x1C][0] = am_ABX;
	op_handlers[0x1C][1] = op_NOP_RD;
	op_handlers[0x3C][0] = am_ABX;
	op_handlers[0x3C][1] = op_NOP_RD;
	op_handlers[0x5C][0] = am_ABX;
	op_handlers[0x5C][1] = op_NOP_RD;
	op_handlers[0x7C][0] = am_ABX;
	op_handlers[0x7C][1] = op_NOP_RD;
	op_handlers[0xDC][0] = am_ABX;
	op_handlers[0xDC][1] = op_NOP_RD;
	op_handlers[0xFC][0] = am_ABX;
	op_handlers[0xFC][1] = op_NOP_RD;
	// LAX
	op_handlers[0xA7][0] = am_ZP;
	op_handlers[0xA7][1] = op_LAX;
	op_handlers[0xB7][0] = am_ZPY;
	op_handlers[0xB7][1] = op_LAX;
	op_handlers[0xAF][0] = am_ABS;
	op_handlers[0xAF][1] = op_LAX;
	op_handlers[0xBF][0] = am_ABY;
	op_handlers[0xBF][1] = op_LAX;
	op_handlers[0xA3][0] = am_IZX;
	op_handlers[0xA3][1] = op_LAX;
	op_handlers[0xB3][0] = am_IZY;
	op_handlers[0xB3][1] = op_LAX;
	op_handlers[0xAB][0] = am_IMM;
	op_handlers[0xAB][1] = op_LAX;
	// SAX
	op_handlers[0x87][0] = am_ZP;
	op_handlers[0x87][1] = op_SAX;
	op_handlers[0x97][0] = am_ZPY;
	op_handlers[0x97][1] = op_SAX;
	op_handlers[0x8F][0] = am_ABS;
	op_handlers[0x8F][1] = op_SAX;
	op_handlers[0x83][0] = am_IZX;
	op_handlers[0x83][1] = op_SAX;
	// DCP
	op_handlers[0xC7][0] = am_ZP;
	op_handlers[0xC7][1] = op_DCP;
	op_handlers[0xD7][0] = am_ZPX;
	op_handlers[0xD7][1] = op_DCP;
	op_handlers[0xCF][0] = am_ABS;
	op_handlers[0xCF][1] = op_DCP;
	op_handlers[0xDF][0] = am_ABX;
	op_handlers[0xDF][1] = op_DCP;
	op_handlers[0xDB][0] = am_ABY;
	op_handlers[0xDB][1] = op_DCP;
	op_handlers[0xC3][0] = am_IZX;
	op_handlers[0xC3][1] = op_DCP;
	op_handlers[0xD3][0] = am_IZY;
	op_handlers[0xD3][1] = op_DCP;
	// ISB
	op_handlers[0xE7][0] = am_ZP;
	op_handlers[0xE7][1] = op_ISB;
	op_handlers[0xF7][0] = am_ZPX;
	op_handlers[0xF7][1] = op_ISB;
	op_handlers[0xEF][0] = am_ABS;
	op_handlers[0xEF][1] = op_ISB;
	op_handlers[0xFF][0] = am_ABX;
	op_handlers[0xFF][1] = op_ISB;
	op_handlers[0xFB][0] = am_ABY;
	op_handlers[0xFB][1] = op_ISB;
	op_handlers[0xE3][0] = am_IZX;
	op_handlers[0xE3][1] = op_ISB;
	op_handlers[0xF3][0] = am_IZY;
	op_handlers[0xF3][1] = op_ISB;
	// SLO
	op_handlers[0x07][0] = am_ZP;
	op_handlers[0x07][1] = op_SLO;
	op_handlers[0x17][0] = am_ZPX;
	op_handlers[0x17][1] = op_SLO;
	op_handlers[0x0F][0] = am_ABS;
	op_handlers[0x0F][1] = op_SLO;
	op_handlers[0x1F][0] = am_ABX;
	op_handlers[0x1F][1] = op_SLO;
	op_handlers[0x1B][0] = am_ABY;
	op_handlers[0x1B][1] = op_SLO;
	op_handlers[0x03][0] = am_IZX;
	op_handlers[0x03][1] = op_SLO;
	op_handlers[0x13][0] = am_IZY;
	op_handlers[0x13][1] = op_SLO;
	// RLA
	op_handlers[0x27][0] = am_ZP;
	op_handlers[0x27][1] = op_RLA;
	op_handlers[0x37][0] = am_ZPX;
	op_handlers[0x37][1] = op_RLA;
	op_handlers[0x2F][0] = am_ABS;
	op_handlers[0x2F][1] = op_RLA;
	op_handlers[0x3F][0] = am_ABX;
	op_handlers[0x3F][1] = op_RLA;
	op_handlers[0x3B][0] = am_ABY;
	op_handlers[0x3B][1] = op_RLA;
	op_handlers[0x23][0] = am_IZX;
	op_handlers[0x23][1] = op_RLA;
	op_handlers[0x33][0] = am_IZY;
	op_handlers[0x33][1] = op_RLA;
	// SRE
	op_handlers[0x47][0] = am_ZP;
	op_handlers[0x47][1] = op_SRE;
	op_handlers[0x57][0] = am_ZPX;
	op_handlers[0x57][1] = op_SRE;
	op_handlers[0x4F][0] = am_ABS;
	op_handlers[0x4F][1] = op_SRE;
	op_handlers[0x5F][0] = am_ABX;
	op_handlers[0x5F][1] = op_SRE;
	op_handlers[0x5B][0] = am_ABY;
	op_handlers[0x5B][1] = op_SRE;
	op_handlers[0x43][0] = am_IZX;
	op_handlers[0x43][1] = op_SRE;
	op_handlers[0x53][0] = am_IZY;
	op_handlers[0x53][1] = op_SRE;
	// RRA
	op_handlers[0x67][0] = am_ZP;
	op_handlers[0x67][1] = op_RRA;
	op_handlers[0x77][0] = am_ZPX;
	op_handlers[0x77][1] = op_RRA;
	op_handlers[0x6F][0] = am_ABS;
	op_handlers[0x6F][1] = op_RRA;
	op_handlers[0x7F][0] = am_ABX;
	op_handlers[0x7F][1] = op_RRA;
	op_handlers[0x7B][0] = am_ABY;
	op_handlers[0x7B][1] = op_RRA;
	op_handlers[0x63][0] = am_IZX;
	op_handlers[0x63][1] = op_RRA;
	op_handlers[0x73][0] = am_IZY;
	op_handlers[0x73][1] = op_RRA;
	// ANC
	op_handlers[0x0B][0] = am_IMM;
	op_handlers[0x0B][1] = op_ANC;
	op_handlers[0x2B][0] = am_IMM;
	op_handlers[0x2B][1] = op_ANC;
	// ALR
	op_handlers[0x4B][0] = am_IMM;
	op_handlers[0x4B][1] = op_ALR;
	// ARR
	op_handlers[0x6B][0] = am_IMM;
	op_handlers[0x6B][1] = op_ARR;
	// XAA
	op_handlers[0x8B][0] = am_IMM;
	op_handlers[0x8B][1] = op_XAA;
	// AXS
	op_handlers[0xCB][0] = am_IMM;
	op_handlers[0xCB][1] = op_AXS;
	// AHX
	op_handlers[0x9F][0] = am_ABY;
	op_handlers[0x9F][1] = op_AHX;
	op_handlers[0x93][0] = am_IZY;
	op_handlers[0x93][1] = op_AHX;
	// SHY
	op_handlers[0x9C][0] = am_ABX;
	op_handlers[0x9C][1] = op_SHY;
	// SHX
	op_handlers[0x9E][0] = am_ABY;
	op_handlers[0x9E][1] = op_SHX;
	// TAS
	op_handlers[0x9B][0] = am_ABY;
	op_handlers[0x9B][1] = op_TAS;
	// LAS
	op_handlers[0xBB][0] = am_ABY;
	op_handlers[0xBB][1] = op_LAS;
	// KIL / JAM
	op_handlers[0x02][0] = am_IMP;
	op_handlers[0x02][1] = op_KIL;
	op_handlers[0x12][0] = am_IMP;
	op_handlers[0x12][1] = op_KIL;
	op_handlers[0x22][0] = am_IMP;
	op_handlers[0x22][1] = op_KIL;
	op_handlers[0x32][0] = am_IMP;
	op_handlers[0x32][1] = op_KIL;
	op_handlers[0x42][0] = am_IMP;
	op_handlers[0x42][1] = op_KIL;
	op_handlers[0x52][0] = am_IMP;
	op_handlers[0x52][1] = op_KIL;
	op_handlers[0x62][0] = am_IMP;
	op_handlers[0x62][1] = op_KIL;
	op_handlers[0x72][0] = am_IMP;
	op_handlers[0x72][1] = op_KIL;
	op_handlers[0x92][0] = am_IMP;
	op_handlers[0x92][1] = op_KIL;
	op_handlers[0xB2][0] = am_IMP;
	op_handlers[0xB2][1] = op_KIL;
	op_handlers[0xD2][0] = am_IMP;
	op_handlers[0xD2][1] = op_KIL;
	op_handlers[0xF2][0] = am_IMP;
	op_handlers[0xF2][1] = op_KIL;
#endif
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

void map67_(int8_t page) { map_bank(page, 0); }
void map89_(int8_t page) { map_bank(page, 1); }
void mapAB_(int8_t page) { map_bank(page, 2); }
void mapCD_(int8_t page) { map_bank(page, 3); }
void mapEF_(int8_t page) { map_bank(page, 4); }

void cpu6502_init(void)
{
	memset(NES_RAM, 0, 0x800);
	uint32_t rom_offset = 16;
	if (RomHeader && (RomHeader->flags_1 & 0x04)) rom_offset += 512;
	uint32_t num_16k_banks = RomHeader ? RomHeader->num_16k_rom_banks : 2;
	uint32_t num_8k_banks = num_16k_banks * 2;
	CPU_ROM_banks[0] = NES_SRAM;
	CPU_ROM_banks[1] = romfile + rom_offset;
	CPU_ROM_banks[2] = romfile + rom_offset + 0x2000;
	uint32_t last_bank_idx = num_8k_banks - 1;
	uint32_t prev_bank_idx = (num_8k_banks > 1) ? (num_8k_banks - 2) : 0;
	CPU_ROM_banks[3] = romfile + rom_offset + (prev_bank_idx * 0x2000);
	CPU_ROM_banks[4] = romfile + rom_offset + (last_bank_idx * 0x2000);
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
	cpu_jam = 0;
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
		if (cpu_jam) return; // Stop if CPU is jammed
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
		g_page_crossed = 0;
		uint8_t opcode = K6502_Read(PC++);
		op_handlers[opcode][0]();
		op_handlers[opcode][1]();
		clocks += cycles_map[opcode];
	}
}
