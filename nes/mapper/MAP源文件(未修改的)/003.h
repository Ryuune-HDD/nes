/////////////////////////////////////////////////////////////////////
// Mapper 3
class NES_mapper3 : public NES_mapper
{
public:
    NES_mapper3(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper3()
    {
    }

    void Reset();
    void MemoryWrite(uint32_t addr, uint8_t data);

protected:
private:
};

/////////////////////////////////////////////////////////////////////