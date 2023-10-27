#include "Cpu.h"
#include <exception>
#include <sstream>
#include <iomanip>
#include "Emulator.h"

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