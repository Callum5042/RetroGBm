#include "RetroGBm/Pch.h"
#include "RetroGBm/Cartridge/CartridgeMBC1.h"

CartridgeMBC1::CartridgeMBC1(CartridgeDataV2 cartridge_data) : BaseCartridge(cartridge_data)
{
}

uint8_t CartridgeMBC1::Read(uint16_t address)
{
    if (address >= 0x0 && address <= 0x3FFF)
    {
        // Read from ROM
        if (m_BankMode == BankMode::ROM)
        {
            uint16_t bank_number = m_RomBank << 5;
            int offset = (address + (0x4000 * bank_number)) % m_CartridgeData.data.size();
            return m_CartridgeData.data[offset];
        }
        else if (m_BankMode == BankMode::RAM)
        {
            return m_CartridgeData.data[address];
        }
    }
    else if (address >= 0x4000 && address <= 0x7FFF)
    {
        // Read from ROM
        if (m_BankMode == BankMode::ROM)
        {
            uint8_t bank = m_RomBank;
            return m_CartridgeData.data[(address - 0x4000) + (0x4000 * bank)];
        }
        else if (m_BankMode == BankMode::RAM)
        {
            return m_CartridgeData.data[address];
        }
    }
    else if (address >= 0xA000 && address <= 0xBFFF)
    {
        if (HasRAM())
        {
            if (m_BankMode == BankMode::ROM)
            {
                return m_ExternalRam[address - 0xA000];
            }
            else if (m_BankMode == BankMode::RAM)
            {
                uint8_t bank = (m_RomBank >> 6);
                return m_ExternalRam[(address - 0xA000) + (bank * 0x2000)];
            }
        }
    }

    return 0xFF;
}

void CartridgeMBC1::Write(uint16_t address, uint8_t value)
{
    if (address >= 0 && address <= 0x1FFF)
    {
        m_ExternalRamEnabled = value == 0xA;
    }
    else if (address >= 0x2000 && address <= 0x3FFF)
    {
        // Cannot set rom bank to 0
        if (value == 0)
        {
            value = 1;
        }

        // Only write to the lower 5 bits
        m_RomBank &= ~0x1F;
        m_RomBank |= value & 0x1F;
    }
    else if (address >= 0x4000 && address <= 0x5FFF)
    {
        m_RomBank &= ~0xC0;
        m_RomBank |= (value & 0x3) << 6;
    }
    else if (address >= 0x6000 && address <= 0x7FFF)
    {
        m_BankMode = static_cast<BankMode>(value & 0x1);
    }
}
 