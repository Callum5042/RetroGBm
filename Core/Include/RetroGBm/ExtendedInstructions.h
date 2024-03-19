#pragma once

#include "RetroGBm/Cpu.h"
struct EmulatorContext;

namespace CB
{
	void ShiftRightLogically(EmulatorContext* context, RegisterType8 reg);

	void ShiftRightLogicallyIndirectHL(EmulatorContext* context);

	void RotateRight(EmulatorContext* context, RegisterType8 reg);

	void RotateRightIndirectHL(EmulatorContext* context);

	void RotateLeft(EmulatorContext* context, RegisterType8 reg);

	void RotateLeftIndirectHL(EmulatorContext* context);

	void SwapR8(EmulatorContext* context, RegisterType8 reg);

	void SwapIndirectHL(EmulatorContext* context);

	void RotateRightCarry(EmulatorContext* context, RegisterType8 reg);

	void RotateRightCarryIndirectHL(EmulatorContext* context);

	void RotateLeftCarry(EmulatorContext* context, RegisterType8 reg);

	void RotateLeftCarryIndirectHL(EmulatorContext* context);

	void Bit(EmulatorContext* context, uint8_t bit, RegisterType8 reg);

	void BitIndirectHL(EmulatorContext* context, uint8_t bit);

	void Set(EmulatorContext* context, uint8_t bit, RegisterType8 reg);

	void SetIndirectHL(EmulatorContext* context, uint8_t bit);

	void Reset(EmulatorContext* context, uint8_t bit, RegisterType8 reg);

	void ResetIndirectHL(EmulatorContext* context, uint8_t bit);

	void ShiftLeftArithmetically(EmulatorContext* context, RegisterType8 reg);

	void ShiftLeftArithmeticallyIndirectHL(EmulatorContext* context);

	void ShiftRightArithmetically(EmulatorContext* context, RegisterType8 reg);

	void ShiftRightArithmeticallyIndirectHL(EmulatorContext* context);
}