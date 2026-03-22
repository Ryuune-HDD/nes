/////////////////////////////////////////////////////////////////////
// Mapper 64
class NES_mapper64 : public NES_mapper
{
    friend void adopt_MPRD(SnssMapperBlock* block, NES* nes);
    friend int extract_MPRD(SnssMapperBlock* block, NES* nes);

public:
    NES_mapper64(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper64()
    {
    }

    void Reset();
    void MemoryWrite(uint32_t addr, uint8_t data);
    void HSync(uint32_t scanline);

protected:
    uint8_t regs[3];
    uint8_t irq_latch;
    uint8_t irq_counter;
    uint8_t irq_enabled;

private:
};

/////////////////////////////////////////////////////////////////////