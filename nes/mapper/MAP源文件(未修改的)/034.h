/////////////////////////////////////////////////////////////////////
// Mapper 34
class NES_mapper34 : public NES_mapper
{
    friend void adopt_MPRD(SnssMapperBlock* block, NES* nes);
    friend int extract_MPRD(SnssMapperBlock* block, NES* nes);

public:
    NES_mapper34(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper34()
    {
    }

    void Reset();

    void MemoryWriteSaveRAM(uint32_t addr, uint8_t data);
    void MemoryWrite(uint32_t addr, uint8_t data);

protected:
private:
};

/////////////////////////////////////////////////////////////////////