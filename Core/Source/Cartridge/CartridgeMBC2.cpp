#include "RetroGBm/Pch.h"
#include "RetroGBm/Cartridge/CartridgeMBC2.h"

CartridgeMBC2::CartridgeMBC2(CartridgeDataV2 cartridge_data) : BaseCartridge(cartridge_data)
{
	// The header reports no RAM for MBC2, the RAM is always present in the mapper
	m_ExternalRam.resize(RamSize);
	std::fill(m_ExternalRam.begin(), m_ExternalRam.end(), 0x0);
}

uint8_t CartridgeMBC2::Read(uint16_t address)
{
	if (address >= 0x0 && address <= 0x3FFF)
	{
		// Bank 0 is always mapped here
		return m_CartridgeData.data[address % m_CartridgeData.data.size()];
	}
	else if (address >= 0x4000 && address <= 0x7FFF)
	{
		int offset = ((address - 0x4000) + (0x4000 * m_RomBank)) % m_CartridgeData.data.size();
		return m_CartridgeData.data[offset];
	}
	else if (address >= 0xA000 && address <= 0xBFFF)
	{
		if (this->IsRamEnabled())
		{
			// Only the bottom 9 bits are decoded so the 512 bytes echo across the range.
			// RAM is 4 bits wide, the upper nibble is open bus and reads back as 1s.
			return m_ExternalRam[(address & 0x1FF) % m_ExternalRam.size()] | 0xF0;
		}
	}

	return 0xFF;
}

void CartridgeMBC2::Write(uint16_t address, uint8_t value)
{
	if (address >= 0x0 && address <= 0x3FFF)
	{
		// Address bit 8 selects which register is written
		if ((address & 0x100) == 0)
		{
			m_ExternalRamEnabled = (value & 0xF) == 0xA;

			// Save to file each time we disable the ram
			if (this->HasBattery() && !m_ExternalRamEnabled && m_WriteRamCallback)
			{
				m_WriteRamCallback();
			}
		}
		else
		{
			// 4-bit ROM bank, bank 0 maps to bank 1
			uint8_t bank_number = value & 0xF;
			if (bank_number == 0)
			{
				bank_number = 1;
			}

			m_RomBank = bank_number;
		}
	}
	else if (address >= 0xA000 && address <= 0xBFFF)
	{
		if (this->IsRamEnabled())
		{
			m_ExternalRam[(address & 0x1FF) % m_ExternalRam.size()] = value & 0x0F;
		}
	}
}

void CartridgeMBC2::SaveState(std::fstream* file)
{
	int ram_size = static_cast<int>(m_ExternalRam.size());
	file->write(reinterpret_cast<const char*>(&ram_size), sizeof(ram_size));
	file->write(reinterpret_cast<const char*>(m_ExternalRam.data()), m_ExternalRam.size() * sizeof(uint8_t));

	file->write(reinterpret_cast<const char*>(&m_ExternalRamEnabled), sizeof(m_ExternalRamEnabled));
	file->write(reinterpret_cast<const char*>(&m_RomBank), sizeof(m_RomBank));
}

void CartridgeMBC2::LoadState(std::fstream* file)
{
	int ram_size = 0;
	file->read(reinterpret_cast<char*>(&ram_size), sizeof(ram_size));

	m_ExternalRam.resize(ram_size);
	file->read(reinterpret_cast<char*>(m_ExternalRam.data()), m_ExternalRam.size() * sizeof(uint8_t));

	file->read(reinterpret_cast<char*>(&m_ExternalRamEnabled), sizeof(m_ExternalRamEnabled));
	file->read(reinterpret_cast<char*>(&m_RomBank), sizeof(m_RomBank));
}
