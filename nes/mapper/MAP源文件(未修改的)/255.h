/////////////////////////////////////////////////////////////////////
// Mapper 255
class NES_mapper255 : public NES_mapper
{
    friend void adopt_MPRD(SnssMapperBlock* block, NES* nes);
    friend int extract_MPRD(SnssMapperBlock* block, NES* nes);

public:
    NES_mapper255(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper255()
    {
    }

    void Reset();
    uint8_t MemoryReadLow(uint32_t);
    void MemoryWriteLow(uint32_t addr, uint8_t data);
    void MemoryWrite(uint32_t addr, uint8_t data);

protected:
    uint8_t regs[4];

private:
};

/////////////////////////////////////////////////////////////////////