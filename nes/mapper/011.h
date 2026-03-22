/////////////////////////////////////////////////////////////////////
// Mapper 11
class NES_mapper11 : public NES_mapper
{
    friend void adopt_MPRD(SnssMapperBlock* block, NES* nes);
    friend int extract_MPRD(SnssMapperBlock* block, NES* nes);

public:
    NES_mapper11(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper11()
    {
    }

    void Reset();

    void MemoryWrite(uint32_t addr, uint8_t data);

protected:
private:
};

/////////////////////////////////////////////////////////////////////