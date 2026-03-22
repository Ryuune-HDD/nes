/////////////////////////////////////////////////////////////////////
// Mapper 7
class NES_mapper7 : public NES_mapper
{
public:
    NES_mapper7(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper7()
    {
    }

    void Reset();

    void MemoryWrite(uint32_t addr, uint8_t data);

protected:
private:
};

/////////////////////////////////////////////////////////////////////