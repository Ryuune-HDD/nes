/////////////////////////////////////////////////////////////////////
// Mapper 20
class NES_mapper20 : public NES_mapper
{
    friend void adopt_MPRD(SnssMapperBlock* block, NES* nes);
    friend int extract_MPRD(SnssMapperBlock* block, NES* nes);

    friend void adopt_ExMPRD(const char* fn, NES* nes);
    friend void extract_ExMPRD(const char* fn, NES* nes);

public:
    NES_mapper20(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper20()
    {
    }

    void Reset();
    uint8_t MemoryReadLow(uint32_t addr);
    void MemoryWriteLow(uint32_t addr, uint8_t data);
    void MemoryWriteSaveRAM(uint32_t addr, uint8_t data);
    void MemoryWrite(uint32_t addr, uint8_t data);
    void HSync(uint32_t scanline);
    void VSync();

    uint8_t GetDiskSideNum() { return ROM_banks[4]; }
    uint8_t GetDiskSide() { return current_side; }

    void SetDiskSide(uint8_t side)
    {
        if (side <= ROM_banks[4])
        {
            current_side = side;
            insert_wait = 0;
        }
    }

    uint8_t GetDiskData(uint32_t pt) { return disk[pt]; }
    void SetDiskData(uint32_t pt, uint8_t data) { disk[pt] = data; }

    uint8_t DiskAccessed()
    {
        uint8_t retval = access_flag;
        access_flag = 0;
        return retval;
    }

protected:
    uint8_t bios[0x2000];
    uint8_t wram[0x8000];
    uint8_t disk[0x40000];

    uint8_t irq_enabled;
    uint32_t irq_counter;
    uint32_t irq_latch;
    uint8_t irq_wait;

    uint8_t disk_enabled;
    uint32_t head_position;
    uint8_t write_skip;
    uint8_t disk_status;
    uint8_t write_reg;
    uint8_t current_side;

    uint8_t access_flag;
    uint8_t last_side;
    uint8_t insert_wait;

    void SNSS_fixup();

private:
};

/////////////////////////////////////////////////////////////////////