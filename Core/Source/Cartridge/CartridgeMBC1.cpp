#include "RetroGBm/Pch.h"
#include "RetroGBm/Cartridge/CartridgeMBC1.h"

CartridgeMBC1::CartridgeMBC1(CartridgeDataV2 cartridge_data) : BaseCartridge(cartridge_data)
{
	m_ExternalRam.resize(cartridge_data.ram_size);
}

uint8_t CartridgeMBC1::Read(uint16_t address)
{
	if (address >= 0x0 && address <= 0x3FFF)
	{
		// Read from ROM
		if (m_BankMode == BankMode::ROM)
		{
			return m_CartridgeData.data[address];
		}
		else if (m_BankMode == BankMode::RAM)
		{
			uint16_t bank_number = (m_RomBank & 0x60);
			int offset = (address + (0x4000 * bank_number)) % m_CartridgeData.data.size();
			return m_CartridgeData.data[offset];
		}
	}
	else if (address >= 0x4000 && address <= 0x7FFF)
	{
		// Read from ROM
		uint8_t bank = m_RomBank;
		int offset = ((address - 0x4000) + (0x4000 * bank)) % m_CartridgeData.data.size();
		return m_CartridgeData.data[offset];
	}
	else if (address >= 0xA000 && address <= 0xBFFF)
	{
		if (this->IsRamEnabled())
		{
			if (m_BankMode == BankMode::ROM)
			{
				return m_ExternalRam[(address - 0xA000) % m_ExternalRam.size()];
			}
			else if (m_BankMode == BankMode::RAM)
			{
				uint8_t bank = (m_RomBank >> 5);
				int offset = ((address - 0xA000) + (bank * 0x2000)) % m_ExternalRam.size();
				return m_ExternalRam[offset];
			}
		}
	}

	return 0xFF;
}

void CartridgeMBC1::Write(uint16_t address, uint8_t value)
{
	if (address >= 0 && address <= 0x1FFF)
	{
		m_ExternalRamEnabled = (value & 0xF) == 0xA;
	}
	else if (address >= 0x2000 && address <= 0x3FFF)
	{
		// Cannot set rom bank to 0
		uint8_t bank_number = value & 0x1F;
		if ((bank_number & ~0b00000) == 0)
		{
			bank_number = 0b00001;
		}

		// Only write to the lower 5 bits
		m_RomBank &= ~0x1F;
		m_RomBank = bank_number;
	}
	else if (address >= 0x4000 && address <= 0x5FFF)
	{
		// m_RomSecondBank = value & 0b11;

		m_RomBank &= ~0x60;
		m_RomBank |= (value & 0x3) << 5;
	}
	else if (address >= 0x6000 && address <= 0x7FFF)
	{
		m_BankMode = static_cast<BankMode>(value & 0x1);
	}
	else if (address >= 0xA000 && address <= 0xBFFF)
	{
		if (this->IsRamEnabled())
		{
			if (m_BankMode == BankMode::ROM)
			{
				m_ExternalRam[(address - 0xA000) % m_ExternalRam.size()] = value;
			}
			else if (m_BankMode == BankMode::RAM)
			{
				uint8_t bank = (m_RomBank >> 5);
				int offset = ((address - 0xA000) + (bank * 0x2000)) % m_ExternalRam.size();
				m_ExternalRam[offset] = value;
			}

			if (this->HasBattery())
			{
				m_WriteRamCallback();
			}
		}
	}
}
