#pragma once

#include <cstdint>
#include "Cartridge.h"

const uint8_t ReadFromBus(const CartridgeInfo* cartridge, const uint16_t address);