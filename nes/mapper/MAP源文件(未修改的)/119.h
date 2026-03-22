/////////////////////////////////////////////////////////////////////
// Mapper 119
class NES_mapper119 : public NES_mapper
{
    friend void adopt_MPRD(SnssMapperBlock* block, NES* nes);
    friend int extract_MPRD(SnssMapperBlock* block, NES* nes);

public:
    NES_mapper119(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper119()
    {
    }

    void Reset();

    void MemoryWrite(uint32_t addr, uint8_t data);
    void HSync(uint32_t scanline);

protected:
    uint8_t regs[8];

    uint32_t prg0, prg1;
    uint32_t chr01, chr23, chr4, chr5, chr6, chr7;

    uint32_t chr_swap() { return regs[0] & 0x80; }
    uint32_t prg_swap() { return regs[0] & 0x40; }

    uint8_t irq_enabled; // IRQs enabled
    uint8_t irq_counter; // IRQ scanline counter, decreasing
    uint8_t irq_latch; // IRQ scanline counter latch

    void MMC3_set_CPU_banks();
    void MMC3_set_PPU_banks();

private:
};

/////////////////////////////////////////////////////////////////////