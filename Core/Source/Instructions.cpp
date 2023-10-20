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
	context->cpu->ProgramCounter += 1;

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

std::string Op::LoadIndirectSP(EmulatorContext* context)
{
	uint8_t low = ReadFromBus(context, context->cpu->ProgramCounter + 1);
	uint8_t high = ReadFromBus(context, context->cpu->ProgramCounter + 2);
	uint16_t address = (high << 8) | low;

	uint16_t data = context->cpu->GetRegister(RegisterType16::REG_SP);
	uint8_t data_low = data & 0xFF;
	uint8_t data_high = (data >> 8) & 0xFF;

	WriteToBus(context, address + 0, data_low);
	WriteToBus(context, address + 1, data_high);

	context->cpu->ProgramCounter += 3;
	context->cycles += 20;

	std::string opcode_name = std::format("LD [n16], SP 0x{:x}", address);
	return opcode_name;
}

std::string Op::LoadHLFromSP(EmulatorContext* context)
{
	uint16_t data = context->cpu->GetRegister(RegisterType16::REG_HL);
	context->cpu->SetRegister(RegisterType16::REG_SP, data);

	context->cpu->ProgramCounter += 1;
	context->cycles += 8;

	std::string opcode_name = std::format("LD SP, HL");
	return opcode_name;
}

std::string Op::LoadHLFromSPRelative(EmulatorContext* context)
{
	uint16_t reg_data = context->cpu->GetRegister(RegisterType16::REG_SP);
	int8_t data = static_cast<int8_t>(ReadFromBus(context, context->cpu->ProgramCounter + 1));
	uint16_t result = data + reg_data;

	context->cpu->SetRegister(RegisterType16::REG_HL, result);

	context->cpu->SetFlag(CpuFlag::Zero, false);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	/*context->cpu->SetFlag(CpuFlag::HalfCarry, (reg_data & 0xF) + (data & 0xF) > 0xF);
	context->cpu->SetFlag(CpuFlag::Carry, ((reg_data & 0xFF) + data) > 0xF);*/

	context->cpu->SetFlag(CpuFlag::HalfCarry, (((reg_data & 0x0f) + (data & 0x0f)) & 0x10) != 0);
	context->cpu->SetFlag(CpuFlag::Carry, (((reg_data & 0xff) + (data & 0xff)) & 0x100) != 0);

	context->cpu->ProgramCounter += 2;
	context->cycles += 12;

	std::string opcode_name = std::format("LD SP, HL + 0x{:x}", data);
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
	context->cpu->ProgramCounter += 1;

	std::string opcode_name = std::format("LDD r8, [HL-] 0x{:x}", data);
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
	uint8_t data = context->cpu->GetRegister(RegisterType8::REG_A);
	uint16_t address = context->cpu->GetRegister(RegisterType8::REG_C);
	WriteToBus(context, 0xFF00 + address, data);

	context->cycles += 8;
	context->cpu->ProgramCounter += 1;

	std::string opcode_name = std::format("LD [C], A");
	return opcode_name;
}

std::string Op::LoadIndirectAC(EmulatorContext* context)
{
	uint16_t address = context->cpu->GetRegister(RegisterType8::REG_C);
	uint8_t data = ReadFromBus(context, 0xFF00 + address);
	context->cpu->SetRegister(RegisterType8::REG_A, data);

	context->cycles += 8;
	context->cpu->ProgramCounter += 1;

	std::string opcode_name = std::format("LD A, [C]");
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

std::string Op::SubR8(EmulatorContext* context, RegisterType8 reg)
{
	uint8_t result_a = context->cpu->GetRegister(RegisterType8::REG_A);
	uint8_t result_b = context->cpu->GetRegister(reg);

	uint16_t result = result_a - result_b;
	context->cpu->SetFlag(CpuFlag::Zero, (result & 0xFF) == 0x0);
	context->cpu->SetFlag(CpuFlag::Subtraction, true);
	context->cpu->SetFlag(CpuFlag::HalfCarry, (result_a & 0xF) < (result_b & 0xF));
	context->cpu->SetFlag(CpuFlag::Carry, result_a < result_b);

	context->cpu->SetRegister(RegisterType8::REG_A, static_cast<uint8_t>(result & 0xFF));

	context->cpu->ProgramCounter += 1;
	context->cycles += 4;

	std::string opcode_name = std::format("SUB A, {}", RegisterTypeString8(reg));
	return opcode_name;
}

std::string Op::SubIndirectHL(EmulatorContext* context)
{
	uint16_t address = context->cpu->GetRegister(RegisterType16::REG_HL);
	uint8_t result_a = context->cpu->GetRegister(RegisterType8::REG_A);
	uint8_t result_b = ReadFromBus(context, address);

	uint16_t result = result_a - result_b;
	context->cpu->SetFlag(CpuFlag::Zero, (result & 0xFF) == 0x0);
	context->cpu->SetFlag(CpuFlag::Subtraction, true);
	context->cpu->SetFlag(CpuFlag::HalfCarry, (result_a & 0xF) < (result_b & 0xF));
	context->cpu->SetFlag(CpuFlag::Carry, result_a < result_b);

	context->cpu->SetRegister(RegisterType8::REG_A, static_cast<uint8_t>(result & 0xFF));

	context->cpu->ProgramCounter += 1;
	context->cycles += 8;

	std::string opcode_name = std::format("SUB A, [HL]");
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
	context->cpu->SetFlag(CpuFlag::Zero, (result & 0xFF) == 0x0);
	context->cpu->SetFlag(CpuFlag::Subtraction, true);
	context->cpu->SetFlag(CpuFlag::HalfCarry, (result_a & 0xF) < (result_b & 0xF));
	context->cpu->SetFlag(CpuFlag::Carry, result_a < result_b);

	context->cpu->SetRegister(RegisterType8::REG_A, static_cast<uint8_t>(result & 0xFF));

	context->cpu->ProgramCounter += 2;
	context->cycles += 8;

	std::string opcode_name = std::format("ADD A, [HL]");
	return opcode_name;
}

std::string Op::AddSP(EmulatorContext* context)
{
	uint16_t reg_sp = context->cpu->GetRegister(RegisterType16::REG_SP);
	int8_t data = static_cast<int8_t>(ReadFromBus(context, context->cpu->ProgramCounter + 1));

	uint16_t result = reg_sp + data;
	context->cpu->SetRegister(RegisterType16::REG_SP, result);

	context->cpu->SetFlag(CpuFlag::Zero, false);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, (((reg_sp & 0x0f) + (data & 0x0f)) & 0x10) != 0);
	context->cpu->SetFlag(CpuFlag::Carry, (((reg_sp & 0xff) + (data & 0xff)) & 0x100) != 0);

	context->cpu->ProgramCounter += 2;
	context->cycles += 16;

	std::string opcode_name = std::format("ADD SP, 0x{:x}", result);
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

std::string Op::CallN16FlagSet(EmulatorContext* context, CpuFlag flag)
{
	uint8_t low = ReadFromBus(context, context->cpu->ProgramCounter + 1);
	uint8_t high = ReadFromBus(context, context->cpu->ProgramCounter + 2);
	uint16_t address = low | (high << 8);

	bool flag_result = context->cpu->GetFlag(flag);
	if (flag_result)
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

	std::string opcode_name = std::format("CALL {}, 0x{:x}", FlagString(flag), address);
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

std::string Op::ReturnInterrupt(EmulatorContext* context)
{
	context->cpu->StackPointer += 2;

	uint8_t high = ReadFromBus(context, context->cpu->StackPointer - 1);
	uint8_t low = ReadFromBus(context, context->cpu->StackPointer - 2);
	uint16_t address = low | (high << 8);

	context->cpu->EnableMasterInterrupts();

	context->cpu->ProgramCounter = address;
	context->cycles += 16;

	std::string opcode_name = std::format("RETI 0x{:x}", address);
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
	context->cpu->SetFlag(CpuFlag::HalfCarry, (data & 0xF) < (value & 0xF));
	context->cpu->SetFlag(CpuFlag::Carry, data < value);

	context->cycles += 8;
	context->cpu->ProgramCounter += 1;

	std::string opcode_name = std::format("CP A, [HL]", value);
	return opcode_name;
}

std::string Op::PushR16(EmulatorContext* context, RegisterType16 reg)
{
	context->cpu->StackPointer -= 2;

	uint16_t address = context->cpu->GetRegister(reg);
	uint8_t high = address >> 8;
	uint8_t low = address & 0xFF;

	WriteToBus(context, context->cpu->StackPointer + 1, high);
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

std::string Op::IncIndirectHL(EmulatorContext* context)
{
	uint16_t address = context->cpu->GetRegister(RegisterType16::REG_HL);
	uint8_t data = ReadFromBus(context, address);
	uint8_t result = data + 1;

	WriteToBus(context, address, result);
	context->cpu->SetFlag(CpuFlag::Subtraction, true);
	context->cpu->SetFlag(CpuFlag::Zero, result == 0);
	context->cpu->SetFlag(CpuFlag::HalfCarry, (data & 0xF) + (1 & 0xF) > 0xF);

	context->cpu->ProgramCounter += 1;
	context->cycles += 12;

	std::string opcode_name = std::format("INC [HL]");
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

std::string Op::OrN8(EmulatorContext* context)
{
	uint8_t result_a = context->cpu->GetRegister(RegisterType8::REG_A);
	uint8_t data = ReadFromBus(context, context->cpu->ProgramCounter + 1);

	uint8_t result = result_a | data;
	context->cpu->SetRegister(RegisterType8::REG_A, result);

	context->cpu->SetFlag(CpuFlag::Zero, result == 0);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, false);
	context->cpu->SetFlag(CpuFlag::Carry, false);

	context->cpu->ProgramCounter += 2;
	context->cycles += 8;

	std::string opcode_name = std::format("OR A, {}", data);
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
	context->cycles += 8;

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

	context->cpu->ProgramCounter += 1;
	context->cycles += 4;

	std::string opcode_name = std::format("AND A, {}", RegisterTypeString8(reg));
	return opcode_name;
}

std::string Op::AndIndirectHL(EmulatorContext* context)
{
	uint16_t address = context->cpu->GetRegister(RegisterType16::REG_HL);
	uint8_t result_a = context->cpu->GetRegister(RegisterType8::REG_A);
	uint8_t result_r = ReadFromBus(context, address);
	uint8_t result = result_a & result_r;
	context->cpu->SetRegister(RegisterType8::REG_A, result);

	context->cpu->SetFlag(CpuFlag::Zero, result == 0);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, true);
	context->cpu->SetFlag(CpuFlag::Carry, false);

	context->cpu->ProgramCounter += 2;
	context->cycles += 8;

	std::string opcode_name = std::format("AND A, [HL]");
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

std::string Op::RotateRegisterRightA(EmulatorContext* context)
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

std::string Op::RotateRegisterLeftA(EmulatorContext* context)
{
	uint8_t data = context->cpu->GetRegister(RegisterType8::REG_A);
	uint8_t result = data << 1;
	uint8_t bit7 = (data >> 7);

	bool carry_flag = context->cpu->GetFlag(CpuFlag::Carry);
	result |= (carry_flag ? 1 : 0);
	context->cpu->SetRegister(RegisterType8::REG_A, result);

	context->cpu->SetFlag(CpuFlag::Zero, false);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, false);
	context->cpu->SetFlag(CpuFlag::Carry, bit7 == 1);

	context->cpu->ProgramCounter += 1;
	context->cycles += 4;

	std::string opcode_name = std::format("RLA");
	return opcode_name;
}

std::string Op::ExtendedPrefix(EmulatorContext* context)
{
	uint8_t extended_op = ReadFromBus(context, context->cpu->ProgramCounter + 1);

	switch (extended_op)
	{
			// Rotate Left Carry
		case 0x0:
			CB::RotateLeftCarry(context, RegisterType8::REG_B);
			break;
		case 0x1:
			CB::RotateLeftCarry(context, RegisterType8::REG_C);
			break;
		case 0x2:
			CB::RotateLeftCarry(context, RegisterType8::REG_D);
			break;
		case 0x3:
			CB::RotateLeftCarry(context, RegisterType8::REG_E);
			break;
		case 0x4:
			CB::RotateLeftCarry(context, RegisterType8::REG_H);
			break;
		case 0x5:
			CB::RotateLeftCarry(context, RegisterType8::REG_L);
			break;
		case 0x7:
			CB::RotateLeftCarry(context, RegisterType8::REG_A);
			break;

			// Rotate Right Carry
		case 0x8:
			CB::RotateRightCarry(context, RegisterType8::REG_B);
			break;
		case 0x9:
			CB::RotateRightCarry(context, RegisterType8::REG_C);
			break;
		case 0xA:
			CB::RotateRightCarry(context, RegisterType8::REG_D);
			break;
		case 0xB:
			CB::RotateRightCarry(context, RegisterType8::REG_E);
			break;
		case 0xC:
			CB::RotateRightCarry(context, RegisterType8::REG_H);
			break;
		case 0xD:
			CB::RotateRightCarry(context, RegisterType8::REG_L);
			break;
		case 0xF:
			CB::RotateRightCarry(context, RegisterType8::REG_A);
			break;

			// Rotate Left
		case 0x10:
			CB::RotateLeft(context, RegisterType8::REG_B);
			break;
		case 0x11:
			CB::RotateLeft(context, RegisterType8::REG_C);
			break;
		case 0x12:
			CB::RotateLeft(context, RegisterType8::REG_D);
			break;
		case 0x13:
			CB::RotateLeft(context, RegisterType8::REG_E);
			break;
		case 0x14:
			CB::RotateLeft(context, RegisterType8::REG_H);
			break;
		case 0x15:
			CB::RotateLeft(context, RegisterType8::REG_L);
			break;
		case 0x17:
			CB::RotateLeft(context, RegisterType8::REG_A);
			break;

			// Rotate Right
		case 0x18:
			CB::RotateRight(context, RegisterType8::REG_B);
			break;
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

			// Shift Left Arithmetically
		case 0x20:
			CB::ShiftLeftArithmetically(context, RegisterType8::REG_B);
			break;
		case 0x21:
			CB::ShiftLeftArithmetically(context, RegisterType8::REG_C);
			break;
		case 0x22:
			CB::ShiftLeftArithmetically(context, RegisterType8::REG_D);
			break;
		case 0x23:
			CB::ShiftLeftArithmetically(context, RegisterType8::REG_E);
			break;
		case 0x24:
			CB::ShiftLeftArithmetically(context, RegisterType8::REG_H);
			break;
		case 0x25:
			CB::ShiftLeftArithmetically(context, RegisterType8::REG_L);
			break;
		case 0x27:
			CB::ShiftLeftArithmetically(context, RegisterType8::REG_A);
			break;

			// Shift Right Arithmetically
		case 0x28:
			CB::ShiftRightArithmetically(context, RegisterType8::REG_B);
			break;
		case 0x29:
			CB::ShiftRightArithmetically(context, RegisterType8::REG_C);
			break;
		case 0x2A:
			CB::ShiftRightArithmetically(context, RegisterType8::REG_D);
			break;
		case 0x2B:
			CB::ShiftRightArithmetically(context, RegisterType8::REG_E);
			break;
		case 0x2C:
			CB::ShiftRightArithmetically(context, RegisterType8::REG_H);
			break;
		case 0x2D:
			CB::ShiftRightArithmetically(context, RegisterType8::REG_L);
			break;
		case 0x2F:
			CB::ShiftRightArithmetically(context, RegisterType8::REG_A);
			break;

			// Swap
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

			// Shift Right Logically
		case 0x38:
			CB::ShiftRightLogically(context, RegisterType8::REG_B);
			break;
		case 0x39:
			CB::ShiftRightLogically(context, RegisterType8::REG_C);
			break;
		case 0x3A:
			CB::ShiftRightLogically(context, RegisterType8::REG_D);
			break;
		case 0x3B:
			CB::ShiftRightLogically(context, RegisterType8::REG_E);
			break;
		case 0x3C:
			CB::ShiftRightLogically(context, RegisterType8::REG_H);
			break;
		case 0x3D:
			CB::ShiftRightLogically(context, RegisterType8::REG_L);
			break;
		case 0x3F:
			CB::ShiftRightLogically(context, RegisterType8::REG_A);
			break;

			// Test bit 0
		case 0x40:
			CB::Bit(context, 0, RegisterType8::REG_B);
			break;
		case 0x41:
			CB::Bit(context, 0, RegisterType8::REG_C);
			break;
		case 0x42:
			CB::Bit(context, 0, RegisterType8::REG_D);
			break;
		case 0x43:
			CB::Bit(context, 0, RegisterType8::REG_E);
			break;
		case 0x44:
			CB::Bit(context, 0, RegisterType8::REG_H);
			break;
		case 0x45:
			CB::Bit(context, 0, RegisterType8::REG_L);
			break;
		case 0x46:
			CB::BitIndirectHL(context, 0);
			break;
		case 0x47:
			CB::Bit(context, 0, RegisterType8::REG_A);
			break;

			// Test bit 1
		case 0x48:
			CB::Bit(context, 1, RegisterType8::REG_B);
			break;
		case 0x49:
			CB::Bit(context, 1, RegisterType8::REG_C);
			break;
		case 0x4A:
			CB::Bit(context, 1, RegisterType8::REG_D);
			break;
		case 0x4B:
			CB::Bit(context, 1, RegisterType8::REG_E);
			break;
		case 0x4C:
			CB::Bit(context, 1, RegisterType8::REG_H);
			break;
		case 0x4D:
			CB::Bit(context, 1, RegisterType8::REG_L);
			break;
		case 0x4E:
			CB::BitIndirectHL(context, 1);
			break;
		case 0x4F:
			CB::Bit(context, 1, RegisterType8::REG_A);
			break;

			// Test bit 2
		case 0x50:
			CB::Bit(context, 2, RegisterType8::REG_B);
			break;
		case 0x51:
			CB::Bit(context, 2, RegisterType8::REG_C);
			break;
		case 0x52:
			CB::Bit(context, 2, RegisterType8::REG_D);
			break;
		case 0x53:
			CB::Bit(context, 2, RegisterType8::REG_E);
			break;
		case 0x54:
			CB::Bit(context, 2, RegisterType8::REG_H);
			break;
		case 0x55:
			CB::Bit(context, 2, RegisterType8::REG_L);
			break;
		case 0x56:
			CB::BitIndirectHL(context, 2);
			break;
		case 0x57:
			CB::Bit(context, 2, RegisterType8::REG_A);
			break;

			// Test bit 3
		case 0x58:
			CB::Bit(context, 3, RegisterType8::REG_B);
			break;
		case 0x59:
			CB::Bit(context, 3, RegisterType8::REG_C);
			break;
		case 0x5A:
			CB::Bit(context, 3, RegisterType8::REG_D);
			break;
		case 0x5B:
			CB::Bit(context, 3, RegisterType8::REG_E);
			break;
		case 0x5C:
			CB::Bit(context, 3, RegisterType8::REG_H);
			break;
		case 0x5D:
			CB::Bit(context, 3, RegisterType8::REG_L);
			break;
		case 0x5E:
			CB::BitIndirectHL(context, 3);
			break;
		case 0x5F:
			CB::Bit(context, 3, RegisterType8::REG_A);
			break;

			// Test bit 4
		case 0x60:
			CB::Bit(context, 4, RegisterType8::REG_B);
			break;
		case 0x61:
			CB::Bit(context, 4, RegisterType8::REG_C);
			break;
		case 0x62:
			CB::Bit(context, 4, RegisterType8::REG_D);
			break;
		case 0x63:
			CB::Bit(context, 4, RegisterType8::REG_E);
			break;
		case 0x64:
			CB::Bit(context, 4, RegisterType8::REG_H);
			break;
		case 0x65:
			CB::Bit(context, 4, RegisterType8::REG_L);
			break;
		case 0x66:
			CB::BitIndirectHL(context, 4);
			break;
		case 0x67:
			CB::Bit(context, 4, RegisterType8::REG_A);
			break;

			// Test bit 5
		case 0x68:
			CB::Bit(context, 5, RegisterType8::REG_B);
			break;
		case 0x69:
			CB::Bit(context, 5, RegisterType8::REG_C);
			break;
		case 0x6A:
			CB::Bit(context, 5, RegisterType8::REG_D);
			break;
		case 0x6B:
			CB::Bit(context, 5, RegisterType8::REG_E);
			break;
		case 0x6C:
			CB::Bit(context, 5, RegisterType8::REG_H);
			break;
		case 0x6D:
			CB::Bit(context, 5, RegisterType8::REG_L);
			break;
		case 0x6E:
			CB::BitIndirectHL(context, 5);
			break;
		case 0x6F:
			CB::Bit(context, 5, RegisterType8::REG_A);
			break;

			// Test bit 6
		case 0x70:
			CB::Bit(context, 6, RegisterType8::REG_B);
			break;
		case 0x71:
			CB::Bit(context, 6, RegisterType8::REG_C);
			break;
		case 0x72:
			CB::Bit(context, 6, RegisterType8::REG_D);
			break;
		case 0x73:
			CB::Bit(context, 6, RegisterType8::REG_E);
			break;
		case 0x74:
			CB::Bit(context, 6, RegisterType8::REG_H);
			break;
		case 0x75:
			CB::Bit(context, 6, RegisterType8::REG_L);
			break;
		case 0x76:
			CB::BitIndirectHL(context, 6);
			break;
		case 0x77:
			CB::Bit(context, 6, RegisterType8::REG_A);
			break;

			// Test bit 7
		case 0x78:
			CB::Bit(context, 7, RegisterType8::REG_B);
			break;
		case 0x79:
			CB::Bit(context, 7, RegisterType8::REG_C);
			break;
		case 0x7A:
			CB::Bit(context, 7, RegisterType8::REG_D);
			break;
		case 0x7B:
			CB::Bit(context, 7, RegisterType8::REG_E);
			break;
		case 0x7C:
			CB::Bit(context, 7, RegisterType8::REG_H);
			break;
		case 0x7D:
			CB::Bit(context, 7, RegisterType8::REG_L);
			break;
		case 0x7E:
			CB::BitIndirectHL(context, 7);
			break;
		case 0x7F:
			CB::Bit(context, 7, RegisterType8::REG_A);
			break;

			// Reset bit 0
		case 0x80:
			CB::Reset(context, 0, RegisterType8::REG_B);
			break;
		case 0x81:
			CB::Reset(context, 0, RegisterType8::REG_C);
			break;
		case 0x82:
			CB::Reset(context, 0, RegisterType8::REG_D);
			break;
		case 0x83:
			CB::Reset(context, 0, RegisterType8::REG_E);
			break;
		case 0x84:
			CB::Reset(context, 0, RegisterType8::REG_H);
			break;
		case 0x85:
			CB::Reset(context, 0, RegisterType8::REG_L);
			break;
		case 0x86:
			CB::ResetIndirectHL(context, 0);
			break;
		case 0x87:
			CB::Reset(context, 0, RegisterType8::REG_A);
			break;

			// Reset bit 1
		case 0x88:
			CB::Reset(context, 1, RegisterType8::REG_B);
			break;
		case 0x89:
			CB::Reset(context, 1, RegisterType8::REG_C);
			break;
		case 0x8A:
			CB::Reset(context, 1, RegisterType8::REG_D);
			break;
		case 0x8B:
			CB::Reset(context, 1, RegisterType8::REG_E);
			break;
		case 0x8C:
			CB::Reset(context, 1, RegisterType8::REG_H);
			break;
		case 0x8D:
			CB::Reset(context, 1, RegisterType8::REG_L);
			break;
		case 0x8E:
			CB::ResetIndirectHL(context, 1);
			break;
		case 0x8F:
			CB::Reset(context, 1, RegisterType8::REG_A);
			break;

			// Reset bit 2
		case 0x90:
			CB::Reset(context, 2, RegisterType8::REG_B);
			break;
		case 0x91:
			CB::Reset(context, 2, RegisterType8::REG_C);
			break;
		case 0x92:
			CB::Reset(context, 2, RegisterType8::REG_D);
			break;
		case 0x93:
			CB::Reset(context, 2, RegisterType8::REG_E);
			break;
		case 0x94:
			CB::Reset(context, 2, RegisterType8::REG_H);
			break;
		case 0x95:
			CB::Reset(context, 2, RegisterType8::REG_L);
			break;
		case 0x96:
			CB::ResetIndirectHL(context, 2);
			break;
		case 0x97:
			CB::Reset(context, 2, RegisterType8::REG_A);
			break;

			// Reset bit 3
		case 0x98:
			CB::Reset(context, 3, RegisterType8::REG_B);
			break;
		case 0x99:
			CB::Reset(context, 3, RegisterType8::REG_C);
			break;
		case 0x9A:
			CB::Reset(context, 3, RegisterType8::REG_D);
			break;
		case 0x9B:
			CB::Reset(context, 3, RegisterType8::REG_E);
			break;
		case 0x9C:
			CB::Reset(context, 3, RegisterType8::REG_H);
			break;
		case 0x9D:
			CB::Reset(context, 3, RegisterType8::REG_L);
			break;
		case 0x9E:
			CB::ResetIndirectHL(context, 3);
			break;
		case 0x9F:
			CB::Reset(context, 3, RegisterType8::REG_A);
			break;

			// Reset bit 4
		case 0xA0:
			CB::Reset(context, 4, RegisterType8::REG_B);
			break;
		case 0xA1:
			CB::Reset(context, 4, RegisterType8::REG_C);
			break;
		case 0xA2:
			CB::Reset(context, 4, RegisterType8::REG_D);
			break;
		case 0xA3:
			CB::Reset(context, 4, RegisterType8::REG_E);
			break;
		case 0xA4:
			CB::Reset(context, 4, RegisterType8::REG_H);
			break;
		case 0xA5:
			CB::Reset(context, 4, RegisterType8::REG_L);
			break;
		case 0xA6:
			CB::ResetIndirectHL(context, 4);
			break;
		case 0xA7:
			CB::Reset(context, 4, RegisterType8::REG_A);
			break;

			// Reset bit 5
		case 0xA8:
			CB::Reset(context, 5, RegisterType8::REG_B);
			break;
		case 0xA9:
			CB::Reset(context, 5, RegisterType8::REG_C);
			break;
		case 0xAA:
			CB::Reset(context, 5, RegisterType8::REG_D);
			break;
		case 0xAB:
			CB::Reset(context, 5, RegisterType8::REG_E);
			break;
		case 0xAC:
			CB::Reset(context, 5, RegisterType8::REG_H);
			break;
		case 0xAD:
			CB::Reset(context, 5, RegisterType8::REG_L);
			break;
		case 0xAE:
			CB::ResetIndirectHL(context, 5);
			break;
		case 0xAF:
			CB::Reset(context, 5, RegisterType8::REG_A);
			break;

			// Reset bit 6
		case 0xB0:
			CB::Reset(context, 6, RegisterType8::REG_B);
			break;
		case 0xB1:
			CB::Reset(context, 6, RegisterType8::REG_C);
			break;
		case 0xB2:
			CB::Reset(context, 6, RegisterType8::REG_D);
			break;
		case 0xB3:
			CB::Reset(context, 6, RegisterType8::REG_E);
			break;
		case 0xB4:
			CB::Reset(context, 6, RegisterType8::REG_H);
			break;
		case 0xB5:
			CB::Reset(context, 6, RegisterType8::REG_L);
			break;
		case 0xB6:
			CB::ResetIndirectHL(context, 6);
			break;
		case 0xB7:
			CB::Reset(context, 6, RegisterType8::REG_A);
			break;

			// Reset bit 7
		case 0xB8:
			CB::Reset(context, 7, RegisterType8::REG_B);
			break;
		case 0xB9:
			CB::Reset(context, 7, RegisterType8::REG_C);
			break;
		case 0xBA:
			CB::Reset(context, 7, RegisterType8::REG_D);
			break;
		case 0xBB:
			CB::Reset(context, 7, RegisterType8::REG_E);
			break;
		case 0xBC:
			CB::Reset(context, 7, RegisterType8::REG_H);
			break;
		case 0xBD:
			CB::Reset(context, 7, RegisterType8::REG_L);
			break;
		case 0xBE:
			CB::ResetIndirectHL(context, 7);
			break;
		case 0xBF:
			CB::Reset(context, 7, RegisterType8::REG_A);
			break;

			// Set bit 0
		case 0xC0:
			CB::Set(context, 0, RegisterType8::REG_B);
			break;
		case 0xC1:
			CB::Set(context, 0, RegisterType8::REG_C);
			break;
		case 0xC2:
			CB::Set(context, 0, RegisterType8::REG_D);
			break;
		case 0xC3:
			CB::Set(context, 0, RegisterType8::REG_E);
			break;
		case 0xC4:
			CB::Set(context, 0, RegisterType8::REG_H);
			break;
		case 0xC5:
			CB::Set(context, 0, RegisterType8::REG_L);
			break;
		case 0xC6:
			CB::SetIndirectHL(context, 0);
			break;
		case 0xC7:
			CB::Set(context, 0, RegisterType8::REG_A);
			break;

			// Set bit 1
		case 0xC8:
			CB::Set(context, 1, RegisterType8::REG_B);
			break;
		case 0xC9:
			CB::Set(context, 1, RegisterType8::REG_C);
			break;
		case 0xCA:
			CB::Set(context, 1, RegisterType8::REG_D);
			break;
		case 0xCB:
			CB::Set(context, 1, RegisterType8::REG_E);
			break;
		case 0xCC:
			CB::Set(context, 1, RegisterType8::REG_H);
			break;
		case 0xCD:
			CB::Set(context, 1, RegisterType8::REG_L);
			break;
		case 0xCE:
			CB::SetIndirectHL(context, 1);
			break;
		case 0xCF:
			CB::Set(context, 1, RegisterType8::REG_A);
			break;

			// Set bit 2
		case 0xD0:
			CB::Set(context, 2, RegisterType8::REG_B);
			break;
		case 0xD1:
			CB::Set(context, 2, RegisterType8::REG_C);
			break;
		case 0xD2:
			CB::Set(context, 2, RegisterType8::REG_D);
			break;
		case 0xD3:
			CB::Set(context, 2, RegisterType8::REG_E);
			break;
		case 0xD4:
			CB::Set(context, 2, RegisterType8::REG_H);
			break;
		case 0xD5:
			CB::Set(context, 2, RegisterType8::REG_L);
			break;
		case 0xD6:
			CB::SetIndirectHL(context, 2);
			break;
		case 0xD7:
			CB::Set(context, 2, RegisterType8::REG_A);
			break;

			// Set bit 3
		case 0xD8:
			CB::Set(context, 3, RegisterType8::REG_B);
			break;
		case 0xD9:
			CB::Set(context, 3, RegisterType8::REG_C);
			break;
		case 0xDA:
			CB::Set(context, 3, RegisterType8::REG_D);
			break;
		case 0xDB:
			CB::Set(context, 3, RegisterType8::REG_E);
			break;
		case 0xDC:
			CB::Set(context, 3, RegisterType8::REG_H);
			break;
		case 0xDD:
			CB::Set(context, 3, RegisterType8::REG_L);
			break;
		case 0xDE:
			CB::SetIndirectHL(context, 3);
			break;
		case 0xDF:
			CB::Set(context, 3, RegisterType8::REG_A);
			break;

			// Set bit 4
		case 0xE0:
			CB::Set(context, 4, RegisterType8::REG_B);
			break;
		case 0xE1:
			CB::Set(context, 4, RegisterType8::REG_C);
			break;
		case 0xE2:
			CB::Set(context, 4, RegisterType8::REG_D);
			break;
		case 0xE3:
			CB::Set(context, 4, RegisterType8::REG_E);
			break;
		case 0xE4:
			CB::Set(context, 4, RegisterType8::REG_H);
			break;
		case 0xE5:
			CB::Set(context, 4, RegisterType8::REG_L);
			break;
		case 0xE6:
			CB::SetIndirectHL(context, 4);
			break;
		case 0xE7:
			CB::Set(context, 4, RegisterType8::REG_A);
			break;

			// Set bit 5
		case 0xE8:
			CB::Set(context, 5, RegisterType8::REG_B);
			break;
		case 0xE9:
			CB::Set(context, 5, RegisterType8::REG_C);
			break;
		case 0xEA:
			CB::Set(context, 5, RegisterType8::REG_D);
			break;
		case 0xEB:
			CB::Set(context, 5, RegisterType8::REG_E);
			break;
		case 0xEC:
			CB::Set(context, 5, RegisterType8::REG_H);
			break;
		case 0xED:
			CB::Set(context, 5, RegisterType8::REG_L);
			break;
		case 0xEE:
			CB::SetIndirectHL(context, 5);
			break;
		case 0xEF:
			CB::Set(context, 5, RegisterType8::REG_A);
			break;

			// Set bit 6
		case 0xF0:
			CB::Set(context, 6, RegisterType8::REG_B);
			break;
		case 0xF1:
			CB::Set(context, 6, RegisterType8::REG_C);
			break;
		case 0xF2:
			CB::Set(context, 6, RegisterType8::REG_D);
			break;
		case 0xF3:
			CB::Set(context, 6, RegisterType8::REG_E);
			break;
		case 0xF4:
			CB::Set(context, 6, RegisterType8::REG_H);
			break;
		case 0xF5:
			CB::Set(context, 6, RegisterType8::REG_L);
			break;
		case 0xF6:
			CB::SetIndirectHL(context, 6);
			break;
		case 0xF7:
			CB::Set(context, 6, RegisterType8::REG_A);
			break;

			// Set bit 7
		case 0xF8:
			CB::Set(context, 7, RegisterType8::REG_B);
			break;
		case 0xF9:
			CB::Set(context, 7, RegisterType8::REG_C);
			break;
		case 0xFA:
			CB::Set(context, 7, RegisterType8::REG_D);
			break;
		case 0xFB:
			CB::Set(context, 7, RegisterType8::REG_E);
			break;
		case 0xFC:
			CB::Set(context, 7, RegisterType8::REG_H);
			break;
		case 0xFD:
			CB::Set(context, 7, RegisterType8::REG_L);
			break;
		case 0xFE:
			CB::SetIndirectHL(context, 7);
			break;
		case 0xFF:
			CB::Set(context, 7, RegisterType8::REG_A);
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

	context->cpu->SetRegister(RegisterType8::REG_A, (result & 0xFF));

	context->cpu->ProgramCounter += 2;
	context->cycles += 8;

	std::string opcode_name = std::format("ADC A, 0x{:x}", reg);
	return opcode_name;
}

std::string Op::SubCarryN8(EmulatorContext* context)
{
	uint8_t reg = context->cpu->GetRegister(RegisterType8::REG_A);
	uint8_t data = ReadFromBus(context, context->cpu->ProgramCounter + 1);

	bool carry_flag = context->cpu->GetFlag(CpuFlag::Carry);
	uint8_t carry_value = (carry_flag ? 1 : 0);
	uint16_t result = reg - data - carry_value;

	bool set_half_carry_flag = (reg & 0xF) < ((data & 0xF) + carry_value);
	context->cpu->SetFlag(CpuFlag::Zero, (result & 0xFF) == 0);
	context->cpu->SetFlag(CpuFlag::Subtraction, true);
	context->cpu->SetFlag(CpuFlag::HalfCarry, set_half_carry_flag);
	context->cpu->SetFlag(CpuFlag::Carry, reg < (data + carry_value));

	context->cpu->SetRegister(RegisterType8::REG_A, (result & 0xFF));

	context->cpu->ProgramCounter += 2;
	context->cycles += 8;

	std::string opcode_name = std::format("SBC A, 0x{:x}", reg);
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

std::string Op::Rst(EmulatorContext* context, uint8_t offset)
{
	context->cpu->StackPointer -= 2;

	uint8_t pc_high = (context->cpu->ProgramCounter + 1) >> 8;
	uint8_t pc_low = (context->cpu->ProgramCounter + 1) & 0xFF;

	WriteToBus(context, context->cpu->StackPointer + 1, pc_high);
	WriteToBus(context, context->cpu->StackPointer + 0, pc_low);

	context->cpu->ProgramCounter = offset;
	context->cycles += 16;

	std::string opcode_name = std::format("RST 0x{:x}", offset);
	return opcode_name;
}

std::string Op::ComplementCarryFlag(EmulatorContext* context)
{
	// 0x3F
	bool flag = context->cpu->GetFlag(CpuFlag::Carry);

	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, false);
	context->cpu->SetFlag(CpuFlag::Carry, !flag);

	context->cpu->ProgramCounter += 1;
	context->cycles += 4;

	std::string opcode_name = std::format("CCF");
	return opcode_name;
}

std::string Op::AddCarryR8(EmulatorContext* context, RegisterType8 reg)
{
	uint8_t result_a = context->cpu->GetRegister(RegisterType8::REG_A);
	uint8_t result_b = context->cpu->GetRegister(reg);

	bool carry_flag = context->cpu->GetFlag(CpuFlag::Carry);

	uint16_t result = result_a + result_b + (carry_flag ? 1 : 0);
	context->cpu->SetFlag(CpuFlag::Zero, (result & 0xFF) == 0x0);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, (result_a & 0xF) + (result_b & 0xF) > (carry_flag ? 0xE : 0xF));
	context->cpu->SetFlag(CpuFlag::Carry, result > 0xFF);

	context->cpu->SetRegister(RegisterType8::REG_A, static_cast<uint8_t>(result & 0xFF));

	context->cpu->ProgramCounter += 1;
	context->cycles += 4;

	std::string opcode_name = std::format("ADC A, {}", RegisterTypeString8(reg));
	return opcode_name;
}

std::string Op::AddCarryIndirectHL(EmulatorContext* context)
{
	uint16_t address = context->cpu->GetRegister(RegisterType16::REG_HL);

	uint8_t result_a = context->cpu->GetRegister(RegisterType8::REG_A);
	uint8_t result_b = ReadFromBus(context, address);

	bool carry_flag = context->cpu->GetFlag(CpuFlag::Carry);

	uint16_t result = result_a + result_b + (carry_flag ? 1 : 0);
	context->cpu->SetFlag(CpuFlag::Zero, (result & 0xFF) == 0x0);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, (result_a & 0xF) + (result_b & 0xF) > (carry_flag ? 0xE : 0xF));
	context->cpu->SetFlag(CpuFlag::Carry, result > 0xFF);

	context->cpu->SetRegister(RegisterType8::REG_A, static_cast<uint8_t>(result & 0xFF));

	context->cpu->ProgramCounter += 1;
	context->cycles += 8;

	std::string opcode_name = std::format("ADC A, [HL]");
	return opcode_name;
}

std::string Op::SubCarryR8(EmulatorContext* context, RegisterType8 reg)
{
	uint8_t result_a = context->cpu->GetRegister(RegisterType8::REG_A);
	uint8_t result_b = context->cpu->GetRegister(reg);

	bool carry_flag = context->cpu->GetFlag(CpuFlag::Carry);
	uint8_t carry_value = (carry_flag ? 1 : 0);
	uint16_t result = result_a - result_b - carry_value;

	bool set_half_carry_flag = (result_a & 0xF) < ((result_b & 0xF) + carry_value);
	context->cpu->SetFlag(CpuFlag::Zero, (result & 0xFF) == 0);
	context->cpu->SetFlag(CpuFlag::Subtraction, true);
	context->cpu->SetFlag(CpuFlag::HalfCarry, set_half_carry_flag);
	context->cpu->SetFlag(CpuFlag::Carry, result_a < (result_b + carry_value));

	context->cpu->SetRegister(RegisterType8::REG_A, (result & 0xFF));

	context->cpu->ProgramCounter += 1;
	context->cycles += 4;

	std::string opcode_name = std::format("SBC A, {}", RegisterTypeString8(reg));
	return opcode_name;
}

std::string Op::SubCarryIndirectHL(EmulatorContext* context)
{
	uint16_t address = context->cpu->GetRegister(RegisterType16::REG_HL);
	uint8_t result_a = context->cpu->GetRegister(RegisterType8::REG_A);
	uint8_t result_b = ReadFromBus(context, address);

	bool carry_flag = context->cpu->GetFlag(CpuFlag::Carry);
	uint8_t carry_value = (carry_flag ? 1 : 0);
	uint16_t result = result_a - result_b - carry_value;

	bool set_half_carry_flag = (result_a & 0xF) < ((result_b & 0xF) + carry_value);
	context->cpu->SetFlag(CpuFlag::Zero, (result & 0xFF) == 0);
	context->cpu->SetFlag(CpuFlag::Subtraction, true);
	context->cpu->SetFlag(CpuFlag::HalfCarry, set_half_carry_flag);
	context->cpu->SetFlag(CpuFlag::Carry, result_a < (result_b + carry_value));

	context->cpu->SetRegister(RegisterType8::REG_A, (result & 0xFF));

	context->cpu->ProgramCounter += 1;
	context->cycles += 8;

	std::string opcode_name = std::format("SBC A, [HL]");
	return opcode_name;
}

std::string Op::RotateRegisterLeftCarryA(EmulatorContext* context)
{
	uint8_t value = context->cpu->GetRegister(RegisterType8::REG_A);
	uint8_t bit7 = (value >> 7);

	value <<= 1;
	value |= bit7;

	context->cpu->SetRegister(RegisterType8::REG_A, value);

	context->cpu->SetFlag(CpuFlag::Zero, false);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, false);
	context->cpu->SetFlag(CpuFlag::Carry, bit7 == 1);

	context->cpu->ProgramCounter += 1;
	context->cycles += 4;

	std::string opcode_name = std::format("RLCA");
	return opcode_name;
}

std::string Op::RotateRegisterRightCarryA(EmulatorContext* context)
{
	uint8_t value = context->cpu->GetRegister(RegisterType8::REG_A);
	uint8_t bit0 = (value & 0x1);

	value >>= 1;
	value |= (bit0 << 7);

	context->cpu->SetRegister(RegisterType8::REG_A, value);

	context->cpu->SetFlag(CpuFlag::Zero, false);
	context->cpu->SetFlag(CpuFlag::Subtraction, false);
	context->cpu->SetFlag(CpuFlag::HalfCarry, false);
	context->cpu->SetFlag(CpuFlag::Carry, bit0 == 1);

	context->cpu->ProgramCounter += 1;
	context->cycles += 4;

	std::string opcode_name = std::format("RRCA");
	return opcode_name;
}