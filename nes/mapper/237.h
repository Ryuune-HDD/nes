/////////////////////////////////////////////////////////////////////
// Mapper 237
class NES_mapper237 : public NES_mapper
{
    friend void adopt_MPRD(SnssMapperBlock* block, NES* nes);
    friend int extract_MPRD(SnssMapperBlock* block, NES* nes);

public:
    NES_mapper237(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper237()
    {
    }

    void Reset();
    void MemoryWrite(uint32_t addr, uint8_t data);

protected:
    uint8_t wram[0x8000];

private:
};

/////////////////////////////////////////////////////////////////////