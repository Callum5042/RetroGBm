#include "Instructions.h"
#include "Bus.h"
#include "Emulator.h"
#include "Cpu.h"
using namespace Op;

std::string Op::Nop(EmulatorContext* context)
{
	context->cycles += 4;
	context->cpu->ProgramCounter += 1;

	std::string opcode_name = "NOP";
	return opcode_name;
}

std::string Op::Stop(EmulatorContext* context)
{
	context->cycles += 4;
	context->cpu->ProgramCounter++;

	throw std::exception("STOP not implemented");

	std::string opcode_name = "STOP";
	return opcode_name;
}

std::string Op::EnableInterrupts(EmulatorContext* context)
{
	context->cpu->EnableMasterInterrupts();
	context->cycles += 4;

	std::string opcode_name = "EI";
	return opcode_name;
}

std::string Op::DisableInterrupts(EmulatorContext* context)
{
	context->cpu->DisableMasterInterrupts();
	context->cycles += 4;
	context->cpu->ProgramCounter += 1;

	std::string opcode_name = "DI";
	return opcode_name;
}

std::string Op::JumpN16(EmulatorContext* context)
{
	uint8_t low = ReadFromBus(context, context->cpu->ProgramCounter + 1);
	uint8_t high = ReadFromBus(context, context->cpu->ProgramCounter + 2);
	uint16_t data = low | (high << 8);

	context->cpu->ProgramCounter = data;
	context->cycles += 16;

	std::string opcode_name = std::format("JP 0x{:x}", data);
	return opcode_name;
}

std::string Op::JumpFlagN16(EmulatorContext* context, CpuFlag flag, bool condition)
{
	uint8_t low = ReadFromBus(context, context->cpu->ProgramCounter++);
	uint8_t high = ReadFromBus(context, context->cpu->ProgramCounter++);
	uint16_t data = low | (high << 8);

	bool enabled = context->cpu->GetFlag(flag);
	if (enabled == condition)
	{
		context->cycles += 16;
		context->cpu->ProgramCounter = data;
	}
	else
	{
		context->cycles += 12;
	}

	std::string opcode_name = std::format("JP {}{}, n16 (0x{:x} 0x{:x})", (condition ? "" : "N"), FlagString(flag), low, high);
	return opcode_name;
}

std::string Op::JumpHL(EmulatorContext* context)
{
	uint16_t address = context->cpu->GetRegister(RegisterType16::REG_HL);
	context->cpu->ProgramCounter = address;
	context->cycles += 4;

	std::string opcode_name = std::format("JP HL");
	return opcode_name;
}

std::string Op::JumpRelativeN8(EmulatorContext* context)
{
	uint16_t current_pc = context->cpu->ProgramCounter;
	int8_t data = ReadFromBus(context, context->cpu->ProgramCounter++);

	context->cpu->ProgramCounter += data;
	context->cycles += 12;

	std::string opcode_name = std::format("JR e8 0x{:x} (0x:{:x})", current_pc + data, data);
	return opcode_name;
}

std::string Op::JumpRelativeFlagN8(EmulatorContext* context, CpuFlag flag, bool condition)
{
	int8_t data = ReadFromBus(context, context->cpu->ProgramCounter++);
	uint16_t current_pc = context->cpu->ProgramCounter;

	bool enabled = context->cpu->GetFlag(flag);
	if (enabled == condition)
	{
		context->cpu->ProgramCounter = (context->cpu->ProgramCounter + data);
		context->cycles += 12;
	}
	else
	{
		context->cycles += 8;
	}

	std::string opcode_name = std::format("JR {}{}, e8 0x{:x} (0x{:x})", (condition ? "" : "N"), FlagString(flag), current_pc + data, static_cast<uint8_t>(data));
	return opcode_name;
}

std::string Op::XorR8(EmulatorContext* context, RegisterType8 type)
{
	uint8_t reg_a = context->cpu->GetRegister(RegisterType8::REG_A);
	uint8_t reg_b = context->cpu->GetRegister(type);

	uint8_t result = reg_a ^ reg_b;
	context->cpu->SetRegister(RegisterType8::REG_A, result);
	context->cpu->SetFlag(CpuFlag::Zero, result == 0x0);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, false);
	context->cpu->SetFlag(CpuFlag::Carry, false);

	context->cycles += 4;

	std::string opcode_name = "XOR r8";
	return opcode_name;
}

std::string Op::XorN8(EmulatorContext* context)
{
	uint8_t reg_a = context->cpu->GetRegister(RegisterType8::REG_A);
	uint8_t data = ReadFromBus(context, context->cpu->ProgramCounter++);

	uint8_t result = reg_a ^ data;
	context->cpu->SetRegister(RegisterType8::REG_A, result);
	context->cpu->SetFlag(CpuFlag::Zero, result == 0);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, false);
	context->cpu->SetFlag(CpuFlag::Carry, false);

	context->cycles += 8;

	std::string opcode_name = std::format("XOR n8 (0x{:x})", data);
	return opcode_name;
}

std::string Op::XorR16(EmulatorContext* context, RegisterType16 type)
{
	uint16_t address = context->cpu->GetRegister(RegisterType16::REG_HL);
	uint8_t data = ReadFromBus(context, address);

	uint8_t reg_a = context->cpu->GetRegister(RegisterType8::REG_A);
	uint8_t result = reg_a ^ data;

	context->cpu->SetRegister(RegisterType8::REG_A, result);
	context->cpu->SetFlag(CpuFlag::Zero, result == 0);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, false);
	context->cpu->SetFlag(CpuFlag::Carry, false);

	context->cycles += 8;

	std::string opcode_name = std::format("XOR r16 (0x{:x})", data);
	return opcode_name;
}

std::string Op::LoadR8(EmulatorContext* context, RegisterType8 reg1, RegisterType8 reg2)
{
	uint8_t data = context->cpu->GetRegister(reg2);
	context->cpu->SetRegister(reg1, data);

	context->cycles += 4;
	context->cpu->ProgramCounter += 1;

	std::string opcode_name = std::format("LD {}, {}", RegisterTypeString8(reg1), RegisterTypeString8(reg2));
	return opcode_name;
}

std::string Op::LoadN8(EmulatorContext* context, RegisterType8 type)
{
	uint8_t data = ReadFromBus(context, context->cpu->ProgramCounter + 1);
	context->cpu->SetRegister(type, data);

	context->cycles += 8;
	context->cpu->ProgramCounter += 2;

	std::string opcode_name = std::format("LD {}, 0x{:x}", RegisterTypeString8(type), data);
	return opcode_name;
}

std::string Op::LoadN16(EmulatorContext* context, RegisterType16 type)
{
	uint8_t low = ReadFromBus(context, context->cpu->ProgramCounter + 1);
	uint8_t high = ReadFromBus(context, context->cpu->ProgramCounter + 2);

	uint16_t result = (high << 8) | low;

	context->cpu->SetRegister(type, result);
	context->cycles += 12;
	context->cpu->ProgramCounter += 3;

	std::string opcode_name = std::format("LD {}, 0x{:x}", RegisterTypeString16(type), result);
	return opcode_name;
}

std::string Op::StoreIncrementHL(EmulatorContext* context)
{
	uint16_t address = context->cpu->GetRegister(RegisterType16::REG_HL);
	uint8_t data = context->cpu->GetRegister(RegisterType8::REG_A);
	WriteToBus(context, address, data);

	context->cpu->SetRegister(RegisterType16::REG_HL, address + 1);

	context->cycles += 8;

	std::string opcode_name = std::format("LDI [{}], r8", RegisterTypeString16(RegisterType16::REG_HL));
	return opcode_name;
}

std::string Op::StoreDecrementHL(EmulatorContext* context)
{
	uint16_t address = context->cpu->GetRegister(RegisterType16::REG_HL);
	uint8_t data = context->cpu->GetRegister(RegisterType8::REG_A);
	WriteToBus(context, address, data);

	context->cpu->SetRegister(RegisterType16::REG_HL, address - 1);

	context->cycles += 8;

	std::string opcode_name = std::format("LDD [{}], r8 (0x{:x})", RegisterTypeString16(RegisterType16::REG_HL), data);
	return opcode_name;
}

std::string Op::LoadIncrementHL(EmulatorContext* context)
{
	uint16_t address = context->cpu->GetRegister(RegisterType16::REG_HL);
	uint8_t data = ReadFromBus(context, address);

	context->cpu->SetRegister(RegisterType8::REG_A, data);
	context->cpu->SetRegister(RegisterType16::REG_HL, address + 1);

	context->cycles += 8;
	context->cpu->ProgramCounter += 1;

	std::string opcode_name = std::format("LD A, [HL+] 0x{:x}", data);
	return opcode_name;
}

std::string Op::LoadDecrementHL(EmulatorContext* context)
{
	uint16_t address = context->cpu->GetRegister(RegisterType16::REG_HL);
	uint8_t data = ReadFromBus(context, address);

	context->cpu->SetRegister(RegisterType8::REG_A, data);
	context->cpu->SetRegister(RegisterType16::REG_HL, address - 1);

	context->cycles += 8;

	std::string opcode_name = std::format("LDD r8, [{}] (0x{:x})", RegisterTypeString16(RegisterType16::REG_HL), data);
	return opcode_name;
}

std::string Op::StoreIndirectR8(EmulatorContext* context, RegisterType8 reg)
{
	uint8_t low = ReadFromBus(context, context->cpu->ProgramCounter++);
	uint8_t high = ReadFromBus(context, context->cpu->ProgramCounter++);
	uint16_t address = low | (high << 8);

	uint8_t data = context->cpu->GetRegister(reg);
	WriteToBus(context, address, data);

	context->cycles += 16;

	std::string opcode_name = std::format("LD [a16], {} (0x{:x}, 0x{:x})", RegisterTypeString8(reg), low, high);
	return opcode_name;
}

std::string Op::LoadIndirectR8(EmulatorContext* context, RegisterType8 reg)
{
	uint8_t low = ReadFromBus(context, context->cpu->ProgramCounter++);
	uint8_t high = ReadFromBus(context, context->cpu->ProgramCounter++);
	uint16_t address = low | (high << 8);

	uint8_t data = ReadFromBus(context, address);
	context->cpu->SetRegister(reg, data);

	context->cycles += 16;

	std::string opcode_name = std::format("LD [a16], {} (0x{:x}, 0x{:x})", RegisterTypeString8(reg), low, high);
	return opcode_name;
}

std::string Op::StoreHighRam(EmulatorContext* context)
{
	uint8_t data = ReadFromBus(context, context->cpu->ProgramCounter++);
	uint8_t reg_data = context->cpu->GetRegister(RegisterType8::REG_A);

	WriteToBus(context, 0xFF00 + data, reg_data);
	context->cycles += 12;

	std::string opcode_name = std::format("LDH [a8], A (0x{:x})", data);
	return opcode_name;
}

std::string Op::LoadHighRam(EmulatorContext* context)
{
	uint8_t data = ReadFromBus(context, context->cpu->ProgramCounter++);
	uint8_t result = ReadFromBus(context, 0xFF00 + data);

	context->cpu->SetRegister(RegisterType8::REG_A, result);
	context->cycles += 12;

	std::string opcode_name = std::format("LDH A, [a8] (0x{:x} 0x{:x})", data, result);
	return opcode_name;
}

std::string Op::StoreR8(EmulatorContext* context, RegisterType8 reg, RegisterType16 reg_pointer)
{
	uint8_t data = context->cpu->GetRegister(reg);
	uint16_t address = context->cpu->GetRegister(reg_pointer);
	WriteToBus(context, address, data);

	context->cycles += 8;
	context->cpu->ProgramCounter += 1;

	std::string opcode_name = std::format("LD [{}], {}", RegisterTypeString16(reg_pointer), RegisterTypeString8(reg));
	return opcode_name;
}

std::string Op::StoreN8(EmulatorContext* context, RegisterType16 reg_pointer)
{
	uint8_t data = ReadFromBus(context, context->cpu->ProgramCounter++);
	uint16_t address = context->cpu->GetRegister(reg_pointer);
	WriteToBus(context, address, data);

	context->cycles += 12;

	std::string opcode_name = std::format("LD [{}], n8 (0x{:x})", RegisterTypeString16(reg_pointer), data);
	return opcode_name;
}

std::string Op::LoadIndirectR16(EmulatorContext* context, RegisterType8 reg, RegisterType16 reg_pointer)
{
	uint16_t address = context->cpu->GetRegister(reg_pointer);
	uint8_t data = ReadFromBus(context, address);
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
	WriteToBus(context, 0xFF00 + address, data);

	context->cycles += 8;

	std::string opcode_name = std::format("LD [C], A (0x{:x})", data);
	return opcode_name;
}

std::string Op::LoadIndirectAC(EmulatorContext* context)
{
	// Opcode: 0xF2
	uint16_t address = context->cpu->GetRegister(RegisterType8::REG_C);
	uint8_t data = ReadFromBus(context, 0xFF00 + address);
	context->cpu->SetRegister(RegisterType8::REG_A, data);

	context->cycles += 8;

	std::string opcode_name = std::format("LD A, [C] (0x{:x})", data);
	return opcode_name;
}

std::string Op::StoreIndirectA16(EmulatorContext* context)
{
	// Opcode: 0xEA
	uint8_t low = ReadFromBus(context, context->cpu->ProgramCounter++);
	uint8_t high = ReadFromBus(context, context->cpu->ProgramCounter++);
	uint16_t address = low | (high << 8);

	uint8_t data = context->cpu->GetRegister(RegisterType8::REG_A);
	WriteToBus(context, address, data);

	context->cycles += 16;

	std::string opcode_name = std::format("LD [a16], A (0x{:x} 0x{:x})", low, high);
	return opcode_name;
}

std::string Op::LoadIndirectA16(EmulatorContext* context)
{
	// Opcode: 0xFA
	uint8_t low = ReadFromBus(context, context->cpu->ProgramCounter++);
	uint8_t high = ReadFromBus(context, context->cpu->ProgramCounter++);
	uint16_t address = low | (high << 8);

	uint8_t data = ReadFromBus(context, address);
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
	uint8_t result_b = ReadFromBus(context, context->cpu->ProgramCounter++);

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
	uint8_t result_b = ReadFromBus(context, address);

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

std::string Op::AddSP(EmulatorContext* context)
{
	// 0xE8
	uint16_t reg_sp = context->cpu->GetRegister(RegisterType16::REG_SP);
	int8_t data = static_cast<int8_t>(ReadFromBus(context, context->cpu->ProgramCounter++));

	uint16_t result = reg_sp + data;
	context->cpu->SetRegister(RegisterType16::REG_SP, result);

	if (result > 0xFF)
	{
		context->cpu->SetFlag(CpuFlag::Carry, true);
	}
	else
	{
		context->cpu->SetFlag(CpuFlag::Carry, false);
	}

	bool half_carry = (((reg_sp & 0xF) + (data & 0xF)) & 0x10) == 0x10;
	context->cpu->SetFlag(CpuFlag::HalfCarry, half_carry);

	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::Zero, false);
	context->cycles += 16;

	std::string opcode_name = std::format("ADD SP, e8 0x{:x} (0x:{:x})", result, static_cast<uint8_t>(data));
	return opcode_name;
}

std::string Op::AddR16(EmulatorContext* context, RegisterType16 reg)
{
	uint16_t result_a = context->cpu->GetRegister(RegisterType16::REG_HL);
	uint16_t result_b = context->cpu->GetRegister(reg);

	uint32_t result = result_a + result_b;
	context->cpu->SetRegister(RegisterType16::REG_HL, result);

	context->cpu->SetFlag(CpuFlag::Carry, result > 0xFFFF);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);

	if (((result_a & 0x0FFF) + (result_b & 0x0FFF) > 0x0FFF))
	{
		// Set the half-carry flag if there's a carry from low nibble to high nibble
		context->cpu->SetFlag(CpuFlag::HalfCarry, true);
	}
	else
	{
		// Clear the half-carry flag if there's no carry
		context->cpu->SetFlag(CpuFlag::HalfCarry, false);
	}

	context->cycles += 8;

	std::string opcode_name = std::format("ADD HL, {}", RegisterTypeString16(reg));
	return opcode_name;
}

std::string Op::IncR8(EmulatorContext* context, RegisterType8 reg)
{
	uint8_t data = context->cpu->GetRegister(reg);
	uint8_t result = data + 1;

	context->cpu->SetRegister(reg, result);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::Zero, result == 0);
	context->cpu->SetFlag(CpuFlag::HalfCarry, (data & 0xF) + (1 & 0xF) > 0xF);

	context->cycles += 4;
	context->cpu->ProgramCounter += 1;

	std::string opcode_name = std::format("INC {}", RegisterTypeString8(reg));
	return opcode_name;
}

std::string Op::DecR8(EmulatorContext* context, RegisterType8 reg)
{
	uint8_t data = context->cpu->GetRegister(reg);
	uint8_t result = data - 1;

	context->cpu->SetRegister(reg, result);
	context->cpu->SetFlag(CpuFlag::Subtraction, true);
	context->cpu->SetFlag(CpuFlag::Zero, result == 0);
	context->cpu->SetFlag(CpuFlag::HalfCarry, (data & 0xF) < (1 & 0xF));

	context->cycles += 4;
	context->cpu->ProgramCounter += 1;

	std::string opcode_name = std::format("DEC {}", RegisterTypeString8(reg));
	return opcode_name;
}

std::string Op::CallN16(EmulatorContext* context)
{
	uint8_t low = ReadFromBus(context, context->cpu->ProgramCounter++);
	uint8_t high = ReadFromBus(context, context->cpu->ProgramCounter++);
	uint16_t address = low | (high << 8);

	uint8_t pc_low = ((context->cpu->ProgramCounter) >> 8) & 0xFF;
	uint8_t pc_high = ((context->cpu->ProgramCounter) & 0xFF);

	WriteToBus(context, context->cpu->StackPointer--, pc_low);
	WriteToBus(context, context->cpu->StackPointer--, pc_high);
	context->cpu->ProgramCounter = address;

	context->cycles += 24;

	std::string opcode_name = std::format("CALL n16 (0x{:x} 0x{:x})", low, high);
	return opcode_name;
}

std::string Op::CallN16Condition(EmulatorContext* context, CpuFlag flag, bool condition)
{
	uint8_t low = ReadFromBus(context, context->cpu->ProgramCounter++);
	uint8_t high = ReadFromBus(context, context->cpu->ProgramCounter++);
	uint16_t address = low | (high << 8);

	bool flag_result = context->cpu->GetFlag(flag);
	if (flag_result == condition)
	{
		uint8_t pc_low = ((context->cpu->ProgramCounter) >> 8) & 0xFF;
		uint8_t pc_high = ((context->cpu->ProgramCounter) & 0xFF);

		WriteToBus(context, context->cpu->StackPointer--, pc_low);
		WriteToBus(context, context->cpu->StackPointer--, pc_high);
		context->cpu->ProgramCounter = address;
		context->cycles += 24;
	}
	else
	{
		context->cycles += 12;
	}

	std::string opcode_name = std::format("CALL {}{}, n16 (0x{:x} 0x{:x})", (condition ? "" : "N"), FlagString(flag), low, high);
	return opcode_name;
}

std::string Op::Return(EmulatorContext* context)
{
	uint8_t low = ReadFromBus(context, context->cpu->StackPointer + 1);
	uint8_t high = ReadFromBus(context, context->cpu->StackPointer + 2);
	uint16_t address = low | (high << 8);

	context->cpu->StackPointer += 2;

	context->cpu->ProgramCounter = address;
	context->cycles += 16;

	std::string opcode_name = std::format("RET");
	return opcode_name;
}

std::string Op::ReturnCondition(EmulatorContext* context, CpuFlag flag, bool condition)
{
	bool flag_result = context->cpu->GetFlag(flag);
	if (flag_result == condition)
	{
		uint8_t low = ReadFromBus(context, context->cpu->StackPointer++);
		uint8_t high = ReadFromBus(context, context->cpu->StackPointer++);
		uint16_t address = low | (high << 8);

		context->cpu->ProgramCounter = address;
		context->cycles += 20;
	}
	else
	{
		context->cycles += 8;
	}

	std::string opcode_name = std::format("RET {}{}, n16", (condition ? "" : "N"), FlagString(flag));
	return opcode_name;
}

std::string Op::CompareR8(EmulatorContext* context, RegisterType8 reg)
{
	uint8_t a = context->cpu->GetRegister(RegisterType8::REG_A);
	uint8_t value = context->cpu->GetRegister(reg);

	uint8_t result = a - value;

	context->cpu->SetFlag(CpuFlag::Zero, result == 0);
	context->cpu->SetFlag(CpuFlag::Subtraction, true);
	context->cpu->SetFlag(CpuFlag::HalfCarry, (0x0f & value) > (0x0f & a));
	context->cpu->SetFlag(CpuFlag::Carry, value > a);

	context->cycles += 4;

	std::string opcode_name = std::format("CP A, {}", RegisterTypeString8(reg));
	return opcode_name;
}

std::string Op::CompareN8(EmulatorContext* context)
{
	uint8_t data = context->cpu->GetRegister(RegisterType8::REG_A);
	uint8_t value = ReadFromBus(context, context->cpu->ProgramCounter++);

	uint8_t result = data - value;

	context->cpu->SetFlag(CpuFlag::Zero, result == 0);
	context->cpu->SetFlag(CpuFlag::Subtraction, true);
	context->cpu->SetFlag(CpuFlag::HalfCarry, (0x0f & value) > (0x0f & data));
	context->cpu->SetFlag(CpuFlag::Carry, value > data);

	context->cycles += 8;

	std::string opcode_name = std::format("CP A, n8 (0x{:x})", value);
	return opcode_name;
}

std::string Op::CompareIndirectHL(EmulatorContext* context)
{
	uint16_t address = context->cpu->GetRegister(RegisterType16::REG_HL);

	uint8_t data = context->cpu->GetRegister(RegisterType8::REG_A);
	uint8_t value = ReadFromBus(context, address);

	uint8_t result = data - value;

	context->cpu->SetFlag(CpuFlag::Zero, result == 0);
	context->cpu->SetFlag(CpuFlag::Subtraction, true);
	context->cpu->SetFlag(CpuFlag::HalfCarry, (0x0f & value) > (0x0f & data));
	context->cpu->SetFlag(CpuFlag::Carry, value > data);

	context->cycles += 8;

	std::string opcode_name = std::format("CP A, [HL] (0x{:x})", value);
	return opcode_name;
}

std::string Op::PushR16(EmulatorContext* context, RegisterType16 reg)
{
	uint16_t address = context->cpu->GetRegister(reg);
	uint8_t low = address & 0xFF;
	uint8_t high = (address >> 8) & 0xFF;

	WriteToBus(context, context->cpu->StackPointer - 1 , high);
	WriteToBus(context, context->cpu->StackPointer - 2, low);

	context->cpu->StackPointer -= 2;
	context->cycles += 16;

	std::string opcode_name = std::format("PUSH {}", RegisterTypeString16(reg));
	return opcode_name;
}

std::string Op::PopR16(EmulatorContext* context, RegisterType16 reg)
{
	uint8_t low = ReadFromBus(context, context->cpu->StackPointer + 1);
	uint8_t high = ReadFromBus(context, context->cpu->StackPointer);

	context->cpu->SetRegister(reg, high, low);
	context->cpu->StackPointer += 2;
	context->cycles += 12;

	std::string opcode_name = std::format("POP {}", RegisterTypeString16(reg));
	return opcode_name;
}

std::string Op::IncR16(EmulatorContext* context, RegisterType16 reg)
{
	uint16_t data = context->cpu->GetRegister(reg);
	context->cpu->SetRegister(reg, data + 1);

	context->cycles += 8;

	std::string opcode_name = std::format("INC {}", RegisterTypeString16(reg));
	return opcode_name;
}

std::string Op::DecR16(EmulatorContext* context, RegisterType16 reg)
{
	uint16_t data = context->cpu->GetRegister(reg);
	context->cpu->SetRegister(reg, data - 1);

	context->cycles += 8;

	std::string opcode_name = std::format("DEC {}", RegisterTypeString16(reg));
	return opcode_name;
}

std::string Op::JumpRelativeNotZero(EmulatorContext* context)
{
	bool flag_zero = context->cpu->GetFlag(CpuFlag::Zero);
	int8_t data = static_cast<int8_t>(ReadFromBus(context, context->cpu->ProgramCounter + 1));
	uint16_t address = static_cast<int16_t>(context->cpu->ProgramCounter + data + 2);

	if (!flag_zero)
	{
		context->cycles += 12;
		context->cpu->ProgramCounter = address;
	}
	else
	{
		context->cycles += 8;
		context->cpu->ProgramCounter += 2;
	}

	std::string opcode_name = std::format("JR NZ, 0x{:x}", address);
	return opcode_name;
}