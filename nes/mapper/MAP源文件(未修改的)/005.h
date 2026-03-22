/////////////////////////////////////////////////////////////////////
// Mapper 5
class NES_mapper5 : public NES_mapper
{
    friend void adopt_MPRD(SnssMapperBlock* block, NES* nes);
    friend int extract_MPRD(SnssMapperBlock* block, NES* nes);

    friend void adopt_ExMPRD(const char* fn, NES* nes);
    friend void extract_ExMPRD(const char* fn, NES* nes);

public:
    NES_mapper5(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper5()
    {
    }

    void Reset();

    uint8_t MemoryReadLow(uint32_t addr);
    void MemoryWriteLow(uint32_t addr, uint8_t data);
    void MemoryWriteSaveRAM(uint32_t addr, uint8_t data);
    void MemoryWrite(uint32_t addr, uint8_t data);
    void HSync(uint32_t scanline);
    uint8_t PPU_Latch_RenderScreen(uint8_t mode, uint32_t addr);

protected:
    void MMC5_set_CPU_bank(uint8_t page, uint8_t bank);
    void MMC5_set_WRAM_bank(uint8_t page, uint8_t bank);
    void sync_Chr_banks(uint8_t page);

    uint32_t wb[8];
    uint8_t wram[8 * 0x2000];
    uint8_t wram_size;

    uint8_t chr_reg[8][2];

    uint8_t irq_enabled;
    uint8_t irq_status;
    uint32_t irq_line;

    uint32_t value0;
    uint32_t value1;

    uint8_t wram_protect0;
    uint8_t wram_protect1;
    uint8_t prg_size;
    uint8_t chr_size;
    uint8_t gfx_mode;
    uint8_t split_control;
    uint8_t split_bank;

private:
};

/////////////////////////////////////////////////////////////////////