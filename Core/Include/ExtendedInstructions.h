#pragma once

#include <cstdint>
#include <string>
#include "Cartridge.h"
#include "Cpu.h"

struct EmulatorContext;

namespace CB
{
	void ShiftRightLogically(EmulatorContext* context, RegisterType8 reg);

	void RotateRight(EmulatorContext* context, RegisterType8 reg);

	void SwapR8(EmulatorContext* context, RegisterType8 reg);
}