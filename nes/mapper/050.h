/////////////////////////////////////////////////////////////////////
// Mapper 50
class NES_mapper50 : public NES_mapper
{
    friend void adopt_MPRD(SnssMapperBlock* block, NES* nes);
    friend int extract_MPRD(SnssMapperBlock* block, NES* nes);

public:
    NES_mapper50(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper50()
    {
    }

    void Reset();
    void MemoryWriteLow(uint32_t addr, uint8_t data);
    void HSync(uint32_t scanline);

protected:
    uint8_t irq_enabled;

private:
};

/////////////////////////////////////////////////////////////////////