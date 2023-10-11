#pragma once

#include <cstdint>
#include "Emulator.h"

const uint8_t ReadFromBus(EmulatorContext* context, const uint16_t address);

void WriteToBus(EmulatorContext* context, uint16_t address, uint8_t data);