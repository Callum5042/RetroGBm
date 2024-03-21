#include "RetroGBm/Pch.h"
#include "RetroGBm/Cartridge/CartridgeROM.h"

CartridgeROM::CartridgeROM(CartridgeDataV2 cartridge_data) : BaseCartridge(cartridge_data)
{
}

uint8_t CartridgeROM::Read(uint16_t address)
{
	return 0;
}

uint8_t CartridgeROM::Write(uint16_t address, uint8_t value)
{
	return 0;
}