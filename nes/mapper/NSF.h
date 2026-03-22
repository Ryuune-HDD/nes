/////////////////////////////////////////////////////////////////////
// Mapper NSF - private mapper number = 12 (decimal)
class NES_mapperNSF : public NES_mapper
{
    friend void adopt_MPRD(SnssMapperBlock* block, NES* nes);
    friend int extract_MPRD(SnssMapperBlock* block, NES* nes);

public:
    NES_mapperNSF(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapperNSF()
    {
    }

    void Reset();
    uint8_t MemoryReadLow(uint32_t addr);
    void MemoryWriteLow(uint32_t addr, uint8_t data);
    void MemoryWriteSaveRAM(uint32_t addr, uint8_t data);
    void MemoryWrite(uint32_t addr, uint8_t data);

protected:
    void BankSwitch(uint8_t num, uint8_t bank);
    void LoadPlayer();

    uint8_t wram1[0x2000];
    uint8_t wram2[0x8000];
    uint8_t chip_type;

private:
};

/////////////////////////////////////////////////////////////////////