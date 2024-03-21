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
};