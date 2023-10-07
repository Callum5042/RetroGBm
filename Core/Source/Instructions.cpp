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