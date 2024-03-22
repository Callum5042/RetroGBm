#pragma once

#include <cstdint>
#include <RetroGBm/Cartridge/BaseCartridge.h>

class MockCartridge : public BaseCartridge
{
public:
	MockCartridge() = default;
	virtual ~MockCartridge() = default;

	// Inherited via BaseCartridge
	uint8_t Read(uint16_t address) override;
	void Write(uint16_t address, uint8_t value) override;
};