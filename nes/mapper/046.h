/////////////////////////////////////////////////////////////////////
// Mapper 46
class NES_mapper46 : public NES_mapper
{
    friend void adopt_MPRD(SnssMapperBlock* block, NES* nes);
    friend int extract_MPRD(SnssMapperBlock* block, NES* nes);

public:
    NES_mapper46(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper46()
    {
    }

    void Reset();
    void MemoryWriteSaveRAM(uint32_t addr, uint8_t data);
    void MemoryWrite(uint32_t addr, uint8_t data);

protected:
    void set_rom_banks();
    uint8_t regs[4];

private:
};

/////////////////////////////////////////////////////////////////////