/////////////////////////////////////////////////////////////////////
// Mapper 96
class NES_mapper96 : public NES_mapper
{
    friend void adopt_MPRD(SnssMapperBlock* block, NES* nes);
    friend int extract_MPRD(SnssMapperBlock* block, NES* nes);

    friend void adopt_ExMPRD(const char* fn, NES* nes);
    friend void extract_ExMPRD(const char* fn, NES* nes);

public:
    NES_mapper96(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper96()
    {
    }

    void Reset();
    void MemoryWrite(uint32_t addr, uint8_t data);
    void PPU_Latch_Address(uint32_t addr);

protected:
    void sync_PPU_banks();
    uint8_t vbank0, vbank1;

private:
};

/////////////////////////////////////////////////////////////////////