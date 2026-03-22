/////////////////////////////////////////////////////////////////////
// Mapper 16
class NES_mapper16 : public NES_mapper
{
    friend void adopt_MPRD(SnssMapperBlock* block, NES* nes);
    friend int extract_MPRD(SnssMapperBlock* block, NES* nes);

public:
    NES_mapper16(NES* parent) : NES_mapper(parent)
    {
    }

    ~NES_mapper16()
    {
    }

    void Reset();
    void MemoryWriteSaveRAM(uint32_t addr, uint8_t data);
    void MemoryWrite(uint32_t addr, uint8_t data);
    void MemoryReadSaveRAM(uint32_t addr);
    void HSync(uint32_t scanline);
    void SetBarcodeValue(uint32_t value_low, uint32_t value_high);

protected:
    void MemoryWrite2(uint32_t addr, uint8_t data);
    void MemoryWrite3(uint32_t addr, uint8_t data);

    uint8_t patch;
    uint8_t regs[3];

    uint8_t serial_out[0x2000];

    uint8_t eeprom_cmd[4];
    uint8_t eeprom_status;
    uint8_t eeprom_mode;
    uint8_t eeprom_pset;
    uint8_t eeprom_addr, eeprom_data;
    uint8_t eeprom_wbit, eeprom_rbit;

    uint8_t barcode[256];
    uint8_t barcode_status;
    uint8_t barcode_pt;
    uint8_t barcode_pt_max;
    uint8_t barcode_phase;
    uint32_t barcode_wait;
    uint8_t barcode_enabled;

    uint8_t irq_enabled;
    uint32_t irq_counter;
    uint32_t irq_latch;

private:
};

/////////////////////////////////////////////////////////////////////