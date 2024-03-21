#include "RetroGBm/Pch.h"
#include "RetroGBm/Cartridge/CartridgeMBC5.h"

CartridgeMBC5::CartridgeMBC5(CartridgeDataV2 cartridge_data) : BaseCartridge(cartridge_data)
{
}

uint8_t CartridgeMBC5::Read(uint16_t address)
{
    return 0;
}

uint8_t CartridgeMBC5::Write(uint16_t address, uint8_t value)
{
    return 0;
}
