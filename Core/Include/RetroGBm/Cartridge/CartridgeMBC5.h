#pragma once

#include "BaseCartridge.h"
#include <cstdint>

class CartridgeMBC5 : public BaseCartridge
{
public:
	CartridgeMBC5(CartridgeDataV2 cartridge_data);
	virtual ~CartridgeMBC5() = default;

	// Inherited via BaseCartridge
	uint8_t Read(uint16_t address) override;
	void Write(uint16_t address, uint8_t value) override;

	void SaveState(std::fstream* file);
	void LoadState(std::fstream* file);

	inline bool IsRamEnabled() const
	{
		return m_ExternalRamEnabled && !m_ExternalRam.empty();
	}

	inline uint16_t GetRomBank() const
	{
		return m_RomBank;
	}

	inline uint16_t GetRamBank() const
	{
		return m_RamBank;
	}

protected:
	bool m_ExternalRamEnabled = false;
	uint16_t m_RomBank = 1;
	uint16_t m_RamBank = 0;
};