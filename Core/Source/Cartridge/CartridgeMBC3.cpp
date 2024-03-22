#include "RetroGBm/Pch.h"
#include "RetroGBm/Cartridge/CartridgeMBC3.h"

CartridgeMBC3::CartridgeMBC3(CartridgeDataV2 cartridge_data) : BaseCartridge(cartridge_data)
{
    m_ExternalRam.resize(cartridge_data.ram_size);
}

uint8_t CartridgeMBC3::Read(uint16_t address)
{
    if (address >= 0 && address <= 0x3FFF)
    {
        return m_CartridgeData.data[address];
    }
    else if (address >= 0x4000 && address <= 0x7FFF)
    {
        uint16_t bank_number = m_RomBank;
        int offset = ((address - 0x4000) + (0x4000 * bank_number)) % m_CartridgeData.data.size();
        return m_CartridgeData.data[offset];
    }
    else if (address >= 0xA000 && address <= 0xBFFF)
    {
        if (this->IsRamEnabled())
        {
            int offset = ((address - 0xA000) + (m_RamBank * 0x2000)) % m_ExternalRam.size();
            return m_ExternalRam[offset];
        }
    }

    return 0xFF;
}

void CartridgeMBC3::Write(uint16_t address, uint8_t value)
{
    if (address >= 0x0 && address <= 0x1FFF)
    {
        m_ExternalRamEnabled = (value & 0xF) == 0xA;
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
            int offset = ((address - 0xA000) + (m_RamBank * 0x2000)) % m_ExternalRam.size();
            m_ExternalRam[offset] = value;

            if (this->HasBattery())
            {
                m_WriteRamCallback();
            }
        }
    }
}

void CartridgeMBC3::SaveState(std::fstream* file)
{
    int ram_size = static_cast<int>(m_ExternalRam.size());
    file->write(reinterpret_cast<const char*>(&ram_size), sizeof(ram_size));
    file->write(reinterpret_cast<const char*>(m_ExternalRam.data()), m_ExternalRam.size() * sizeof(uint8_t));

    file->write(reinterpret_cast<const char*>(&m_ExternalRamEnabled), sizeof(m_ExternalRamEnabled));
    file->write(reinterpret_cast<const char*>(&m_RomBank), sizeof(m_RomBank));
    file->write(reinterpret_cast<const char*>(&m_RamBank), sizeof(m_RamBank));
}

void CartridgeMBC3::LoadState(std::fstream* file)
{
    int ram_size = 0;
    file->read(reinterpret_cast<char*>(&ram_size), sizeof(ram_size));

    m_ExternalRam.resize(ram_size);
    file->read(reinterpret_cast<char*>(m_ExternalRam.data()), m_ExternalRam.size() * sizeof(uint8_t));

    file->read(reinterpret_cast<char*>(&m_ExternalRamEnabled), sizeof(m_ExternalRamEnabled));
    file->read(reinterpret_cast<char*>(&m_RomBank), sizeof(m_RomBank));
    file->read(reinterpret_cast<char*>(&m_RamBank), sizeof(m_RamBank));
}