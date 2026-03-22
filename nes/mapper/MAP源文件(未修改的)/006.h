/////////////////////////////////////////////////////////////////////
// Mapper 6
class NES_mapper6 : public NES_mapper
{
    friend void adopt_MPRD(SnssMapperBlock* block, NES* nes);
    friend int extract_MPRD(SnssMapperBlock* block, NES* nes);

    friend void adopt_ExMPRD(const char* fn, NES* nes);
    friend void extract_ExMPRD(const char* fn, NES* nes);

public:
    NES_mapper6(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper6()
    {
    }

    void Reset();
    void MemoryWriteLow(uint32_t addr, uint8_t data);
    void MemoryWrite(uint32_t addr, uint8_t data);
    void HSync(uint32_t scanline);

protected:
    uint8_t irq_enabled;
    uint32_t irq_counter;
    uint8_t chr_ram[4 * 0x2000];

private:
};

/////////////////////////////////////////////////////////////////////