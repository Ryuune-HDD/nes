/////////////////////////////////////////////////////////////////////
// Mapper 13
class NES_mapper13 : public NES_mapper
{
    friend void adopt_MPRD(SnssMapperBlock* block, NES* nes);
    friend int extract_MPRD(SnssMapperBlock* block, NES* nes);

    friend void adopt_ExMPRD(const char* fn, NES* nes);
    friend void extract_ExMPRD(const char* fn, NES* nes);

public:
    NES_mapper13(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper13()
    {
    }

    void Reset();

    void MemoryWrite(uint32_t addr, uint8_t data);

protected:
    uint8_t prg_bank;
    uint8_t chr_bank;

    void SNSS_fixup(); // HACK HACK HACK HACK

private:
};

/////////////////////////////////////////////////////////////////////