#pragma once

#include "BaseCartridge.h"
#include <cstdint>
#include <vector>

class CartridgeROM : public BaseCartridge
{
public:
	CartridgeROM(CartridgeDataV2 cartridge_data) noexcept;
	virtual ~CartridgeROM() = default;

	// Inherited via BaseCartridge
	uint8_t Read(uint16_t address) override;
	void Write(uint16_t address, uint8_t value) override;

	void SaveState(std::fstream* file) override;
	void LoadState(std::fstream* file) override;

private:
	std::vector<uint8_t> m_ExternalRam;
};