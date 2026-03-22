/////////////////////////////////////////////////////////////////////
// Mapper 43
class NES_mapper43 : public NES_mapper
{
    friend void adopt_MPRD(SnssMapperBlock* block, NES* nes);
    friend int extract_MPRD(SnssMapperBlock* block, NES* nes);

public:
    NES_mapper43(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper43()
    {
    }

    void Reset();
    uint8_t MemoryReadLow(uint32_t addr);
    void MemoryWriteLow(uint32_t addr, uint8_t data);
    void MemoryWrite(uint32_t addr, uint8_t data);
    void HSync(uint32_t scanline);

protected:
    uint8_t irq_enabled;
    uint32_t irq_counter;

private:
};

/////////////////////////////////////////////////////////////////////