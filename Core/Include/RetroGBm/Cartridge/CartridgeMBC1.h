#pragma once

#include "BaseCartridge.h"
#include <cstdint>

enum class BankMode
{
	ROM = 0,
	RAM = 1
};

class CartridgeMBC1 : public BaseCartridge
{
public:
	CartridgeMBC1(CartridgeDataV2 cartridge_data);
	virtual ~CartridgeMBC1() = default;

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

	inline BankMode GetBankMode() const
	{
		return m_BankMode;
	}

protected:
	bool m_ExternalRamEnabled = false;
	uint8_t m_RomBank = 1;
	BankMode m_BankMode = BankMode::ROM;
};