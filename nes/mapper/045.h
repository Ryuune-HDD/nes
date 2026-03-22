/////////////////////////////////////////////////////////////////////
// Mapper 45
class NES_mapper45 : public NES_mapper
{
    friend void adopt_MPRD(SnssMapperBlock* block, NES* nes);
    friend int extract_MPRD(SnssMapperBlock* block, NES* nes);

public:
    NES_mapper45(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper45()
    {
    }

    void Reset();
    void MemoryWriteSaveRAM(uint32_t addr, uint8_t data);
    void MemoryWrite(uint32_t addr, uint8_t data);
    void HSync(uint32_t scanline);

protected:
    uint8_t patch;

    uint8_t regs[7];
    uint32_t p[4], prg0, prg1, prg2, prg3;
    uint32_t c[8], chr0, chr1, chr2, chr3, chr4, chr5, chr6, chr7;

    uint8_t irq_enabled;
    uint8_t irq_counter;
    uint8_t irq_latch;

    void MAP45_set_CPU_bank4(uint8_t data);
    void MAP45_set_CPU_bank5(uint8_t data);
    void MAP45_set_CPU_bank6(uint8_t data);
    void MAP45_set_CPU_bank7(uint8_t data);
    void MAP45_set_PPU_banks();

private:
};

/////////////////////////////////////////////////////////////////////