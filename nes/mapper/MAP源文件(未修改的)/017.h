/////////////////////////////////////////////////////////////////////
// Mapper 17
class NES_mapper17 : public NES_mapper
{
    friend void adopt_MPRD(SnssMapperBlock* block, NES* nes);
    friend int extract_MPRD(SnssMapperBlock* block, NES* nes);

public:
    NES_mapper17(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper17()
    {
    }

    void Reset();
    void MemoryWriteLow(uint32_t addr, uint8_t data);
    void HSync(uint32_t scanline);

protected:
    uint8_t irq_enabled;
    uint32_t irq_counter;
    uint32_t irq_latch;

private:
};

/////////////////////////////////////////////////////////////////////