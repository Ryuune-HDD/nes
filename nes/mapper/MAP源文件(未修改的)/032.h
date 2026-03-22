/////////////////////////////////////////////////////////////////////
// Mapper 32
class NES_mapper32 : public NES_mapper
{
    friend void adopt_MPRD(SnssMapperBlock* block, NES* nes);
    friend int extract_MPRD(SnssMapperBlock* block, NES* nes);

public:
    NES_mapper32(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper32()
    {
    }

    void Reset();
    void MemoryWrite(uint32_t addr, uint8_t data);

protected:
    uint8_t patch;
    uint8_t regs[1];

private:
};

/////////////////////////////////////////////////////////////////////