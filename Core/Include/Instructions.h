#pragma once

#include <cstdint>
#include <string>
#include "Cartridge.h"
#include "Cpu.h"

struct EmulatorContext;

namespace Op
{
	std::string Nop(EmulatorContext* context);

	std::string JumpN16(EmulatorContext* context);

	std::string XorR8(EmulatorContext* context, RegisterType8 type);

	std::string LoadN16(EmulatorContext* context, RegisterType16 reg);
}