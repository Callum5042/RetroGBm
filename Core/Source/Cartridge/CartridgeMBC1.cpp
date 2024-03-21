#include "RetroGBm/Pch.h"
#include "RetroGBm/Cartridge/CartridgeMBC1.h"

CartridgeMBC1::CartridgeMBC1(CartridgeDataV2 cartridge_data) : BaseCartridge(cartridge_data)
{
}

uint8_t CartridgeMBC1::Read(uint16_t address)
{
    return 0;
}

uint8_t CartridgeMBC1::Write(uint16_t address, uint8_t value)
{
    return 0;
}
