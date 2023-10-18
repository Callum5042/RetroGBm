#include "Instructions.h"
#include "Bus.h"
#include "Emulator.h"
#include "Cpu.h"
#include "ExtendedInstructions.h"
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
	uint8_t low = ReadFromBus(context, context->cpu->ProgramCounter + 1);
	uint8_t high = ReadFromBus(context, context->cpu->ProgramCounter + 2);
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
		context->cpu->ProgramCounter += 3;
	}

	std::string opcode_name = std::format("JP {}{}, {}", (condition ? "" : "N"), FlagString(flag), data);
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
	int8_t data = ReadFromBus(context, context->cpu->ProgramCounter + 1);
	uint16_t address = static_cast<int16_t>(context->cpu->ProgramCounter + data + 2);

	context->cpu->ProgramCounter = address;
	context->cycles += 12;

	std::string opcode_name = std::format("JR 0x{:x}", address);
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
	context->cpu->ProgramCounter += 1;

	std::string opcode_name = std::format("XOR A, {}", RegisterTypeString8(type));
	return opcode_name;
}

std::string Op::XorN8(EmulatorContext* context)
{
	uint8_t reg_a = context->cpu->GetRegister(RegisterType8::REG_A);
	uint8_t data = ReadFromBus(context, context->cpu->ProgramCounter + 1);

	uint8_t result = reg_a ^ data;
	context->cpu->SetRegister(RegisterType8::REG_A, result);
	context->cpu->SetFlag(CpuFlag::Zero, result == 0);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, false);
	context->cpu->SetFlag(CpuFlag::Carry, false);

	context->cycles += 8;
	context->cpu->ProgramCounter += 2;

	std::string opcode_name = std::format("XOR 0x{:x}", data);
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
	context->cpu->ProgramCounter += 1;

	std::string opcode_name = std::format("XOR 0x{:x}", data);
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
	context->cpu->ProgramCounter += 1;

	std::string opcode_name = std::format("LDI [{}], A", RegisterTypeString16(RegisterType16::REG_HL));
	return opcode_name;
}

std::string Op::StoreDecrementHL(EmulatorContext* context)
{
	uint16_t address = context->cpu->GetRegister(RegisterType16::REG_HL);
	uint8_t data = context->cpu->GetRegister(RegisterType8::REG_A);
	WriteToBus(context, address, data);

	context->cpu->SetRegister(RegisterType16::REG_HL, address - 1);

	context->cycles += 8;
	context->cpu->ProgramCounter += 1;

	std::string opcode_name = std::format("LDD [{}], A", RegisterTypeString16(RegisterType16::REG_HL));
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
	uint8_t low = ReadFromBus(context, context->cpu->ProgramCounter + 1);
	uint8_t high = ReadFromBus(context, context->cpu->ProgramCounter + 2);
	uint16_t address = low | (high << 8);

	uint8_t data = context->cpu->GetRegister(reg);
	WriteToBus(context, address, data);

	context->cycles += 16;
	context->cpu->ProgramCounter += 3;

	std::string opcode_name = std::format("LD [a16], {} 0x{:x}", RegisterTypeString8(reg), address);
	return opcode_name;
}

std::string Op::LoadIndirectR8(EmulatorContext* context, RegisterType8 reg)
{
	uint8_t low = ReadFromBus(context, context->cpu->ProgramCounter + 1);
	uint8_t high = ReadFromBus(context, context->cpu->ProgramCounter + 2);
	uint16_t address = low | (high << 8);

	uint8_t data = ReadFromBus(context, address);
	context->cpu->SetRegister(reg, data);

	context->cycles += 16;
	context->cpu->ProgramCounter += 3;

	std::string opcode_name = std::format("LD [a16], {} 0x{:x})", RegisterTypeString8(reg), address);
	return opcode_name;
}

std::string Op::StoreFF00(EmulatorContext* context)
{
	uint8_t data = ReadFromBus(context, context->cpu->ProgramCounter + 1);
	uint8_t reg_data = context->cpu->GetRegister(RegisterType8::REG_A);

	WriteToBus(context, 0xFF00 + data, reg_data);
	context->cycles += 12;
	context->cpu->ProgramCounter += 2;

	std::string opcode_name = std::format("LDH [a8], A (0x{:x})", data);
	return opcode_name;
}

std::string Op::LoadFF00(EmulatorContext* context)
{
	uint8_t data = ReadFromBus(context, context->cpu->ProgramCounter + 1);
	uint8_t result = ReadFromBus(context, 0xFF00 + data);

	context->cpu->SetRegister(RegisterType8::REG_A, result);
	context->cycles += 12;
	context->cpu->ProgramCounter += 2;

	std::string opcode_name = std::format("LDH A, [a8] 0x{:x}", data);
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
	uint8_t data = ReadFromBus(context, context->cpu->ProgramCounter + 1);
	uint16_t address = context->cpu->GetRegister(reg_pointer);
	WriteToBus(context, address, data);

	context->cycles += 12;
	context->cpu->ProgramCounter += 2;

	std::string opcode_name = std::format("LD [{}], 0x{:x}", RegisterTypeString16(reg_pointer), data);
	return opcode_name;
}

std::string Op::LoadIndirectR16(EmulatorContext* context, RegisterType8 reg, RegisterType16 reg_pointer)
{
	uint16_t address = context->cpu->GetRegister(reg_pointer);
	uint8_t data = ReadFromBus(context, address);
	context->cpu->SetRegister(reg, data);

	context->cycles += 8;
	context->cpu->ProgramCounter += 1;

	std::string opcode_name = std::format("LD {}, [{}] 0x{:x}", RegisterTypeString8(reg), RegisterTypeString16(reg_pointer), data);
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
	context->cpu->SetFlag(CpuFlag::Zero, (result & 0xFF) == 0x0);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, (result_a & 0xF) + (result_b & 0xF) > 0xF);
	context->cpu->SetFlag(CpuFlag::Carry, result > 0xFF);

	context->cpu->SetRegister(RegisterType8::REG_A, static_cast<uint8_t>(result & 0xFF));

	context->cpu->ProgramCounter += 1;
	context->cycles += 4;

	std::string opcode_name = std::format("ADD A, {}", RegisterTypeString8(reg));
	return opcode_name;
}

std::string Op::AddN8(EmulatorContext* context)
{
	uint8_t result_a = context->cpu->GetRegister(RegisterType8::REG_A);
	uint8_t result_b = ReadFromBus(context, context->cpu->ProgramCounter + 1);

	uint16_t result = result_a + result_b;
	context->cpu->SetFlag(CpuFlag::Zero, (result & 0xFF) == 0x0);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, (result_a & 0xF) + (result_b & 0xF) > 0xF);
	context->cpu->SetFlag(CpuFlag::Carry, result > 0xFF);

	context->cpu->SetRegister(RegisterType8::REG_A, static_cast<uint8_t>(result & 0xFF));
	
	context->cpu->ProgramCounter += 2;
	context->cycles += 8;

	std::string opcode_name = std::format("ADD A, 0x{:x}", result_b);
	return opcode_name;
}

std::string Op::SubN8(EmulatorContext* context)
{
	uint8_t result_a = context->cpu->GetRegister(RegisterType8::REG_A);
	uint8_t result_b = ReadFromBus(context, context->cpu->ProgramCounter + 1);

	uint16_t result = result_a - result_b;
	context->cpu->SetFlag(CpuFlag::Zero, (result & 0xFF) == 0x0);
	context->cpu->SetFlag(CpuFlag::Subtraction, true);
	context->cpu->SetFlag(CpuFlag::HalfCarry, (result_a & 0xF) < (result_b & 0xF));
	context->cpu->SetFlag(CpuFlag::Carry, result_a < result_b);

	context->cpu->SetRegister(RegisterType8::REG_A, static_cast<uint8_t>(result & 0xFF));

	context->cpu->ProgramCounter += 2;
	context->cycles += 8;

	std::string opcode_name = std::format("SUB A, 0x{:x}", result_b);
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

	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::Carry, result > 0xFFFF);
	context->cpu->SetFlag(CpuFlag::HalfCarry, (result_a & 0xFFF) + (result_b & 0xFFF) > 0xFFF);

	context->cpu->ProgramCounter += 1;
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
	context->cpu->StackPointer -= 2;

	uint8_t low = ReadFromBus(context, context->cpu->ProgramCounter + 1);
	uint8_t high = ReadFromBus(context, context->cpu->ProgramCounter + 2);
	uint16_t address = low | (high << 8);

	uint8_t pc_high = (context->cpu->ProgramCounter + 3) >> 8;
	uint8_t pc_low = (context->cpu->ProgramCounter + 3) & 0xFF;

	WriteToBus(context, context->cpu->StackPointer + 1, pc_high);
	WriteToBus(context, context->cpu->StackPointer + 0, pc_low);
	context->cpu->ProgramCounter = address;

	context->cycles += 24;

	std::string opcode_name = std::format("CALL 0x{:x}", address);
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

std::string Op::CallN16FlagNotSet(EmulatorContext* context, CpuFlag flag)
{
	uint8_t low = ReadFromBus(context, context->cpu->ProgramCounter + 1);
	uint8_t high = ReadFromBus(context, context->cpu->ProgramCounter + 2);
	uint16_t address = low | (high << 8);

	bool flag_result = context->cpu->GetFlag(flag);
	if (!flag_result)
	{
		context->cpu->StackPointer -= 2;

		uint8_t pc_high = (context->cpu->ProgramCounter + 3) >> 8;
		uint8_t pc_low = (context->cpu->ProgramCounter + 3) & 0xFF;

		WriteToBus(context, context->cpu->StackPointer + 1, pc_high);
		WriteToBus(context, context->cpu->StackPointer + 0, pc_low);

		context->cpu->ProgramCounter = address;
		context->cycles += 24;
	}
	else
	{
		context->cycles += 12;
		context->cpu->ProgramCounter += 3;
	}

	std::string opcode_name = std::format("CALL N{}, 0x{:x}", FlagString(flag), address);
	return opcode_name;
}

std::string Op::Return(EmulatorContext* context)
{
	context->cpu->StackPointer += 2;

	uint8_t high = ReadFromBus(context, context->cpu->StackPointer - 1);
	uint8_t low = ReadFromBus(context, context->cpu->StackPointer - 2);
	uint16_t address = low | (high << 8);

	context->cpu->ProgramCounter = address;
	context->cycles += 16;

	std::string opcode_name = std::format("RET 0x{:x}", address);
	return opcode_name;
}

std::string Op::ReturnCondition(EmulatorContext* context, CpuFlag flag, bool condition)
{
	bool flag_result = context->cpu->GetFlag(flag);
	if (flag_result == condition)
	{
		context->cpu->StackPointer += 2;

		uint8_t high = ReadFromBus(context, context->cpu->StackPointer - 1);
		uint8_t low = ReadFromBus(context, context->cpu->StackPointer - 2);
		uint16_t address = high << 8 | low;

		context->cpu->ProgramCounter = address;
		context->cycles += 20;
	}
	else
	{
		context->cycles += 8;
		context->cpu->ProgramCounter += 1;
	}

	std::string opcode_name = std::format("RET {}{}", (condition ? "" : "N"), FlagString(flag));
	return opcode_name;
}

std::string Op::CompareR8(EmulatorContext* context, RegisterType8 reg)
{
	uint8_t a = context->cpu->GetRegister(RegisterType8::REG_A);
	uint8_t value = context->cpu->GetRegister(reg);

	uint8_t result = a - value;

	context->cpu->SetFlag(CpuFlag::Zero, result == 0);
	context->cpu->SetFlag(CpuFlag::Subtraction, true);
	context->cpu->SetFlag(CpuFlag::HalfCarry, (a & 0xF) < (value & 0xF));
	context->cpu->SetFlag(CpuFlag::Carry, a < value);

	context->cycles += 4;
	context->cpu->ProgramCounter += 1;

	std::string opcode_name = std::format("CP A, {}", RegisterTypeString8(reg));
	return opcode_name;
}

std::string Op::CompareN8(EmulatorContext* context)
{
	uint8_t data = context->cpu->GetRegister(RegisterType8::REG_A);
	uint8_t value = ReadFromBus(context, context->cpu->ProgramCounter + 1);

	uint8_t result = data - value;

	context->cpu->SetFlag(CpuFlag::Zero, result == 0);
	context->cpu->SetFlag(CpuFlag::Subtraction, true);
	context->cpu->SetFlag(CpuFlag::HalfCarry, (data & 0xF) < (value & 0xF));
	context->cpu->SetFlag(CpuFlag::Carry, data < value);

	context->cycles += 8;
	context->cpu->ProgramCounter += 2;

	std::string opcode_name = std::format("CP A, 0x{:x}", value);
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
	context->cpu->StackPointer -= 2;

	uint16_t address = context->cpu->GetRegister(reg);
	uint8_t high = address >> 8;
	uint8_t low = address & 0xFF;

	WriteToBus(context, context->cpu->StackPointer + 1 , high);
	WriteToBus(context, context->cpu->StackPointer + 0, low);
	
	context->cycles += 16;
	context->cpu->ProgramCounter += 1;

	std::string opcode_name = std::format("PUSH {}", RegisterTypeString16(reg));
	return opcode_name;
}

std::string Op::PopR16(EmulatorContext* context, RegisterType16 reg)
{
	context->cpu->StackPointer += 2;

	uint8_t high = ReadFromBus(context, context->cpu->StackPointer - 1);
	uint8_t low = ReadFromBus(context, context->cpu->StackPointer - 2);

	uint16_t data = high << 8 | low;
	context->cpu->SetRegister(reg, data);
	
	context->cycles += 12;
	context->cpu->ProgramCounter += 1;

	std::string opcode_name = std::format("POP {}", RegisterTypeString16(reg));
	return opcode_name;
}

std::string Op::IncR16(EmulatorContext* context, RegisterType16 reg)
{
	uint16_t data = context->cpu->GetRegister(reg);
	context->cpu->SetRegister(reg, data + 1);

	context->cycles += 8;
	context->cpu->ProgramCounter += 1;

	std::string opcode_name = std::format("INC {}", RegisterTypeString16(reg));
	return opcode_name;
}

std::string Op::DecR16(EmulatorContext* context, RegisterType16 reg)
{
	uint16_t data = context->cpu->GetRegister(reg);
	context->cpu->SetRegister(reg, data - 1);

	context->cycles += 8;
	context->cpu->ProgramCounter += 1;

	std::string opcode_name = std::format("DEC {}", RegisterTypeString16(reg));
	return opcode_name;
}

std::string Op::DecIndirectHL(EmulatorContext* context)
{
	uint16_t address = context->cpu->GetRegister(RegisterType16::REG_HL);
	uint8_t data = ReadFromBus(context, address);
	uint8_t result = data - 1;

	WriteToBus(context, address, result);
	context->cpu->SetFlag(CpuFlag::Subtraction, true);
	context->cpu->SetFlag(CpuFlag::Zero, result == 0);
	context->cpu->SetFlag(CpuFlag::HalfCarry, (data & 0xF) < (1 & 0xF));

	context->cpu->ProgramCounter += 1;
	context->cycles += 12;

	std::string opcode_name = std::format("DEC [HL]");
	return opcode_name;
}

std::string Op::JumpRelativeFlagNotSet(EmulatorContext* context, CpuFlag flag)
{
	bool flag_result = context->cpu->GetFlag(flag);
	int8_t data = static_cast<int8_t>(ReadFromBus(context, context->cpu->ProgramCounter + 1));
	uint16_t address = static_cast<int16_t>(context->cpu->ProgramCounter + data + 2);

	if (!flag_result)
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

std::string Op::JumpRelativeFlagSet(EmulatorContext* context, CpuFlag flag)
{
	bool flag_result = context->cpu->GetFlag(flag);
	int8_t data = static_cast<int8_t>(ReadFromBus(context, context->cpu->ProgramCounter + 1));
	uint16_t address = static_cast<int16_t>(context->cpu->ProgramCounter + data + 2);

	if (flag_result)
	{
		context->cycles += 12;
		context->cpu->ProgramCounter = address;
	}
	else
	{
		context->cycles += 8;
		context->cpu->ProgramCounter += 2;
	}

	std::string opcode_name = std::format("JR {}, 0x{:x}", FlagString(flag), address);
	return opcode_name;
}

std::string Op::OrR8(EmulatorContext* context, RegisterType8 reg)
{
	uint8_t result_a = context->cpu->GetRegister(RegisterType8::REG_A);
	uint8_t result_r = context->cpu->GetRegister(reg);

	uint8_t result = result_a | result_r;
	context->cpu->SetRegister(RegisterType8::REG_A, result);

	context->cpu->SetFlag(CpuFlag::Zero, result == 0);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, false);
	context->cpu->SetFlag(CpuFlag::Carry, false);

	context->cpu->ProgramCounter += 1;
	context->cycles += 4;

	std::string opcode_name = std::format("OR A, {}", RegisterTypeString8(reg));
	return opcode_name;
}

std::string Op::OrHL(EmulatorContext* context)
{
	uint8_t result_a = context->cpu->GetRegister(RegisterType8::REG_A);
	uint16_t address = context->cpu->GetRegister(RegisterType16::REG_HL);
	uint8_t result_r = ReadFromBus(context, address);

	uint8_t result = result_a | result_r;
	context->cpu->SetRegister(RegisterType8::REG_A, result);

	context->cpu->SetFlag(CpuFlag::Zero, result == 0);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, false);
	context->cpu->SetFlag(CpuFlag::Carry, false);

	context->cpu->ProgramCounter += 1;
	context->cycles += 4;

	std::string opcode_name = std::format("OR A, [HL]");
	return opcode_name;
}

std::string Op::AndR8(EmulatorContext* context, RegisterType8 reg)
{
	uint8_t result_a = context->cpu->GetRegister(RegisterType8::REG_A);
	uint8_t result_r = context->cpu->GetRegister(reg);
	uint8_t result = result_a & result_r;
	context->cpu->SetRegister(RegisterType8::REG_A, result);

	context->cpu->SetFlag(CpuFlag::Zero, result == 0);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, true);
	context->cpu->SetFlag(CpuFlag::Carry, false);

	context->cpu->ProgramCounter += 2;
	context->cycles += 8;

	std::string opcode_name = std::format("AND A, {}", RegisterTypeString8(reg));
	return opcode_name;
}

std::string Op::AndN8(EmulatorContext* context)
{
	uint8_t result_a = context->cpu->GetRegister(RegisterType8::REG_A);
	uint8_t result_r = ReadFromBus(context, context->cpu->ProgramCounter + 1);
	uint8_t result = result_a & result_r;
	context->cpu->SetRegister(RegisterType8::REG_A, result);

	context->cpu->SetFlag(CpuFlag::Zero, result == 0);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, true);
	context->cpu->SetFlag(CpuFlag::Carry, false);

	context->cpu->ProgramCounter += 2;
	context->cycles += 8;

	std::string opcode_name = std::format("AND A, 0x{:x}", result_r);
	return opcode_name;
}

std::string Op::RotateRegisterA(EmulatorContext* context)
{
	uint8_t data = context->cpu->GetRegister(RegisterType8::REG_A);
	uint8_t result = data >> 1;
	uint8_t bit0 = (data & 0x1);

	bool carry_flag = context->cpu->GetFlag(CpuFlag::Carry);
	result |= (carry_flag ? 0b10000000 : 0);
	context->cpu->SetRegister(RegisterType8::REG_A, result);

	context->cpu->SetFlag(CpuFlag::Zero, false);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, false);
	context->cpu->SetFlag(CpuFlag::Carry, bit0 == 1);

	context->cpu->ProgramCounter += 1;
	context->cycles += 4;

	std::string opcode_name = std::format("RRA");
	return opcode_name;
}

std::string Op::ExtendedPrefix(EmulatorContext* context)
{
	uint8_t extended_op = ReadFromBus(context, context->cpu->ProgramCounter + 1);

	switch (extended_op)
	{
		case 0x19:
			CB::RotateRight(context, RegisterType8::REG_C);
			break;
		case 0x1A:
			CB::RotateRight(context, RegisterType8::REG_D);
			break;
		case 0x1B:
			CB::RotateRight(context, RegisterType8::REG_E);
			break;
		case 0x1C:
			CB::RotateRight(context, RegisterType8::REG_H);
			break;
		case 0x1D:
			CB::RotateRight(context, RegisterType8::REG_L);
			break;
		case 0x1F:
			CB::RotateRight(context, RegisterType8::REG_A);
			break;
		case 0x30:
			CB::SwapR8(context, RegisterType8::REG_B);
			break;
		case 0x31:
			CB::SwapR8(context, RegisterType8::REG_C);
			break;
		case 0x32:
			CB::SwapR8(context, RegisterType8::REG_D);
			break;
		case 0x33:
			CB::SwapR8(context, RegisterType8::REG_E);
			break;
		case 0x34:
			CB::SwapR8(context, RegisterType8::REG_H);
			break;
		case 0x35:
			CB::SwapR8(context, RegisterType8::REG_L);
			break;
		case 0x37:
			CB::SwapR8(context, RegisterType8::REG_A);
			break;
		case 0x38:
			CB::ShiftRightLogically(context, RegisterType8::REG_B);
			break;

		default:
			throw std::exception(std::format("Extended instruction not implement: 0x{:x}", extended_op).c_str());
	}

	std::string opcode_name = std::format("PREFIX 0x{:x}", extended_op);
	return opcode_name;
}

std::string Op::AddCarryN8(EmulatorContext* context)
{
	uint8_t reg = context->cpu->GetRegister(RegisterType8::REG_A);
	uint8_t data = ReadFromBus(context, context->cpu->ProgramCounter + 1);

	bool carry_flag = context->cpu->GetFlag(CpuFlag::Carry);
	uint16_t result = reg + data + (carry_flag ? 1 : 0);

	bool set_half_carry_flag = (reg & 0xF) + (data & 0xF) > (carry_flag ? 0xE : 0xF);

	context->cpu->SetFlag(CpuFlag::Zero, (result & 0xFF) == 0);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, set_half_carry_flag);
	context->cpu->SetFlag(CpuFlag::Carry, (result > 0xFF));

	context->cpu->ProgramCounter += 2;
	context->cycles += 8;

	std::string opcode_name = std::format("ADC A, 0x{:x}", reg);
	return opcode_name;
}

std::string Op::ReturnFlagNotSet(EmulatorContext* context, CpuFlag flag)
{
	bool flag_result = context->cpu->GetFlag(flag);

	if (!flag_result)
	{
		context->cpu->StackPointer += 2;

		uint8_t high = ReadFromBus(context, context->cpu->StackPointer - 1);
		uint8_t low = ReadFromBus(context, context->cpu->StackPointer - 2);
		uint16_t address = high << 8 | low;

		context->cpu->ProgramCounter = address;
		context->cycles += 20;
	}
	else
	{
		context->cpu->ProgramCounter += 1;
		context->cycles += 8;
	}

	std::string opcode_name = std::format("RET N{}", FlagString(flag));
	return opcode_name;
}

std::string Op::ReturnFlagSet(EmulatorContext* context, CpuFlag flag)
{
	bool flag_result = context->cpu->GetFlag(flag);

	if (flag_result)
	{
		context->cpu->StackPointer += 2;

		uint8_t high = ReadFromBus(context, context->cpu->StackPointer - 1);
		uint8_t low = ReadFromBus(context, context->cpu->StackPointer - 2);
		uint16_t address = high << 8 | low;

		context->cpu->ProgramCounter = address;
		context->cycles += 20;
	}
	else
	{
		context->cpu->ProgramCounter += 1;
		context->cycles += 8;
	}

	std::string opcode_name = std::format("RET {}", FlagString(flag));
	return opcode_name;
}

std::string Op::Daa(EmulatorContext* context)
{
	bool subtract_flag = context->cpu->GetFlag(CpuFlag::Subtraction);
	bool carry_flag = context->cpu->GetFlag(CpuFlag::Carry);
	bool half_carry_flag = context->cpu->GetFlag(CpuFlag::HalfCarry);

	uint8_t value = context->cpu->GetRegister(RegisterType8::REG_A);

	if (!subtract_flag)
	{
		// after an addition, adjust if (half-)carry occurred or if result is out of bounds
		if (carry_flag || value > 0x99)
		{
			value += 0x60;
			context->cpu->SetFlag(CpuFlag::Carry, true);
		}

		if (half_carry_flag || (value & 0x0f) > 0x09)
		{
			value += 0x6;
		}
	}
	else
	{
		// after a subtraction, only adjust if (half-)carry occurred
		if (carry_flag)
		{
			value -= 0x60;
		}

		if (half_carry_flag)
		{
			value -= 0x6;
		}
	}

	context->cpu->SetFlag(CpuFlag::Zero, value == 0);
	context->cpu->SetFlag(CpuFlag::HalfCarry, false);

	context->cpu->SetRegister(RegisterType8::REG_A, value);
	context->cpu->ProgramCounter += 1;
	context->cycles += 4;

	std::string opcode_name = std::format("DAA");
	return opcode_name;
}

std::string Op::ComplementA(EmulatorContext* context)
{
	uint8_t value = context->cpu->GetRegister(RegisterType8::REG_A);
	value = ~value;

	context->cpu->SetFlag(CpuFlag::Subtraction, true);
	context->cpu->SetFlag(CpuFlag::HalfCarry, true);

	context->cpu->SetRegister(RegisterType8::REG_A, value);

	context->cpu->ProgramCounter += 1;
	context->cycles += 4;

	std::string opcode_name = std::format("CPL");
	return opcode_name;
}

std::string Op::SetCarryFlag(EmulatorContext* context)
{
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, false);
	context->cpu->SetFlag(CpuFlag::Carry, true);

	context->cpu->ProgramCounter += 1;
	context->cycles += 4;

	std::string opcode_name = std::format("SCF");
	return opcode_name;
}