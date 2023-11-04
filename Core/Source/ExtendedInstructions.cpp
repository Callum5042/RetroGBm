#include "Pch.h"
#include "ExtendedInstructions.h"
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

void CB::ShiftRightLogicallyIndirectHL(EmulatorContext* context)
{
	uint16_t address = context->cpu->GetRegister(RegisterType16::REG_HL);
	uint8_t data = context->bus->ReadBus(address);

	uint8_t bit0 = (data & 0x1);
	uint8_t result = data >> 1;
	context->bus->WriteBus(address, result);

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

void CB::RotateRightIndirectHL(EmulatorContext* context)
{
	uint16_t address = context->cpu->GetRegister(RegisterType16::REG_HL);

	uint8_t data = context->bus->ReadBus(address);
	uint8_t result = data >> 1;
	uint8_t bit0 = (data & 0x1);

	bool carry_flag = context->cpu->GetFlag(CpuFlag::Carry);
	result |= (carry_flag ? 0b10000000 : 0);
	context->bus->WriteBus(address, result);

	context->cpu->SetFlag(CpuFlag::Zero, result == 0);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, false);
	context->cpu->SetFlag(CpuFlag::Carry, bit0 == 1);

	context->cpu->ProgramCounter += 2;
	context->cycles += 8;
}

void CB::RotateLeft(EmulatorContext* context, RegisterType8 reg)
{
	uint8_t data = context->cpu->GetRegister(reg);
	bool carry_flag = context->cpu->GetFlag(CpuFlag::Carry);

	uint8_t result = data << 1;
	result |= (carry_flag ? 1 : 0);
	context->cpu->SetRegister(reg, result);

	context->cpu->SetFlag(CpuFlag::Zero, result == 0);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, false);
	context->cpu->SetFlag(CpuFlag::Carry, (data & (1 << 7)) != 0);

	context->cpu->ProgramCounter += 2;
	context->cycles += 8;
}

void CB::RotateLeftIndirectHL(EmulatorContext* context)
{
	uint16_t address = context->cpu->GetRegister(RegisterType16::REG_HL);

	uint8_t data = context->bus->ReadBus(address);
	bool carry_flag = context->cpu->GetFlag(CpuFlag::Carry);

	uint8_t result = data << 1;
	result |= (carry_flag ? 1 : 0);
	context->bus->WriteBus(address, result);

	context->cpu->SetFlag(CpuFlag::Zero, result == 0);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, false);
	context->cpu->SetFlag(CpuFlag::Carry, (data & (1 << 7)) != 0);

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

void CB::SwapIndirectHL(EmulatorContext* context)
{
	uint16_t address = context->cpu->GetRegister(RegisterType16::REG_HL);

	uint8_t value = context->bus->ReadBus(address);
	uint8_t result = ((value & 0x0F) << 4) | ((value & 0xF0) >> 4);
	context->bus->WriteBus(address, result);

	context->cpu->SetFlag(CpuFlag::Zero, result == 0);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, false);
	context->cpu->SetFlag(CpuFlag::Carry, false);

	context->cpu->ProgramCounter += 2;
	context->cycles += 8;
}

void CB::RotateRightCarry(EmulatorContext* context, RegisterType8 reg)
{
	uint8_t value = context->cpu->GetRegister(reg);
	uint8_t result = value >> 1;
	if ((value & 1) == 1)
	{
		result |= (1 << 7);
		context->cpu->SetFlag(CpuFlag::Carry, true);
	}
	else
	{
		context->cpu->SetFlag(CpuFlag::Carry, false);
	}

	context->cpu->SetRegister(reg, result);

	context->cpu->SetFlag(CpuFlag::Zero, result == 0);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, false);

	context->cpu->ProgramCounter += 2;
	context->cycles += 8;
}

void CB::RotateRightCarryIndirectHL(EmulatorContext* context)
{
	uint16_t address = context->cpu->GetRegister(RegisterType16::REG_HL);

	uint8_t value = context->bus->ReadBus(address);
	uint8_t result = value >> 1;
	if ((value & 1) == 1)
	{
		result |= (1 << 7);
		context->cpu->SetFlag(CpuFlag::Carry, true);
	}
	else
	{
		context->cpu->SetFlag(CpuFlag::Carry, false);
	}

	context->bus->WriteBus(address, result);

	context->cpu->SetFlag(CpuFlag::Zero, result == 0);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, false);

	context->cpu->ProgramCounter += 2;
	context->cycles += 8;
}

void CB::RotateLeftCarry(EmulatorContext* context, RegisterType8 reg)
{
	uint8_t value = context->cpu->GetRegister(reg);
	uint8_t result = (value << 1) & 0xff;
	if ((value & (1 << 7)) != 0)
	{
		result |= 1;
		context->cpu->SetFlag(CpuFlag::Carry, true);
	}
	else
	{
		context->cpu->SetFlag(CpuFlag::Carry, false);
	}

	context->cpu->SetRegister(reg, result);

	context->cpu->SetFlag(CpuFlag::Zero, result == 0);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, false);

	context->cpu->ProgramCounter += 2;
	context->cycles += 8;
}

void CB::RotateLeftCarryIndirectHL(EmulatorContext* context)
{
	uint16_t address = context->cpu->GetRegister(RegisterType16::REG_HL);

	uint8_t value = context->bus->ReadBus(address);
	uint8_t result = (value << 1) & 0xff;
	if ((value & (1 << 7)) != 0)
	{
		result |= 1;
		context->cpu->SetFlag(CpuFlag::Carry, true);
	}
	else
	{
		context->cpu->SetFlag(CpuFlag::Carry, false);
	}

	context->bus->WriteBus(address, result);

	context->cpu->SetFlag(CpuFlag::Zero, result == 0);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, false);

	context->cpu->ProgramCounter += 2;
	context->cycles += 8;
}

void CB::Bit(EmulatorContext* context, uint8_t bit, RegisterType8 reg)
{
	uint8_t value = context->cpu->GetRegister(reg);
	bool set = (value >> bit) & 0x1;

	context->cpu->SetFlag(CpuFlag::Zero, !set);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, true);

	context->cpu->ProgramCounter += 2;
	context->cycles += 8;
}

void CB::BitIndirectHL(EmulatorContext* context, uint8_t bit)
{
	uint16_t address = context->cpu->GetRegister(RegisterType16::REG_HL);
	uint8_t value = context->bus->ReadBus(address);
	bool set = (value >> bit) & 0x1;

	context->cpu->SetFlag(CpuFlag::Zero, !set);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, true);

	context->cpu->ProgramCounter += 2;
	context->cycles += 8;
}

void CB::Set(EmulatorContext* context, uint8_t bit, RegisterType8 reg)
{
	uint8_t value = context->cpu->GetRegister(reg);

	uint8_t result = value | (1 << bit);
	context->cpu->SetRegister(reg, result);

	context->cpu->ProgramCounter += 2;
	context->cycles += 8;
}

void CB::SetIndirectHL(EmulatorContext* context, uint8_t bit)
{
	uint16_t address = context->cpu->GetRegister(RegisterType16::REG_HL);
	uint8_t value = context->bus->ReadBus(address);

	uint8_t result = value | (1 << bit);
	context->bus->WriteBus(address, result);

	context->cpu->ProgramCounter += 2;
	context->cycles += 8;
}

void CB::Reset(EmulatorContext* context, uint8_t bit, RegisterType8 reg)
{
	uint8_t value = context->cpu->GetRegister(reg);

	uint8_t result = value & ~(1 << bit);
	context->cpu->SetRegister(reg, result);

	context->cpu->ProgramCounter += 2;
	context->cycles += 8;
}

void CB::ResetIndirectHL(EmulatorContext* context, uint8_t bit)
{
	uint16_t address = context->cpu->GetRegister(RegisterType16::REG_HL);
	uint8_t value = context->bus->ReadBus(address);

	uint8_t result = value & ~(1 << bit);
	context->bus->WriteBus(address, result);

	context->cpu->ProgramCounter += 2;
	context->cycles += 8;
}

void CB::ShiftLeftArithmetically(EmulatorContext* context, RegisterType8 reg)
{
	uint8_t value = context->cpu->GetRegister(reg);
	uint8_t result = (value << 1) & 0xff;

	context->cpu->SetRegister(reg, result);

	context->cpu->SetFlag(CpuFlag::Zero, result == 0);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, false);
	context->cpu->SetFlag(CpuFlag::Carry, (value & (1 << 7)) != 0);

	context->cpu->ProgramCounter += 2;
	context->cycles += 8;
}

void CB::ShiftLeftArithmeticallyIndirectHL(EmulatorContext* context)
{
	uint16_t address = context->cpu->GetRegister(RegisterType16::REG_HL);
	uint8_t value = context->bus->ReadBus(address);
	uint8_t result = (value << 1) & 0xff;

	context->bus->WriteBus(address, result);

	context->cpu->SetFlag(CpuFlag::Zero, result == 0);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, false);
	context->cpu->SetFlag(CpuFlag::Carry, (value & (1 << 7)) != 0);

	context->cpu->ProgramCounter += 2;
	context->cycles += 8;
}

void CB::ShiftRightArithmetically(EmulatorContext* context, RegisterType8 reg)
{
	uint8_t value = context->cpu->GetRegister(reg);
	uint8_t result = (value >> 1) | (value & (1 << 7));

	context->cpu->SetRegister(reg, result);

	context->cpu->SetFlag(CpuFlag::Zero, result == 0);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, false);
	context->cpu->SetFlag(CpuFlag::Carry, (value & 1) != 0);

	context->cpu->ProgramCounter += 2;
	context->cycles += 8;
}

void CB::ShiftRightArithmeticallyIndirectHL(EmulatorContext* context)
{
	uint16_t address = context->cpu->GetRegister(RegisterType16::REG_HL);
	uint8_t value = context->bus->ReadBus(address);
	uint8_t result = (value >> 1) | (value & (1 << 7));

	context->bus->WriteBus(address, result);

	context->cpu->SetFlag(CpuFlag::Zero, result == 0);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, false);
	context->cpu->SetFlag(CpuFlag::Carry, (value & 1) != 0);

	context->cpu->ProgramCounter += 2;
	context->cycles += 8;
}