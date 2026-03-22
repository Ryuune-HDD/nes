/////////////////////////////////////////////////////////////////////
// Mapper 2
class NES_mapper2 : public NES_mapper
{
public:
    NES_mapper2(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper2()
    {
    }

    void Reset();
    void MemoryWrite(uint32_t addr, uint8_t data);

protected:
private:
};

/////////////////////////////////////////////////////////////////////