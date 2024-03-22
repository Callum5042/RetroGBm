#include "RetroGBm/Pch.h"
#include "RetroGBm/Cartridge/CartridgeMBC5.h"

CartridgeMBC5::CartridgeMBC5(CartridgeDataV2 cartridge_data) : BaseCartridge(cartridge_data)
{
}

uint8_t CartridgeMBC5::Read(uint16_t address)
{
    if (address >= 0x0 && address <= 0x3FFF)
    {
        return m_CartridgeData.data[address];
    }
    else if (address >= 0x4000 && address <= 0x7FFF)
    {
        return m_CartridgeData.data[(address) + (0x4000 * m_RomBank)];
    }
    else if (address >= 0xA000 && address <= 0xBFFF)
    {
        if (this->IsRamEnabled())
        {
            return m_ExternalRam[(address - 0xA000) + (m_RamBank * 0x2000)];
        }
    }

    return 0xFF;
}

void CartridgeMBC5::Write(uint16_t address, uint8_t value)
{
    if (address >= 0x0 && address <= 0x1FFF)
    {
        m_ExternalRamEnabled = value == 0xA;
    }
    else if (address >= 0x2000 && address <= 0x2FFF)
    {
        // Sets bit 0-7
        m_RomBank &= ~0xFF;
        m_RomBank |= value;
    }
    else if (address >= 0x3000 && address <= 0x3FFF)
    {
        // Set bit 8
        m_RomBank &= ~0x100;
        m_RomBank |= (value << 8);
    }
    else if (address >= 0x4000 && address <= 0x5FFF)
    {
        if (value <= 0xF)
        {
            // Only write values less than F to RAM bank
            m_RamBank = value;
        }
    }
}
