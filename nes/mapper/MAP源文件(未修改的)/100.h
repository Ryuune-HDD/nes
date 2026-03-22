/////////////////////////////////////////////////////////////////////
// Mapper 100
class NES_mapper100 : public NES_mapper
{
    friend void adopt_MPRD(SnssMapperBlock* block, NES* nes);
    friend int extract_MPRD(SnssMapperBlock* block, NES* nes);

public:
    NES_mapper100(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper100()
    {
    }

    void Reset();
    void MemoryWrite(uint32_t addr, uint8_t data);
    void HSync(uint32_t scanline);

protected:
    uint8_t regs[8];

    uint32_t prg0, prg1, prg2, prg3;
    uint32_t chr0, chr1, chr2, chr3, chr4, chr5, chr6, chr7;

    uint8_t irq_enabled; // IRQs enabled
    uint8_t irq_counter; // IRQ scanline counter, decreasing
    uint8_t irq_latch; // IRQ scanline counter latch

    void MMC3_set_CPU_banks()
    {
        set_CPU_banks(prg0, prg1, prg2, prg3);
    }

    void MMC3_set_PPU_banks()
    {
        if (num_1k_VROM_banks)
        {
            set_PPU_banks(chr0, chr1, chr2, chr3, chr4, chr5, chr6, chr7);
        }
    }

private:
};

/////////////////////////////////////////////////////////////////////