/////////////////////////////////////////////////////////////////////
// Mapper 188
class NES_mapper188 : public NES_mapper
{
public:
    NES_mapper188(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper188()
    {
    }

    void Reset();
    void MemoryWrite(uint32_t addr, uint8_t data);

protected:
    uint8_t dummy[0x2000];

private:
};

/////////////////////////////////////////////////////////////////////