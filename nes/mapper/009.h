/////////////////////////////////////////////////////////////////////
// Mapper 9
class NES_mapper9 : public NES_mapper
{
    friend void adopt_MPRD(SnssMapperBlock* block, NES* nes);
    friend int extract_MPRD(SnssMapperBlock* block, NES* nes);

public:
    NES_mapper9(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper9()
    {
    }

    void Reset();

    void MemoryWrite(uint32_t addr, uint8_t data);

    void PPU_Latch_FDFE(uint32_t addr);

protected:
    uint8_t regs[6];
    uint8_t latch_0000;
    uint8_t latch_1000;

    void set_VROM_0000();
    void set_VROM_1000();

    void SNSS_fixup(); // HACK HACK HACK HACK

private:
};

/////////////////////////////////////////////////////////////////////