#pragma once

#include "BaseCartridge.h"
#include <cstdint>

class CartridgeMBC2 : public BaseCartridge
{
public:
	// MBC2 has 512 x 4-bit RAM built into the mapper chip
	static constexpr int RamSize = 512;

	CartridgeMBC2(CartridgeDataV2 cartridge_data);
	virtual ~CartridgeMBC2() = default;

	// Inherited via BaseCartridge
	uint8_t Read(uint16_t address) override;
	void Write(uint16_t address, uint8_t value) override;

	void SaveState(std::fstream* file) override;
	void LoadState(std::fstream* file) override;

	inline bool IsRamEnabled() const
	{
		return m_ExternalRamEnabled && !m_ExternalRam.empty();
	}

	inline uint8_t GetRomBank() const
	{
		return m_RomBank;
	}

protected:
	bool m_ExternalRamEnabled = false;
	uint8_t m_RomBank = 1;
};
