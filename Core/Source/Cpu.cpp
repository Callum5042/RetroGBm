#include "Cpu.h"
#include <exception>
#include <sstream>
#include <iomanip>
#include "Emulator.h"
#include "Instructions.h"

Cpu::Cpu()
{
	SetRegister(RegisterType8::REG_A, 0x1);
	SetRegister(RegisterType8::REG_F, 0x0);
	SetRegister(RegisterType8::REG_B, 0x0);
	SetRegister(RegisterType8::REG_C, 0x13);
	SetRegister(RegisterType8::REG_D, 0x0);
	SetRegister(RegisterType8::REG_E, 0xD8);
	SetRegister(RegisterType8::REG_H, 0x1);
	SetRegister(RegisterType8::REG_L, 0x4D);

	SetRegister(RegisterType16::REG_SP, 0xFFFE);

	SetFlag(CpuFlag::Zero, true);
	SetFlag(CpuFlag::Subtraction, false);
	SetFlag(CpuFlag::Carry, false);
	SetFlag(CpuFlag::HalfCarry, false);
}

void Cpu::SetRegister(RegisterType8 type, uint8_t data)
{
	if (type == RegisterType8::REG_F)
	{
		m_Registers[type] = data & 0xF0;
	}
	else
	{
		m_Registers[type] = data;
	}
}

void Cpu::SetRegister(RegisterType16 type, uint16_t data)
{
	uint8_t high = (data >> 8) & 0xFF;
	uint8_t low = data & 0xFF;

	SetRegister(type, high, low);
}

void Cpu::SetRegister(RegisterType16 type, uint8_t data_high, uint8_t data_low)
{
	switch (type)
	{
		case RegisterType16::REG_AF:
			m_Registers[RegisterType8::REG_A] = data_high;
			m_Registers[RegisterType8::REG_F] = data_low & 0xF0;
			break;

		case RegisterType16::REG_BC:
			m_Registers[RegisterType8::REG_B] = data_high;
			m_Registers[RegisterType8::REG_C] = data_low;
			break;

		case RegisterType16::REG_DE:
			m_Registers[RegisterType8::REG_D] = data_high;
			m_Registers[RegisterType8::REG_E] = data_low;
			break;

		case RegisterType16::REG_HL:
			m_Registers[RegisterType8::REG_H] = data_high;
			m_Registers[RegisterType8::REG_L] = data_low;
			break;

		case RegisterType16::REG_SP:
			StackPointer = data_low | (data_high << 8);
			break;
	}
}

uint8_t Cpu::GetRegister(RegisterType8 type) const
{
	auto it = m_Registers.find(type);
	if (it == m_Registers.end())
	{
		// TODO: Should this should throw an exception or just log and continue
		return 0;
	}

	return it->second;
}

uint16_t Cpu::GetRegister(RegisterType16 type) const
{
	uint8_t data_high = 0;
	uint8_t data_low = 0;

	switch (type)
	{
		case RegisterType16::REG_AF:
			data_high = GetRegister(RegisterType8::REG_A);
			data_low = GetRegister(RegisterType8::REG_F);
			break;

		case RegisterType16::REG_BC:
			data_high = GetRegister(RegisterType8::REG_B);
			data_low = GetRegister(RegisterType8::REG_C);
			break;

		case RegisterType16::REG_DE:
			data_high = GetRegister(RegisterType8::REG_D);
			data_low = GetRegister(RegisterType8::REG_E);
			break;

		case RegisterType16::REG_HL:
			data_high = GetRegister(RegisterType8::REG_H);
			data_low = GetRegister(RegisterType8::REG_L);
			break;

		case RegisterType16::REG_SP:
			return StackPointer;

		default:
			throw std::exception("GetRegister flag not supported");
	}

	uint16_t data = data_low | (data_high << 8);
	return data;
}

void Cpu::SetFlag(CpuFlag flag, bool data)
{
	if (data)
	{
		switch (flag)
		{
			case CpuFlag::Zero:
				m_Registers[RegisterType8::REG_F] |= (1 << 7);
				break;

			case CpuFlag::Subtraction:
				m_Registers[RegisterType8::REG_F] |= (1 << 6);
				break;

			case CpuFlag::HalfCarry:
				m_Registers[RegisterType8::REG_F] |= (1 << 5);
				break;

			case CpuFlag::Carry:
				m_Registers[RegisterType8::REG_F] |= (1 << 4);
				break;

			default:
				throw std::exception("Tried to set to unsupported flag");
		}
	}
	else
	{
		switch (flag)
		{
			case CpuFlag::Zero:
				m_Registers[RegisterType8::REG_F] &= ~(1 << 7);
				break;

			case CpuFlag::Subtraction:
				m_Registers[RegisterType8::REG_F] &= ~(1 << 6);
				break;

			case CpuFlag::HalfCarry:
				m_Registers[RegisterType8::REG_F] &= ~(1 << 5);
				break;

			case CpuFlag::Carry:
				m_Registers[RegisterType8::REG_F] &= ~(1 << 4);
				break;

			default:
				throw std::exception("Tried to set to unsupported flag");
		}
	}
}

bool Cpu::GetFlag(CpuFlag flag) const
{
	uint8_t flag_register = GetRegister(RegisterType8::REG_F);

	switch (flag)
	{
		case CpuFlag::Zero:
			return ((flag_register >> 7) & 1) != 0;

		case CpuFlag::Subtraction:
			return ((flag_register >> 6) & 1) != 0;

		case CpuFlag::HalfCarry:
			return ((flag_register >> 5) & 1) != 0;

		case CpuFlag::Carry:
			return ((flag_register >> 4) & 1) != 0;
	}

	// TODO: Should this should throw an exception or just log and continue
	return false;
}

std::string Cpu::Details()
{
	std::stringstream ss;
	ss.setf(std::ios_base::hex, std::ios_base::basefield);

	ss << "Registers("
		<< "A:" << +GetRegister(RegisterType8::REG_A)
		<< " F:" << +GetRegister(RegisterType8::REG_F)
		<< " B:" << +GetRegister(RegisterType8::REG_B)
		<< " C:" << +GetRegister(RegisterType8::REG_C)
		<< " D:" << +GetRegister(RegisterType8::REG_D)
		<< " E:" << +GetRegister(RegisterType8::REG_E)
		<< " H:" << +GetRegister(RegisterType8::REG_H)
		<< " L:" << +GetRegister(RegisterType8::REG_L)
		<< ")";

	ss << " Flags("
		<< "Z:" << GetFlag(CpuFlag::Zero)
		<< " N:" << GetFlag(CpuFlag::Subtraction)
		<< " H:" << GetFlag(CpuFlag::HalfCarry)
		<< " C:" << GetFlag(CpuFlag::Carry)
		<< ")";

	return ss.str();
}

std::string RegisterTypeString16(RegisterType16 type)
{
	switch (type)
	{
		case RegisterType16::REG_AF:
			return "AF";

		case RegisterType16::REG_BC:
			return "BC";

		case RegisterType16::REG_DE:
			return "DE";

		case RegisterType16::REG_HL:
			return "HL";

		case RegisterType16::REG_SP:
			return "SP";
	}

	throw std::exception("unsupported RegisterType16 in function RegisterTypeString16");
}

std::string RegisterTypeString8(RegisterType8 type)
{
	switch (type)
	{
		case RegisterType8::REG_A:
			return "A";

		case RegisterType8::REG_F:
			return "F";

		case RegisterType8::REG_B:
			return "B";

		case RegisterType8::REG_C:
			return "C";

		case RegisterType8::REG_D:
			return "D";

		case RegisterType8::REG_E:
			return "E";

		case RegisterType8::REG_H:
			return "H";

		case RegisterType8::REG_L:
			return "L";
	}

	throw std::exception("unsupported RegisterType16 in function RegisterTypeString8");
}

std::string FlagString(CpuFlag flag)
{
	switch (flag)
	{
		case CpuFlag::Zero:
			return "Z";

		case CpuFlag::Subtraction:
			return "N";

		case CpuFlag::Carry:
			return "C";

		case CpuFlag::HalfCarry:
			return "H";
	}

	throw std::exception("unsupported CpuFlag in function FlagString");
}

void Cpu::EnableMasterInterrupts()
{
	m_EnablingInterrupts = true;
}

void Cpu::DisableMasterInterrupts()
{
	m_InterruptMasterFlag = false;
}

bool Cpu::GetInterruptMasterFlag() const
{
	return m_InterruptMasterFlag;
}

void Cpu::RequestInterrupt(InterruptFlag flag)
{
	m_InterruptFlags |= static_cast<int>(flag);
}

void Cpu::SetInterrupt(uint8_t data)
{
	m_InterruptFlags = data;
}

void Cpu::SetInterruptEnable(uint8_t data)
{
	m_InterruptEnable = data;
}

uint8_t Cpu::GetInterruptEnable() const
{
	return m_InterruptEnable;
}

void Cpu::HandleInterrupts()
{
	if (m_InterruptMasterFlag)
	{
		if (InterruptCheck(0x40, InterruptFlag::VBlank))
		{

		}
		else if (InterruptCheck(0x48, InterruptFlag::STAT))
		{

		}
		else if (InterruptCheck(0x50, InterruptFlag::Timer))
		{

		}
		else if (InterruptCheck(0x58, InterruptFlag::Serial))
		{

		}
		else if (InterruptCheck(0x60, InterruptFlag::Joypad))
		{

		}

		m_EnablingInterrupts = false;
	}

	if (m_EnablingInterrupts)
	{
		m_InterruptMasterFlag = true;
	}
}

bool Cpu::InterruptCheck(uint16_t address, InterruptFlag flag)
{
	if (m_InterruptFlags & static_cast<int>(flag) && m_InterruptEnable & static_cast<int>(flag))
	{
		Emulator::Instance->StackPush16(ProgramCounter);
		ProgramCounter = address;

		m_InterruptFlags &= ~static_cast<int>(flag);
		// ctx->halted = false;
		// m_InterruptMasterFlag = false;

		return true;
	}

	return false;
}

uint8_t Cpu::GetInterruptFlags()
{
	return m_InterruptFlags;
}

std::string Cpu::Execute(EmulatorContext* context, const uint8_t opcode)
{
	switch (opcode)
	{
		case 0x0:
			return Op::Nop(context);
		case 0x01:
			return Op::LoadN16(context, RegisterType16::REG_BC);
		case 0x02:
			return Op::StoreR8(context, RegisterType8::REG_A, RegisterType16::REG_BC);
		case 0x03:
			return Op::IncR16(context, RegisterType16::REG_BC);
		case 0x04:
			return Op::IncR8(context, RegisterType8::REG_B);
		case 0x05:
			return Op::DecR8(context, RegisterType8::REG_B);
		case 0x06:
			return Op::LoadN8(context, RegisterType8::REG_B);
		case 0x07:
			return Op::RotateRegisterLeftCarryA(context);
		case 0x08:
			return Op::LoadIndirectSP(context);
		case 0x09:
			return Op::AddR16(context, RegisterType16::REG_BC);
		case 0x0A:
			return Op::LoadIndirectR16(context, RegisterType8::REG_A, RegisterType16::REG_BC);
		case 0x0B:
			return Op::DecR16(context, RegisterType16::REG_BC);
		case 0x0C:
			return Op::IncR8(context, RegisterType8::REG_C);
		case 0x0D:
			return Op::DecR8(context, RegisterType8::REG_C);
		case 0x0E:
			return Op::LoadN8(context, RegisterType8::REG_C);
		case 0x0F:
			return Op::RotateRegisterRightCarryA(context);
			//case 0x10:
			//	return Op::Stop(context);
		case 0x11:
			return Op::LoadN16(context, RegisterType16::REG_DE);
		case 0x12:
			return Op::StoreR8(context, RegisterType8::REG_A, RegisterType16::REG_DE);
		case 0x13:
			return Op::IncR16(context, RegisterType16::REG_DE);
		case 0x14:
			return Op::IncR8(context, RegisterType8::REG_D);
		case 0x15:
			return Op::DecR8(context, RegisterType8::REG_D);
		case 0x16:
			return Op::LoadN8(context, RegisterType8::REG_D);
		case 0x17:
			return Op::RotateRegisterLeftA(context);
		case 0x18:
			return Op::JumpRelativeN8(context);
		case 0x19:
			return Op::AddR16(context, RegisterType16::REG_DE);
		case 0x1A:
			return Op::LoadIndirectR16(context, RegisterType8::REG_A, RegisterType16::REG_DE);
		case 0x1B:
			return Op::DecR16(context, RegisterType16::REG_DE);
		case 0x1C:
			return Op::IncR8(context, RegisterType8::REG_E);
		case 0x1D:
			return Op::DecR8(context, RegisterType8::REG_E);
		case 0x1E:
			return Op::LoadN8(context, RegisterType8::REG_E);
		case 0x1F:
			return Op::RotateRegisterRightA(context);
		case 0x20:
			return Op::JumpRelativeFlagNotSet(context, CpuFlag::Zero);
		case 0x21:
			return Op::LoadN16(context, RegisterType16::REG_HL);
		case 0x22:
			return Op::StoreIncrementHL(context);
		case 0x23:
			return Op::IncR16(context, RegisterType16::REG_HL);
		case 0x24:
			return Op::IncR8(context, RegisterType8::REG_H);
		case 0x25:
			return Op::DecR8(context, RegisterType8::REG_H);
		case 0x26:
			return Op::LoadN8(context, RegisterType8::REG_H);
		case 0x27:
			return Op::Daa(context);
		case 0x28:
			return Op::JumpRelativeFlagSet(context, CpuFlag::Zero);
		case 0x29:
			return Op::AddR16(context, RegisterType16::REG_HL);
		case 0x2A:
			return Op::LoadIncrementHL(context);
		case 0x2B:
			return Op::DecR16(context, RegisterType16::REG_HL);
		case 0x2C:
			return Op::IncR8(context, RegisterType8::REG_L);
		case 0x2D:
			return Op::DecR8(context, RegisterType8::REG_L);
		case 0x2E:
			return Op::LoadN8(context, RegisterType8::REG_L);
		case 0x2F:
			return Op::ComplementA(context);
		case 0x30:
			return Op::JumpRelativeFlagNotSet(context, CpuFlag::Carry);
		case 0x31:
			return Op::LoadN16(context, RegisterType16::REG_SP);
		case 0x32:
			return Op::StoreDecrementHL(context);
		case 0x33:
			return Op::IncR16(context, RegisterType16::REG_SP);
		case 0x34:
			return Op::IncIndirectHL(context);
		case 0x35:
			return Op::DecIndirectHL(context);
		case 0x36:
			return Op::StoreN8(context, RegisterType16::REG_HL);
		case 0x37:
			return Op::SetCarryFlag(context);
		case 0x38:
			return Op::JumpRelativeFlagSet(context, CpuFlag::Carry);
		case 0x39:
			return Op::AddR16(context, RegisterType16::REG_SP);
		case 0x3A:
			return Op::LoadDecrementHL(context);
		case 0x3B:
			return Op::DecR16(context, RegisterType16::REG_SP);
		case 0x3C:
			return Op::IncR8(context, RegisterType8::REG_A);
		case 0x3D:
			return Op::DecR8(context, RegisterType8::REG_A);
		case 0x3E:
			return Op::LoadN8(context, RegisterType8::REG_A);
		case 0x3F:
			return Op::ComplementCarryFlag(context);
		case 0x40:
			return Op::LoadR8(context, RegisterType8::REG_B, RegisterType8::REG_B);
		case 0x41:
			return Op::LoadR8(context, RegisterType8::REG_B, RegisterType8::REG_C);
		case 0x42:
			return Op::LoadR8(context, RegisterType8::REG_B, RegisterType8::REG_D);
		case 0x43:
			return Op::LoadR8(context, RegisterType8::REG_B, RegisterType8::REG_E);
		case 0x44:
			return Op::LoadR8(context, RegisterType8::REG_B, RegisterType8::REG_H);
		case 0x45:
			return Op::LoadR8(context, RegisterType8::REG_B, RegisterType8::REG_L);
		case 0x46:
			return Op::LoadIndirectR16(context, RegisterType8::REG_B, RegisterType16::REG_HL);
		case 0x47:
			return Op::LoadR8(context, RegisterType8::REG_B, RegisterType8::REG_A);
		case 0x48:
			return Op::LoadR8(context, RegisterType8::REG_C, RegisterType8::REG_B);
		case 0x49:
			return Op::LoadR8(context, RegisterType8::REG_C, RegisterType8::REG_C);
		case 0x4A:
			return Op::LoadR8(context, RegisterType8::REG_C, RegisterType8::REG_D);
		case 0x4B:
			return Op::LoadR8(context, RegisterType8::REG_C, RegisterType8::REG_E);
		case 0x4C:
			return Op::LoadR8(context, RegisterType8::REG_C, RegisterType8::REG_H);
		case 0x4D:
			return Op::LoadR8(context, RegisterType8::REG_C, RegisterType8::REG_L);
		case 0x4E:
			return Op::LoadIndirectR16(context, RegisterType8::REG_C, RegisterType16::REG_HL);
		case 0x4F:
			return Op::LoadR8(context, RegisterType8::REG_C, RegisterType8::REG_A);
		case 0x50:
			return Op::LoadR8(context, RegisterType8::REG_D, RegisterType8::REG_B);
		case 0x51:
			return Op::LoadR8(context, RegisterType8::REG_D, RegisterType8::REG_C);
		case 0x52:
			return Op::LoadR8(context, RegisterType8::REG_D, RegisterType8::REG_D);
		case 0x53:
			return Op::LoadR8(context, RegisterType8::REG_D, RegisterType8::REG_E);
		case 0x54:
			return Op::LoadR8(context, RegisterType8::REG_D, RegisterType8::REG_H);
		case 0x55:
			return Op::LoadR8(context, RegisterType8::REG_D, RegisterType8::REG_L);
		case 0x56:
			return Op::LoadIndirectR16(context, RegisterType8::REG_D, RegisterType16::REG_HL);
		case 0x57:
			return Op::LoadR8(context, RegisterType8::REG_D, RegisterType8::REG_A);
		case 0x58:
			return Op::LoadR8(context, RegisterType8::REG_E, RegisterType8::REG_B);
		case 0x59:
			return Op::LoadR8(context, RegisterType8::REG_E, RegisterType8::REG_C);
		case 0x5A:
			return Op::LoadR8(context, RegisterType8::REG_E, RegisterType8::REG_D);
		case 0x5B:
			return Op::LoadR8(context, RegisterType8::REG_E, RegisterType8::REG_E);
		case 0x5C:
			return Op::LoadR8(context, RegisterType8::REG_E, RegisterType8::REG_H);
		case 0x5D:
			return Op::LoadR8(context, RegisterType8::REG_E, RegisterType8::REG_L);
		case 0x5E:
			return Op::LoadIndirectR16(context, RegisterType8::REG_E, RegisterType16::REG_HL);
		case 0x5F:
			return Op::LoadR8(context, RegisterType8::REG_E, RegisterType8::REG_A);
		case 0x60:
			return Op::LoadR8(context, RegisterType8::REG_H, RegisterType8::REG_B);
		case 0x61:
			return Op::LoadR8(context, RegisterType8::REG_H, RegisterType8::REG_C);
		case 0x62:
			return Op::LoadR8(context, RegisterType8::REG_H, RegisterType8::REG_D);
		case 0x63:
			return Op::LoadR8(context, RegisterType8::REG_H, RegisterType8::REG_E);
		case 0x64:
			return Op::LoadR8(context, RegisterType8::REG_H, RegisterType8::REG_H);
		case 0x65:
			return Op::LoadR8(context, RegisterType8::REG_H, RegisterType8::REG_L);
		case 0x66:
			return Op::LoadIndirectR16(context, RegisterType8::REG_H, RegisterType16::REG_HL);
		case 0x67:
			return Op::LoadR8(context, RegisterType8::REG_H, RegisterType8::REG_A);
		case 0x68:
			return Op::LoadR8(context, RegisterType8::REG_L, RegisterType8::REG_B);
		case 0x69:
			return Op::LoadR8(context, RegisterType8::REG_L, RegisterType8::REG_C);
		case 0x6A:
			return Op::LoadR8(context, RegisterType8::REG_L, RegisterType8::REG_D);
		case 0x6B:
			return Op::LoadR8(context, RegisterType8::REG_L, RegisterType8::REG_E);
		case 0x6C:
			return Op::LoadR8(context, RegisterType8::REG_L, RegisterType8::REG_H);
		case 0x6D:
			return Op::LoadR8(context, RegisterType8::REG_L, RegisterType8::REG_L);
		case 0x6E:
			return Op::LoadIndirectR16(context, RegisterType8::REG_L, RegisterType16::REG_HL);
		case 0x6F:
			return Op::LoadR8(context, RegisterType8::REG_L, RegisterType8::REG_A);
		case 0x70:
			return Op::StoreR8(context, RegisterType8::REG_B, RegisterType16::REG_HL);
		case 0x71:
			return Op::StoreR8(context, RegisterType8::REG_C, RegisterType16::REG_HL);
		case 0x72:
			return Op::StoreR8(context, RegisterType8::REG_D, RegisterType16::REG_HL);
		case 0x73:
			return Op::StoreR8(context, RegisterType8::REG_E, RegisterType16::REG_HL);
		case 0x74:
			return Op::StoreR8(context, RegisterType8::REG_H, RegisterType16::REG_HL);
		case 0x75:
			return Op::StoreR8(context, RegisterType8::REG_L, RegisterType16::REG_HL);
		case 0x77:
			return Op::StoreR8(context, RegisterType8::REG_A, RegisterType16::REG_HL);
		case 0x78:
			return Op::LoadR8(context, RegisterType8::REG_A, RegisterType8::REG_B);
		case 0x79:
			return Op::LoadR8(context, RegisterType8::REG_A, RegisterType8::REG_C);
		case 0x7A:
			return Op::LoadR8(context, RegisterType8::REG_A, RegisterType8::REG_D);
		case 0x7B:
			return Op::LoadR8(context, RegisterType8::REG_A, RegisterType8::REG_E);
		case 0x7C:
			return Op::LoadR8(context, RegisterType8::REG_A, RegisterType8::REG_H);
		case 0x7D:
			return Op::LoadR8(context, RegisterType8::REG_A, RegisterType8::REG_L);
		case 0x7E:
			return Op::LoadIndirectR16(context, RegisterType8::REG_A, RegisterType16::REG_HL);
		case 0x7F:
			return Op::LoadR8(context, RegisterType8::REG_A, RegisterType8::REG_A);
		case 0x80:
			return Op::AddR8(context, RegisterType8::REG_B);
		case 0x81:
			return Op::AddR8(context, RegisterType8::REG_C);
		case 0x82:
			return Op::AddR8(context, RegisterType8::REG_D);
		case 0x83:
			return Op::AddR8(context, RegisterType8::REG_E);
		case 0x84:
			return Op::AddR8(context, RegisterType8::REG_H);
		case 0x85:
			return Op::AddR8(context, RegisterType8::REG_L);
		case 0x86:
			return Op::AddIndirectHL(context);
		case 0x87:
			return Op::AddR8(context, RegisterType8::REG_A);
		case 0x88:
			return Op::AddCarryR8(context, RegisterType8::REG_B);
		case 0x89:
			return Op::AddCarryR8(context, RegisterType8::REG_C);
		case 0x8A:
			return Op::AddCarryR8(context, RegisterType8::REG_D);
		case 0x8B:
			return Op::AddCarryR8(context, RegisterType8::REG_E);
		case 0x8C:
			return Op::AddCarryR8(context, RegisterType8::REG_H);
		case 0x8D:
			return Op::AddCarryR8(context, RegisterType8::REG_L);
		case 0x8e:
			return Op::AddCarryIndirectHL(context);
		case 0x8F:
			return Op::AddCarryR8(context, RegisterType8::REG_A);
		case 0x90:
			return Op::SubR8(context, RegisterType8::REG_B);
		case 0x91:
			return Op::SubR8(context, RegisterType8::REG_C);
		case 0x92:
			return Op::SubR8(context, RegisterType8::REG_D);
		case 0x93:
			return Op::SubR8(context, RegisterType8::REG_E);
		case 0x94:
			return Op::SubR8(context, RegisterType8::REG_H);
		case 0x95:
			return Op::SubR8(context, RegisterType8::REG_L);
		case 0x96:
			return Op::SubIndirectHL(context);
		case 0x97:
			return Op::SubR8(context, RegisterType8::REG_A);
		case 0x98:
			return Op::SubCarryR8(context, RegisterType8::REG_B);
		case 0x99:
			return Op::SubCarryR8(context, RegisterType8::REG_C);
		case 0x9A:
			return Op::SubCarryR8(context, RegisterType8::REG_D);
		case 0x9B:
			return Op::SubCarryR8(context, RegisterType8::REG_E);
		case 0x9C:
			return Op::SubCarryR8(context, RegisterType8::REG_H);
		case 0x9D:
			return Op::SubCarryR8(context, RegisterType8::REG_L);
		case 0x9E:
			return Op::SubCarryIndirectHL(context);
		case 0x9F:
			return Op::SubCarryR8(context, RegisterType8::REG_A);
		case 0xA0:
			return Op::AndR8(context, RegisterType8::REG_B);
		case 0xA1:
			return Op::AndR8(context, RegisterType8::REG_C);
		case 0xA2:
			return Op::AndR8(context, RegisterType8::REG_D);
		case 0xA3:
			return Op::AndR8(context, RegisterType8::REG_E);
		case 0xA4:
			return Op::AndR8(context, RegisterType8::REG_H);
		case 0xA5:
			return Op::AndR8(context, RegisterType8::REG_L);
		case 0xA6:
			return Op::AndIndirectHL(context);
		case 0xA7:
			return Op::AndR8(context, RegisterType8::REG_A);
		case 0xA8:
			return Op::XorR8(context, RegisterType8::REG_B);
		case 0xA9:
			return Op::XorR8(context, RegisterType8::REG_C);
		case 0xAA:
			return Op::XorR8(context, RegisterType8::REG_D);
		case 0xAB:
			return Op::XorR8(context, RegisterType8::REG_E);
		case 0xAC:
			return Op::XorR8(context, RegisterType8::REG_H);
		case 0xAD:
			return Op::XorR8(context, RegisterType8::REG_L);
		case 0xAE:
			return Op::XorR16(context, RegisterType16::REG_HL);
		case 0xAF:
			return Op::XorR8(context, RegisterType8::REG_A);
		case 0xB0:
			return Op::OrR8(context, RegisterType8::REG_B);
		case 0xB1:
			return Op::OrR8(context, RegisterType8::REG_C);
		case 0xB2:
			return Op::OrR8(context, RegisterType8::REG_D);
		case 0xB3:
			return Op::OrR8(context, RegisterType8::REG_E);
		case 0xB4:
			return Op::OrR8(context, RegisterType8::REG_H);
		case 0xB5:
			return Op::OrR8(context, RegisterType8::REG_L);
		case 0xB6:
			return Op::OrHL(context);
		case 0xB7:
			return Op::OrR8(context, RegisterType8::REG_A);
		case 0xB8:
			return Op::CompareR8(context, RegisterType8::REG_B);
		case 0xB9:
			return Op::CompareR8(context, RegisterType8::REG_C);
		case 0xBA:
			return Op::CompareR8(context, RegisterType8::REG_D);
		case 0xBB:
			return Op::CompareR8(context, RegisterType8::REG_E);
		case 0xBC:
			return Op::CompareR8(context, RegisterType8::REG_H);
		case 0xBD:
			return Op::CompareR8(context, RegisterType8::REG_L);
		case 0xBE:
			return Op::CompareIndirectHL(context);
		case 0xBF:
			return Op::CompareR8(context, RegisterType8::REG_A);
		case 0xC0:
			return Op::ReturnFlagNotSet(context, CpuFlag::Zero);
		case 0xC1:
			return Op::PopR16(context, RegisterType16::REG_BC);
		case 0xC2:
			return Op::JumpFlagN16(context, CpuFlag::Zero, false);
		case 0xC3:
			return Op::JumpN16(context);
		case 0xC4:
			return Op::CallN16FlagNotSet(context, CpuFlag::Zero);
		case 0xC5:
			return Op::PushR16(context, RegisterType16::REG_BC);
		case 0xC6:
			return Op::AddN8(context);
		case 0xC7:
			return Op::Rst(context, 0x0);
		case 0xC8:
			return Op::ReturnFlagSet(context, CpuFlag::Zero);
		case 0xC9:
			return Op::Return(context);
		case 0xCA:
			return Op::JumpFlagN16(context, CpuFlag::Zero, true);
		case 0xCB:
			return Op::ExtendedPrefix(context);
		case 0xCC:
			return Op::CallN16FlagSet(context, CpuFlag::Zero);
		case 0xCD:
			return Op::CallN16(context);
		case 0xCE:
			return Op::AddCarryN8(context);
		case 0xCF:
			return Op::Rst(context, 0x08);
		case 0xD0:
			return Op::ReturnFlagNotSet(context, CpuFlag::Carry);
		case 0xD1:
			return Op::PopR16(context, RegisterType16::REG_DE);
		case 0xD2:
			return Op::JumpFlagN16(context, CpuFlag::Carry, false);
		case 0xD4:
			return Op::CallN16FlagNotSet(context, CpuFlag::Carry);
		case 0xD5:
			return Op::PushR16(context, RegisterType16::REG_DE);
		case 0xD6:
			return Op::SubN8(context);
		case 0xD7:
			return Op::Rst(context, 0x10);
		case 0xD8:
			return Op::ReturnFlagSet(context, CpuFlag::Carry);
		case 0xD9:
			return Op::ReturnInterrupt(context);
		case 0xDA:
			return Op::JumpFlagN16(context, CpuFlag::Carry, true);
		case 0xDC:
			return Op::CallN16FlagSet(context, CpuFlag::Carry);
		case 0xDE:
			return Op::SubCarryN8(context);
		case 0xDF:
			return Op::Rst(context, 0x18);
		case 0xE0:
			return Op::StoreFF00(context);
		case 0xE1:
			return Op::PopR16(context, RegisterType16::REG_HL);
		case 0xEA:
			return Op::StoreIndirectR8(context, RegisterType8::REG_A);
		case 0xE2:
			return Op::StoreIndirectAC(context);
		case 0xE5:
			return Op::PushR16(context, RegisterType16::REG_HL);
		case 0xE6:
			return Op::AndN8(context);
		case 0xE7:
			return Op::Rst(context, 0x20);
		case 0xE8:
			return Op::AddSP(context);
		case 0xE9:
			return Op::JumpHL(context);
		case 0xEE:
			return Op::XorN8(context);
		case 0xEF:
			return Op::Rst(context, 0x28);
		case 0xF0:
			return Op::LoadFF00(context);
		case 0xF1:
			return Op::PopR16(context, RegisterType16::REG_AF);
		case 0xF2:
			return Op::LoadIndirectAC(context);
		case 0xF3:
			return Op::DisableInterrupts(context);
		case 0xF5:
			return Op::PushR16(context, RegisterType16::REG_AF);
		case 0xF6:
			return Op::OrN8(context);
		case 0xF7:
			return Op::Rst(context, 0x30);
		case 0xF8:
			return Op::LoadHLFromSPRelative(context);
		case 0xF9:
			return Op::LoadHLFromSP(context);
		case 0xFA:
			return Op::LoadIndirectR8(context, RegisterType8::REG_A);
		case 0xFB:
			return Op::EnableInterrupts(context);
		case 0xFE:
			return Op::CompareN8(context);
		case 0xFF:
			return Op::Rst(context, 0x38);
		default:
			throw std::exception(std::format("Instruction not implemented: 0x{:x}", opcode).c_str());
	}
}
