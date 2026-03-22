/////////////////////////////////////////////////////////////////////
// Mapper 189
class NES_mapper189 : public NES_mapper
{
    friend void adopt_MPRD(SnssMapperBlock* block, NES* nes);
    friend int extract_MPRD(SnssMapperBlock* block, NES* nes);

public:
    NES_mapper189(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper189()
    {
    }

    void Reset();
    void MemoryWriteLow(uint32_t addr, uint8_t data);
    void MemoryWrite(uint32_t addr, uint8_t data);
    void HSync(uint32_t scanline);

protected:
    uint8_t regs[1];
    uint8_t irq_counter;
    uint8_t irq_latch;
    uint8_t irq_enabled;

private:
};

/////////////////////////////////////////////////////////////////////