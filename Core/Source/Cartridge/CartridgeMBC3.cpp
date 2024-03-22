#include "RetroGBm/Pch.h"
#include "RetroGBm/Cartridge/CartridgeMBC3.h"

CartridgeMBC3::CartridgeMBC3(CartridgeDataV2 cartridge_data) : BaseCartridge(cartridge_data)
{
}

uint8_t CartridgeMBC3::Read(uint16_t address)
{
    if (address >= 0 && address <= 0x3FFF)
    {
        return m_CartridgeData.data[address];
    }
    else if (address >= 0x4000 && address <= 0x7FFF)
    {
        return m_CartridgeData.data[address + (0x2000 * (m_RomBank - 1))];
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

void CartridgeMBC3::Write(uint16_t address, uint8_t value)
{
    if (address >= 0x0 && address <= 0x1FFF)
    {
        m_ExternalRamEnabled = value == 0xA;
        // TODO: Enable RTC here too (RTC)
    }
    else if (address >= 0x2000 && address <= 0x3FFF)
    {
        // Bank must be at least 1
        if (value == 0)
        {
            value = 1;
        }

        // Sets RAM bank to lower 7 bits
        m_RomBank = value & 0x7F;
    }

    else if (address >= 0x4000 && address <= 0x5FFF)
    {
        if (value <= 3)
        {
            // Only set RAM bank if value is less then 3
            m_RamBank = value & 0x3;
        }
    }
    else if (address >= 0xA000 && address <= 0xBFFF)
    {
        // Writes to RAM
        if (this->IsRamEnabled())
        {
            m_ExternalRam[(address - 0xA000) + (m_RamBank * 0x2000)] = value;
        }
    }
}
