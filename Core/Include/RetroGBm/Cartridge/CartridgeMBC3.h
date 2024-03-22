#pragma once

#include "BaseCartridge.h"
#include <cstdint>

class CartridgeMBC3 : public BaseCartridge
{
public:
	CartridgeMBC3(CartridgeDataV2 cartridge_data);
	virtual ~CartridgeMBC3() = default;

	// Inherited via BaseCartridge
	uint8_t Read(uint16_t address) override;
	void Write(uint16_t address, uint8_t value) override;

	inline bool IsRamEnabled() const
	{
		return m_ExternalRamEnabled;
	}

	inline uint8_t GetRomBank() const
	{
		return m_RomBank;
	}

	inline uint8_t GetRamBank() const
	{
		return m_RamBank;
	}

protected:
	bool m_ExternalRamEnabled = false;
	uint8_t m_RomBank = 1;
	uint8_t m_RamBank = 0;
};