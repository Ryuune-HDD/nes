/////////////////////////////////////////////////////////////////////
// Mapper 79
class NES_mapper79 : public NES_mapper
{
    friend void adopt_MPRD(SnssMapperBlock* block, NES* nes);
    friend int extract_MPRD(SnssMapperBlock* block, NES* nes);

public:
    NES_mapper79(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper79()
    {
    }

    void Reset();
    void MemoryWriteLow(uint32_t addr, uint8_t data);

protected:
private:
};

/////////////////////////////////////////////////////////////////////