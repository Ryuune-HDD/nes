/////////////////////////////////////////////////////////////////////
// Mapper 82
class NES_mapper82 : public NES_mapper
{
    friend void adopt_MPRD(SnssMapperBlock* block, NES* nes);
    friend int extract_MPRD(SnssMapperBlock* block, NES* nes);

public:
    NES_mapper82(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper82()
    {
    }

    void Reset();
    void MemoryWriteSaveRAM(uint32_t addr, uint8_t data);

protected:
    uint8_t regs[1];

private:
};

/////////////////////////////////////////////////////////////////////