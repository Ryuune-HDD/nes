/////////////////////////////////////////////////////////////////////
// Mapper 118
class NES_mapper118 : public NES_mapper
{
    friend void adopt_MPRD(SnssMapperBlock* block, NES* nes);
    friend int extract_MPRD(SnssMapperBlock* block, NES* nes);

public:
    NES_mapper118(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper118()
    {
    }

    void Reset();
    void MemoryWrite(uint32_t addr, uint8_t data);
    void HSync(uint32_t scanline);

protected:
    uint8_t regs[8];
    uint32_t prg0, prg1;
    uint32_t chr0, chr1, chr2, chr3, chr4, chr5, chr6, chr7;
    uint32_t chr_swap() { return regs[0] & 0x80; }
    uint32_t prg_swap() { return regs[0] & 0x40; }
    uint8_t irq_enabled; // IRQs enabled
    uint8_t irq_counter; // IRQ scanline counter, decreasing
    uint8_t irq_latch; // IRQ scanline counter latch
    void MMC3_set_CPU_banks()
    {
        if (prg_swap())
        {
            set_CPU_banks(num_8k_ROM_banks - 2, prg1, prg0, num_8k_ROM_banks - 1);
        }
        else
        {
            set_CPU_banks(prg0, prg1, num_8k_ROM_banks - 2, num_8k_ROM_banks - 1);
        }
    }

    void MMC3_set_PPU_banks()
    {
        if (num_1k_VROM_banks)
        {
            if (chr_swap())
            {
                set_PPU_banks(chr4, chr5, chr6, chr7, chr0, chr1, chr2, chr3);
            }
            else
            {
                set_PPU_banks(chr0, chr1, chr2, chr3, chr4, chr5, chr6, chr7);
            }
        }
    }

    void SNSS_fixup();

private:
};

/////////////////////////////////////////////////////////////////////