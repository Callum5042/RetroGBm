#include "ExtendedInstructions.h"
#include "Bus.h"
#include "Emulator.h"
#include "Cpu.h"

void CB::ShiftRightLogically(EmulatorContext* context, RegisterType8 reg)
{
	uint8_t data = context->cpu->GetRegister(reg);

	uint8_t bit0 = (data & 0x1);
	uint8_t result = data >> 1;
	context->cpu->SetRegister(reg, result);

	context->cpu->SetFlag(CpuFlag::Zero, result == 0);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, false);
	context->cpu->SetFlag(CpuFlag::Carry, bit0 == 1);

	context->cpu->ProgramCounter += 2;
	context->cycles += 8;
}

void CB::RotateRight(EmulatorContext* context, RegisterType8 reg)
{
	uint8_t data = context->cpu->GetRegister(reg);
	uint8_t result = data >> 1;
	uint8_t bit0 = (data & 0x1);

	bool carry_flag = context->cpu->GetFlag(CpuFlag::Carry);
	result |= (carry_flag ? 0b10000000 : 0);
	context->cpu->SetRegister(reg, result);

	context->cpu->SetFlag(CpuFlag::Zero, result == 0);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, false);
	context->cpu->SetFlag(CpuFlag::Carry, bit0 == 1);

	context->cpu->ProgramCounter += 2;
	context->cycles += 8;
}

void CB::SwapR8(EmulatorContext* context, RegisterType8 reg)
{
	uint8_t value = context->cpu->GetRegister(reg);
	uint8_t result = ((value & 0x0F) << 4) | ((value & 0xF0) >> 4);
	context->cpu->SetRegister(reg, result);

	context->cpu->SetFlag(CpuFlag::Zero, result == 0);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, false);
	context->cpu->SetFlag(CpuFlag::Carry, false);

	context->cpu->ProgramCounter += 2;
	context->cycles += 8;
}