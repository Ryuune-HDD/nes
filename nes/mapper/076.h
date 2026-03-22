/////////////////////////////////////////////////////////////////////
// Mapper 76
class NES_mapper76 : public NES_mapper
{
    friend void adopt_MPRD(SnssMapperBlock* block, NES* nes);
    friend int extract_MPRD(SnssMapperBlock* block, NES* nes);

public:
    NES_mapper76(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper76()
    {
    }

    void Reset();
    void MemoryWrite(uint32_t addr, uint8_t data);

protected:
    uint8_t regs[1];

private:
};

/////////////////////////////////////////////////////////////////////