/////////////////////////////////////////////////////////////////////
// Mapper 90
class NES_mapper90 : public NES_mapper
{
    friend void adopt_MPRD(SnssMapperBlock* block, NES* nes);
    friend int extract_MPRD(SnssMapperBlock* block, NES* nes);

public:
    NES_mapper90(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper90()
    {
    }

    void Reset();
    uint8_t MemoryReadLow(uint32_t addr);
    void MemoryWriteLow(uint32_t addr, uint8_t data);
    void MemoryWrite(uint32_t addr, uint8_t data);
    void HSync(uint32_t scanline);

protected:
    void Sync_Prg_Banks();
    void Sync_Chr_Banks();
    void Sync_Mirror();

    uint8_t prg_reg[4];
    uint8_t chr_low_reg[8];
    uint8_t chr_high_reg[8];
    uint8_t nam_low_reg[4];
    uint8_t nam_high_reg[4];

    uint8_t prg_bank_size;
    uint8_t prg_bank_6000;
    uint8_t prg_bank_e000;
    uint8_t chr_bank_size;
    uint8_t mirror_mode;
    uint8_t mirror_type;

    uint32_t value1;
    uint32_t value2;

    uint8_t irq_enabled;
    uint8_t irq_counter;
    uint8_t irq_latch;

private:
};

/////////////////////////////////////////////////////////////////////