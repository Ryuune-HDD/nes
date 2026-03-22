/////////////////////////////////////////////////////////////////////
// Mapper 49
class NES_mapper49 : public NES_mapper
{
    friend void adopt_MPRD(SnssMapperBlock* block, NES* nes);
    friend int extract_MPRD(SnssMapperBlock* block, NES* nes);

public:
    NES_mapper49(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper49()
    {
    }

    void Reset();
    void MemoryWriteSaveRAM(uint32_t addr, uint8_t data);
    void MemoryWrite(uint32_t addr, uint8_t data);
    void HSync(uint32_t scanline);

protected:
    uint8_t regs[3];
    uint32_t prg0, prg1;
    uint32_t chr01, chr23, chr4, chr5, chr6, chr7;

    uint8_t irq_enabled;
    uint8_t irq_counter;
    uint8_t irq_latch;

    void MMC3_set_CPU_banks();
    void MMC3_set_PPU_banks();

    void SNSS_fixup();

private:
};

/////////////////////////////////////////////////////////////////////