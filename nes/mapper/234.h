/////////////////////////////////////////////////////////////////////
// Mapper 234
class NES_mapper234 : public NES_mapper
{
    friend void adopt_MPRD(SnssMapperBlock* block, NES* nes);
    friend int extract_MPRD(SnssMapperBlock* block, NES* nes);

public:
    NES_mapper234(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper234()
    {
    }

    void Reset();
    void MemoryReadSaveRAM(uint32_t addr);
    void MemoryWrite(uint32_t addr, uint8_t data);

protected:
    void Sync();
    uint8_t regs[3];

private:
};

/////////////////////////////////////////////////////////////////////