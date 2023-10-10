#include "Cpu.h"
#include <exception>
#include <sstream>

void Cpu::SetRegister(RegisterType8 type, uint8_t data)
{
	m_Registers[type] = data;
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
			m_StackPointer = data_low | (data_high << 8);
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
			return m_StackPointer;
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
				m_Registers[RegisterType8::REG_F] |= (1 << 6);
				break;

			case CpuFlag::Subtraction:
				m_Registers[RegisterType8::REG_F] |= (1 << 5);
				break;

			case CpuFlag::HalfCarry:
				m_Registers[RegisterType8::REG_F] |= (1 << 4);
				break;

			case CpuFlag::Carry:
				m_Registers[RegisterType8::REG_F] |= (1 << 3);
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
				m_Registers[RegisterType8::REG_F] &= ~(1 << 6);
				break;

			case CpuFlag::Subtraction:
				m_Registers[RegisterType8::REG_F] &= ~(1 << 5);
				break;

			case CpuFlag::HalfCarry:
				m_Registers[RegisterType8::REG_F] &= ~(1 << 4);
				break;

			case CpuFlag::Carry:
				m_Registers[RegisterType8::REG_F] &= ~(1 << 3);
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
			return ((flag_register >> 6) & 1) != 0;

		case CpuFlag::Subtraction:
			return ((flag_register >> 5) & 1) != 0;

		case CpuFlag::HalfCarry:
			return ((flag_register >> 4) & 1) != 0;

		case CpuFlag::Carry:
			return ((flag_register >> 3) & 1) != 0;
	}

	// TODO: Should this should throw an exception or just log and continue
	return false;
}

std::string Cpu::Details()
{
	std::stringstream ss;
	ss.setf(std::ios_base::hex, std::ios_base::basefield);

	ss << "Registers("
		<< " A: 0x" << +GetRegister(RegisterType8::REG_A)
		<< " F: 0x" << +GetRegister(RegisterType8::REG_F)
		<< " B: 0x" << +GetRegister(RegisterType8::REG_B)
		<< " C: 0x" << +GetRegister(RegisterType8::REG_C)
		<< " D: 0x" << +GetRegister(RegisterType8::REG_D)
		<< " E: 0x" << +GetRegister(RegisterType8::REG_E)
		<< " H: 0x" << +GetRegister(RegisterType8::REG_H)
		<< " L: 0x" << +GetRegister(RegisterType8::REG_L)
		<< " )";

	ss << " Flags("
		<< " Z: 0x" << GetFlag(CpuFlag::Zero)
		<< " N: 0x" << GetFlag(CpuFlag::Subtraction)
		<< " H: 0x" << GetFlag(CpuFlag::HalfCarry)
		<< " C: 0x" << GetFlag(CpuFlag::Carry)
		<< " )";

	return ss.str();
}

std::string RegisterTypeString16(RegisterType16 type)
{
	switch (type)
	{
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