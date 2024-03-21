#include "RetroGBm/Pch.h"
#include "RetroGBm/Cartridge/CartridgeMBC3.h"

CartridgeMBC3::CartridgeMBC3(CartridgeDataV2 cartridge_data) : BaseCartridge(cartridge_data)
{
}

uint8_t CartridgeMBC3::Read(uint16_t address)
{
    return 0;
}

void CartridgeMBC3::Write(uint16_t address, uint8_t value)
{
}
