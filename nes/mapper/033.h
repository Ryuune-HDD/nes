/////////////////////////////////////////////////////////////////////
// Mapper 33
class NES_mapper33 : public NES_mapper
{
    friend void adopt_MPRD(SnssMapperBlock* block, NES* nes);
    friend int extract_MPRD(SnssMapperBlock* block, NES* nes);

public:
    NES_mapper33(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper33()
    {
    }

    void Reset();

    void MemoryWrite(uint32_t addr, uint8_t data);
    void HSync(uint32_t scanline);

protected:
    uint8_t patch;
    uint8_t irq_enabled;
    uint8_t irq_counter;

private:
};

/////////////////////////////////////////////////////////////////////