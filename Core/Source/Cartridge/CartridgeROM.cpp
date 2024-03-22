#include "RetroGBm/Pch.h"
#include "RetroGBm/Cartridge/CartridgeROM.h"

CartridgeROM::CartridgeROM(CartridgeDataV2 cartridge_data) noexcept : BaseCartridge(cartridge_data)
{
	if (this->HasRAM())
	{
		m_ExternalRam.resize(m_CartridgeData.ram_size);
		std::fill(m_ExternalRam.begin(), m_ExternalRam.end(), 0x0);
	}
}

uint8_t CartridgeROM::Read(uint16_t address)
{
	if (address >= 0x0 && address <= 0x7FFF)
	{
		// Read from ROM
		return m_CartridgeData.data[address];
	}
	else if (address >= 0xA000 && address <= 0xBFFF)
	{
		// Read from RAM only if cartridge supports it
		if (this->HasRAM() && !m_ExternalRam.empty())
		{
			return m_ExternalRam[(address - 0xA000) % m_ExternalRam.size()];
		}
	}

	return 0xFF;
}

void CartridgeROM::Write(uint16_t address, uint8_t value)
{
	if (address >= 0x0 && address <= 0x7FFF)
	{
		// Ignore writes to ROM
	}
	else if (address >= 0xA000 && address <= 0xBFFF)
	{
		// Write to RAM only if cartridge supports it
		if (this->HasRAM() && !m_ExternalRam.empty())
		{
			m_ExternalRam[(address - 0xA000) % m_ExternalRam.size()] = value;

			if (this->HasBattery())
			{
				m_WriteRamCallback();
			}
		}
	}
}