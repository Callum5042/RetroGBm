#include "Instructions.h"
#include "Bus.h"
#include "Emulator.h"
#include "Cpu.h"
using namespace Op;

std::string Op::Nop(EmulatorContext* context)
{
	context->cycles += 4;
	std::string opcode_name = "NOP (0x0)";

	return opcode_name;
}

std::string Op::JumpN16(EmulatorContext* context)
{
	uint8_t low = ReadFromBus(context->cartridge.get(), context->cpu->ProgramCounter++);
	uint8_t high = ReadFromBus(context->cartridge.get(), context->cpu->ProgramCounter++);
	uint16_t data = low | (high << 8);

	context->cpu->ProgramCounter = data;
	context->cycles += 16;

	std::string opcode_name = std::format("JP n16 (0xC3 0x{:x} 0x{:x})", low, high);
	return opcode_name;
}

std::string Op::XorR8(EmulatorContext* context, RegisterType8 type)
{
	uint8_t reg_a = context->cpu->GetRegister(RegisterType8::REG_A);
	uint8_t reg_b = context->cpu->GetRegister(type);

	uint8_t result = reg_a ^ reg_b;
	context->cpu->SetRegister(RegisterType8::REG_A, result);
	context->cpu->SetFlag(CpuFlag::Zero, result == 0x0);

	context->cycles += 4;

	std::string opcode_name = "XOR r8 (0xAF)";
	return opcode_name;
}

std::string Op::XorN8(EmulatorContext* context)
{
	uint8_t reg_a = context->cpu->GetRegister(RegisterType8::REG_A);
	uint8_t data = ReadFromBus(context->cartridge.get(), context->cpu->ProgramCounter++);

	uint8_t result = reg_a ^ data;
	context->cpu->SetRegister(RegisterType8::REG_A, result);
	context->cpu->SetFlag(CpuFlag::Zero, result == 0);

	context->cycles += 8;

	std::string opcode_name = std::format("XOR n8 (0xEE 0x{:x})", data);
	return opcode_name;
}

std::string Op::XorR16(EmulatorContext* context, RegisterType16 type)
{
	uint16_t address = context->cpu->GetRegister(RegisterType16::REG_HL);
	uint8_t data = ReadFromBus(context->cartridge.get(), address);

	uint8_t reg_a = context->cpu->GetRegister(RegisterType8::REG_A);
	uint8_t result = reg_a ^ data;

	context->cpu->SetRegister(RegisterType8::REG_A, result);
	context->cpu->SetFlag(CpuFlag::Zero, result == 0);

	context->cycles += 8;

	std::string opcode_name = std::format("XOR r16 (0xEE 0x{:x})", data);
	return opcode_name;
}

std::string Op::LoadR8(EmulatorContext* context, RegisterType8 reg1, RegisterType8 reg2)
{
	uint8_t data = context->cpu->GetRegister(reg2);
	context->cpu->SetRegister(reg1, data);

	context->cycles += 4;

	std::string opcode_name = std::format("LD {}, {}", RegisterTypeString8(reg1), RegisterTypeString8(reg2));
	return opcode_name;
}

std::string Op::LoadN8(EmulatorContext* context, RegisterType8 type)
{
	uint8_t data = ReadFromBus(context->cartridge.get(), context->cpu->ProgramCounter++);
	context->cpu->SetRegister(type, data);

	context->cycles += 8;

	std::string opcode_name = std::format("LD {}, n8 (0x{:x})", RegisterTypeString8(type), data);
	return opcode_name;
}

std::string Op::LoadN16(EmulatorContext* context, RegisterType16 type)
{
	uint8_t low = ReadFromBus(context->cartridge.get(), context->cpu->ProgramCounter++);
	uint8_t high = ReadFromBus(context->cartridge.get(), context->cpu->ProgramCounter++);
	context->cpu->SetRegister(type, high, low);
	context->cycles += 12;

	std::string opcode_name = std::format("LD {}, n16 (0xC3 0x{:x} 0x{:x})", RegisterTypeString16(type), low, high);
	return opcode_name;
}

std::string Op::LoadIncrementHL(EmulatorContext* context)
{
	uint16_t address = context->cpu->GetRegister(RegisterType16::REG_HL);
	uint8_t data = context->cpu->GetRegister(RegisterType8::REG_A);
	WriteToBus(context->cartridge.get(), address, data);

	context->cpu->SetRegister(RegisterType16::REG_HL, address + 1);

	context->cycles += 8;

	std::string opcode_name = std::format("LDI [{}], r8", RegisterTypeString16(RegisterType16::REG_HL));
	return opcode_name;
}

std::string Op::LoadDecrementHL(EmulatorContext* context)
{
	uint16_t address = context->cpu->GetRegister(RegisterType16::REG_HL);
	uint8_t data = context->cpu->GetRegister(RegisterType8::REG_A);
	WriteToBus(context->cartridge.get(), address, data);

	context->cpu->SetRegister(RegisterType16::REG_HL, address - 1);

	context->cycles += 8;

	std::string opcode_name = std::format("LDD [{}], r8", RegisterTypeString16(RegisterType16::REG_HL));
	return opcode_name;
}

std::string Op::StoreR8(EmulatorContext* context, RegisterType8 reg, RegisterType16 reg_pointer)
{
	uint8_t data = context->cpu->GetRegister(reg);
	uint16_t address = context->cpu->GetRegister(reg_pointer);
	WriteToBus(context->cartridge.get(), address, data);

	context->cycles += 8;

	std::string opcode_name = std::format("LD [{}], r8", RegisterTypeString16(reg_pointer));
	return opcode_name;
}

std::string Op::StoreN8(EmulatorContext* context, RegisterType16 reg_pointer)
{
	uint8_t data = ReadFromBus(context->cartridge.get(), context->cpu->ProgramCounter++);
	uint16_t address = context->cpu->GetRegister(reg_pointer);
	WriteToBus(context->cartridge.get(), address, data);

	context->cycles += 12;

	std::string opcode_name = std::format("LD [{}], n8 (0x{:x})", RegisterTypeString16(reg_pointer), data);
	return opcode_name;
}

std::string Op::LoadIndirectR16(EmulatorContext* context, RegisterType8 reg, RegisterType16 reg_pointer)
{
	uint16_t address = context->cpu->GetRegister(reg_pointer);
	uint8_t data = ReadFromBus(context->cartridge.get(), address);
	context->cpu->SetRegister(reg, data);

	context->cycles += 8;

	std::string opcode_name = std::format("LD {}, [{}] (0x{:x})", RegisterTypeString8(reg), RegisterTypeString16(reg_pointer), data);
	return opcode_name;
}

std::string Op::StoreIndirectAC(EmulatorContext* context)
{
	// Opcode: 0xE2
	uint8_t data = context->cpu->GetRegister(RegisterType8::REG_A);
	uint16_t address = context->cpu->GetRegister(RegisterType8::REG_C);
	WriteToBus(context->cartridge.get(), address, data);
	
	context->cycles += 8;

	std::string opcode_name = std::format("LD [C], A (0x{:x})", data);
	return opcode_name;
}

std::string Op::LoadIndirectAC(EmulatorContext* context)
{
	// Opcode: 0xF2
	uint16_t address = context->cpu->GetRegister(RegisterType8::REG_C);
	uint8_t data = ReadFromBus(context->cartridge.get(), address);
	context->cpu->SetRegister(RegisterType8::REG_A, data);

	context->cycles += 8;

	std::string opcode_name = std::format("LD A, [C] (0x{:x})", data);
	return opcode_name;
}

std::string Op::StoreIndirectA16(EmulatorContext* context)
{
	// Opcode: 0xEA
	uint8_t low = ReadFromBus(context->cartridge.get(), context->cpu->ProgramCounter++);
	uint8_t high = ReadFromBus(context->cartridge.get(), context->cpu->ProgramCounter++);
	uint16_t address = low | (high << 8);

	uint8_t data = context->cpu->GetRegister(RegisterType8::REG_A);
	WriteToBus(context->cartridge.get(), address, data);

	context->cycles += 16;

	std::string opcode_name = std::format("LD [a16], A (0x{:x} 0x{:x})", low, high);
	return opcode_name;
}

std::string Op::LoadIndirectA16(EmulatorContext* context)
{
	// Opcode: 0xFA
	uint8_t low = ReadFromBus(context->cartridge.get(), context->cpu->ProgramCounter++);
	uint8_t high = ReadFromBus(context->cartridge.get(), context->cpu->ProgramCounter++);
	uint16_t address = low | (high << 8);

	uint8_t data = ReadFromBus(context->cartridge.get(), address);
	context->cpu->SetRegister(RegisterType8::REG_A, data);
	context->cycles += 16;

	std::string opcode_name = std::format("LD A, [a16] (0x{:x} 0x{:x} 0x{:x})", low, high, data);
	return opcode_name;
}

std::string Op::AddR8(EmulatorContext* context, RegisterType8 reg)
{
	uint8_t result_a = context->cpu->GetRegister(RegisterType8::REG_A);
	uint8_t result_b = context->cpu->GetRegister(reg);

	uint16_t result = result_a + result_b;
	context->cpu->SetFlag(CpuFlag::Zero, result == 0x0);

	if (result > 0xFF)
	{
		context->cpu->SetFlag(CpuFlag::Carry, true);
		result -= 0xFF;
	}
	else
	{
		context->cpu->SetFlag(CpuFlag::Carry, false);
	}

	bool half_carry = (((result_a & 0xF) + (result_b & 0xF)) & 0x10) == 0x10;
	context->cpu->SetFlag(CpuFlag::HalfCarry, half_carry);

	context->cpu->SetRegister(RegisterType8::REG_A, static_cast<uint8_t>(result));

	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cycles += 4;

	std::string opcode_name = std::format("ADD A, {}", RegisterTypeString8(reg));
	return opcode_name;
}

std::string Op::AddN8(EmulatorContext* context)
{
	uint8_t result_a = context->cpu->GetRegister(RegisterType8::REG_A);
	uint8_t result_b = ReadFromBus(context->cartridge.get(), context->cpu->ProgramCounter++);

	uint16_t result = result_a + result_b;
	context->cpu->SetFlag(CpuFlag::Zero, result == 0x0);

	if (result > 0xFF)
	{
		context->cpu->SetFlag(CpuFlag::Carry, true);
		result -= 0xFF;
	}
	else
	{
		context->cpu->SetFlag(CpuFlag::Carry, false);
	}

	bool half_carry = (((result_a & 0xF) + (result_b & 0xF)) & 0x10) == 0x10;
	context->cpu->SetFlag(CpuFlag::HalfCarry, half_carry);

	context->cpu->SetRegister(RegisterType8::REG_A, static_cast<uint8_t>(result));

	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cycles += 8;

	std::string opcode_name = std::format("ADD A, n8");
	return opcode_name;
}

std::string Op::AddIndirectHL(EmulatorContext* context)
{
	uint16_t address = context->cpu->GetRegister(RegisterType16::REG_HL);

	uint8_t result_a = context->cpu->GetRegister(RegisterType8::REG_A);
	uint8_t result_b = ReadFromBus(context->cartridge.get(), address);

	uint16_t result = result_a + result_b;
	context->cpu->SetFlag(CpuFlag::Zero, result == 0x0);

	if (result > 0xFF)
	{
		context->cpu->SetFlag(CpuFlag::Carry, true);
		result -= 0xFF;
	}
	else
	{
		context->cpu->SetFlag(CpuFlag::Carry, false);
	}

	bool half_carry = (((result_a & 0xF) + (result_b & 0xF)) & 0x10) == 0x10;
	context->cpu->SetFlag(CpuFlag::HalfCarry, half_carry);

	context->cpu->SetRegister(RegisterType8::REG_A, static_cast<uint8_t>(result));

	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cycles += 8;

	std::string opcode_name = std::format("ADD A, [HL]");
	return opcode_name;
}