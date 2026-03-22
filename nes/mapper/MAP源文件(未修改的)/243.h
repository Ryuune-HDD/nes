/////////////////////////////////////////////////////////////////////
// Mapper 243
class NES_mapper243 : public NES_mapper
{
    friend void adopt_MPRD(SnssMapperBlock* block, NES* nes);
    friend int extract_MPRD(SnssMapperBlock* block, NES* nes);

public:
    NES_mapper243(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper243()
    {
    }

    void Reset();
    void MemoryWriteLow(uint32_t addr, uint8_t data);
    void MemoryWriteSaveRAM(uint32_t addr, uint8_t data);

protected:
    uint8_t regs[4];

private:
};

/////////////////////////////////////////////////////////////////////