#pragma once

#include "BaseCartridge.h"
#include <cstdint>

class CartridgeROM : public BaseCartridge
{
public:
	CartridgeROM(CartridgeDataV2 cartridge_data);
	virtual ~CartridgeROM() = default;

	// Inherited via BaseCartridge
	uint8_t Read(uint16_t address) override;
	uint8_t Write(uint16_t address, uint8_t value) override;
};