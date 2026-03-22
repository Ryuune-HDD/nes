/////////////////////////////////////////////////////////////////////
// Mapper 91
class NES_mapper91 : public NES_mapper
{
    friend void adopt_MPRD(SnssMapperBlock* block, NES* nes);
    friend int extract_MPRD(SnssMapperBlock* block, NES* nes);

public:
    NES_mapper91(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper91()
    {
    }

    void Reset();
    void MemoryWriteSaveRAM(uint32_t addr, uint8_t data);
    void HSync(uint32_t scanline);

protected:
    uint8_t irq_counter;
    uint8_t irq_enabled;

private:
};

/////////////////////////////////////////////////////////////////////