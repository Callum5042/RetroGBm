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

	void RotateLeft(EmulatorContext* context, RegisterType8 reg);

	void SwapR8(EmulatorContext* context, RegisterType8 reg);

	void RotateRightCarry(EmulatorContext* context, RegisterType8 reg);

	void RotateLeftCarry(EmulatorContext* context, RegisterType8 reg);

	void Bit(EmulatorContext* context, uint8_t bit, RegisterType8 reg);

	void Set(EmulatorContext* context, uint8_t bit, RegisterType8 reg);

	void Reset(EmulatorContext* context, uint8_t bit, RegisterType8 reg);
}