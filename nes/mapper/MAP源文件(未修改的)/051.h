/////////////////////////////////////////////////////////////////////
// Mapper 51
class NES_mapper51 : public NES_mapper
{
    friend void adopt_MPRD(SnssMapperBlock* block, NES* nes);
    friend int extract_MPRD(SnssMapperBlock* block, NES* nes);

public:
    NES_mapper51(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper51()
    {
    }

    void Reset();
    void MemoryWriteSaveRAM(uint32_t addr, uint8_t data);
    void MemoryWrite(uint32_t addr, uint8_t data);

protected:
    void Sync_Prg_Banks();
    uint8_t bank, mode;

private:
};

/////////////////////////////////////////////////////////////////////