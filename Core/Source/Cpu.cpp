#include "Pch.h"
#include "Cpu.h"
#include "Emulator.h"
#include "Instructions.h"
#include <exception>
#include <sstream>
#include <format>

Cpu::Cpu()
{
	SetRegister(RegisterType8::REG_A, 0x11);
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
		throw std::exception("GetRegister failed unable to find register type");
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

	throw std::exception("unsupported CpuFlag in function GetFlag");
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
		if (InterruptCheck(InterruptFlag::VBlank))
		{
			InterruptHandle(InterruptFlag::VBlank, 0x40);
		}
		else if (InterruptCheck(InterruptFlag::STAT))
		{
			InterruptHandle(InterruptFlag::STAT, 0x48);
		}
		else if (InterruptCheck(InterruptFlag::Timer))
		{
			InterruptHandle(InterruptFlag::Timer, 0x50);
		}
		else if (InterruptCheck(InterruptFlag::Serial))
		{
			InterruptHandle(InterruptFlag::Serial, 0x58);
		}
		else if (InterruptCheck(InterruptFlag::Joypad))
		{
			InterruptHandle(InterruptFlag::Joypad, 0x60);
		}

		m_EnablingInterrupts = false;
	}

	if (m_EnablingInterrupts)
	{
		m_InterruptMasterFlag = true;
	}
}

bool Cpu::InterruptCheck(InterruptFlag flag)
{
	if (m_InterruptFlags & static_cast<int>(flag) && m_InterruptEnable & static_cast<int>(flag))
	{
		return true;
	}

	return false;
}

void Cpu::InterruptHandle(InterruptFlag flag, uint16_t address)
{
	Emulator::Instance->Cycle(2);

	Emulator::Instance->StackPush16(ProgramCounter);
	Emulator::Instance->Cycle(2);

	ProgramCounter = address;
	Emulator::Instance->Cycle(1);

	m_InterruptFlags &= ~static_cast<int>(flag);
	Emulator::Instance->SetHalt(false);
	m_InterruptMasterFlag = false;
}

uint8_t Cpu::GetInterruptFlags()
{
	return m_InterruptFlags;
}

void Cpu::SaveState(std::fstream* file)
{
	file->write(reinterpret_cast<const char*>(&ProgramCounter), sizeof(uint16_t));
	file->write(reinterpret_cast<const char*>(&StackPointer), sizeof(uint16_t));

	file->write(reinterpret_cast<const char*>(&m_Registers[RegisterType8::REG_A]), sizeof(uint8_t));
	file->write(reinterpret_cast<const char*>(&m_Registers[RegisterType8::REG_F]), sizeof(uint8_t));
	file->write(reinterpret_cast<const char*>(&m_Registers[RegisterType8::REG_B]), sizeof(uint8_t));
	file->write(reinterpret_cast<const char*>(&m_Registers[RegisterType8::REG_C]), sizeof(uint8_t));
	file->write(reinterpret_cast<const char*>(&m_Registers[RegisterType8::REG_D]), sizeof(uint8_t));
	file->write(reinterpret_cast<const char*>(&m_Registers[RegisterType8::REG_E]), sizeof(uint8_t));
	file->write(reinterpret_cast<const char*>(&m_Registers[RegisterType8::REG_H]), sizeof(uint8_t));
	file->write(reinterpret_cast<const char*>(&m_Registers[RegisterType8::REG_L]), sizeof(uint8_t));

	file->write(reinterpret_cast<const char*>(&m_InterruptFlags), sizeof(uint8_t));
	file->write(reinterpret_cast<const char*>(&m_InterruptEnable), sizeof(uint8_t));

	file->write(reinterpret_cast<const char*>(&m_EnablingInterrupts), sizeof(bool));
	file->write(reinterpret_cast<const char*>(&m_InterruptMasterFlag), sizeof(bool));
}

void Cpu::LoadState(std::fstream* file)
{
	file->read(reinterpret_cast<char*>(&ProgramCounter), sizeof(uint16_t));
	file->read(reinterpret_cast<char*>(&StackPointer), sizeof(uint16_t));

	file->read(reinterpret_cast<char*>(&m_Registers[RegisterType8::REG_A]), sizeof(uint8_t));
	file->read(reinterpret_cast<char*>(&m_Registers[RegisterType8::REG_F]), sizeof(uint8_t));
	file->read(reinterpret_cast<char*>(&m_Registers[RegisterType8::REG_B]), sizeof(uint8_t));
	file->read(reinterpret_cast<char*>(&m_Registers[RegisterType8::REG_C]), sizeof(uint8_t));
	file->read(reinterpret_cast<char*>(&m_Registers[RegisterType8::REG_D]), sizeof(uint8_t));
	file->read(reinterpret_cast<char*>(&m_Registers[RegisterType8::REG_E]), sizeof(uint8_t));
	file->read(reinterpret_cast<char*>(&m_Registers[RegisterType8::REG_H]), sizeof(uint8_t));
	file->read(reinterpret_cast<char*>(&m_Registers[RegisterType8::REG_L]), sizeof(uint8_t));

	file->read(reinterpret_cast<char*>(&m_InterruptFlags), sizeof(uint8_t));
	file->read(reinterpret_cast<char*>(&m_InterruptEnable), sizeof(uint8_t));

	file->read(reinterpret_cast<char*>(&m_EnablingInterrupts), sizeof(bool));
	file->read(reinterpret_cast<char*>(&m_InterruptMasterFlag), sizeof(bool));
}

void Cpu::Execute(EmulatorContext* context, const uint8_t opcode)
{
	switch (opcode)
	{
		case 0x0:
			Op::Nop(context);
			break;
		case 0x01:
			Op::LoadN16(context, RegisterType16::REG_BC);
			break;
		case 0x02:
			Op::StoreR8(context, RegisterType8::REG_A, RegisterType16::REG_BC);
			break;
		case 0x03:
			Op::IncR16(context, RegisterType16::REG_BC);
			break;
		case 0x04:
			Op::IncR8(context, RegisterType8::REG_B);
			break;
		case 0x05:
			Op::DecR8(context, RegisterType8::REG_B);
			break;
		case 0x06:
			Op::LoadN8(context, RegisterType8::REG_B);
			break;
		case 0x07:
			Op::RotateRegisterLeftCarryA(context);
			break;
		case 0x08:
			Op::LoadIndirectSP(context);
			break;
		case 0x09:
			Op::AddR16(context, RegisterType16::REG_BC);
			break;
		case 0x0A:
			Op::LoadIndirectR16(context, RegisterType8::REG_A, RegisterType16::REG_BC);
			break;
		case 0x0B:
			Op::DecR16(context, RegisterType16::REG_BC);
			break;
		case 0x0C:
			Op::IncR8(context, RegisterType8::REG_C);
			break;
		case 0x0D:
			Op::DecR8(context, RegisterType8::REG_C);
			break;
		case 0x0E:
			Op::LoadN8(context, RegisterType8::REG_C);
			break;
		case 0x0F:
			Op::RotateRegisterRightCarryA(context);
			break;
		case 0x10:
			Op::Stop(context);
			break;
		case 0x11:
			Op::LoadN16(context, RegisterType16::REG_DE);
			break;
		case 0x12:
			Op::StoreR8(context, RegisterType8::REG_A, RegisterType16::REG_DE);
			break;
		case 0x13:
			Op::IncR16(context, RegisterType16::REG_DE);
			break;
		case 0x14:
			Op::IncR8(context, RegisterType8::REG_D);
			break;
		case 0x15:
			Op::DecR8(context, RegisterType8::REG_D);
			break;
		case 0x16:
			Op::LoadN8(context, RegisterType8::REG_D);
			break;
		case 0x17:
			Op::RotateRegisterLeftA(context);
			break;
		case 0x18:
			Op::JumpRelativeN8(context);
			break;
		case 0x19:
			Op::AddR16(context, RegisterType16::REG_DE);
			break;
		case 0x1A:
			Op::LoadIndirectR16(context, RegisterType8::REG_A, RegisterType16::REG_DE);
			break;
		case 0x1B:
			Op::DecR16(context, RegisterType16::REG_DE);
			break;
		case 0x1C:
			Op::IncR8(context, RegisterType8::REG_E);
			break;
		case 0x1D:
			Op::DecR8(context, RegisterType8::REG_E);
			break;
		case 0x1E:
			Op::LoadN8(context, RegisterType8::REG_E);
			break;
		case 0x1F:
			Op::RotateRegisterRightA(context);
			break;
		case 0x20:
			Op::JumpRelativeFlagNotSet(context, CpuFlag::Zero);
			break;
		case 0x21:
			Op::LoadN16(context, RegisterType16::REG_HL);
			break;
		case 0x22:
			Op::StoreIncrementHL(context);
			break;
		case 0x23:
			Op::IncR16(context, RegisterType16::REG_HL);
			break;
		case 0x24:
			Op::IncR8(context, RegisterType8::REG_H);
			break;
		case 0x25:
			Op::DecR8(context, RegisterType8::REG_H);
			break;
		case 0x26:
			Op::LoadN8(context, RegisterType8::REG_H);
			break;
		case 0x27:
			Op::Daa(context);
			break;
		case 0x28:
			Op::JumpRelativeFlagSet(context, CpuFlag::Zero);
			break;
		case 0x29:
			Op::AddR16(context, RegisterType16::REG_HL);
			break;
		case 0x2A:
			Op::LoadIncrementHL(context);
			break;
		case 0x2B:
			Op::DecR16(context, RegisterType16::REG_HL);
			break;
		case 0x2C:
			Op::IncR8(context, RegisterType8::REG_L);
			break;
		case 0x2D:
			Op::DecR8(context, RegisterType8::REG_L);
			break;
		case 0x2E:
			Op::LoadN8(context, RegisterType8::REG_L);
			break;
		case 0x2F:
			Op::ComplementA(context);
			break;
		case 0x30:
			Op::JumpRelativeFlagNotSet(context, CpuFlag::Carry);
			break;
		case 0x31:
			Op::LoadN16(context, RegisterType16::REG_SP);
			break;
		case 0x32:
			Op::StoreDecrementHL(context);
			break;
		case 0x33:
			Op::IncR16(context, RegisterType16::REG_SP);
			break;
		case 0x34:
			Op::IncIndirectHL(context);
			break;
		case 0x35:
			Op::DecIndirectHL(context);
			break;
		case 0x36:
			Op::StoreN8(context, RegisterType16::REG_HL);
			break;
		case 0x37:
			Op::SetCarryFlag(context);
			break;
		case 0x38:
			Op::JumpRelativeFlagSet(context, CpuFlag::Carry);
			break;
		case 0x39:
			Op::AddR16(context, RegisterType16::REG_SP);
			break;
		case 0x3A:
			Op::LoadDecrementHL(context);
			break;
		case 0x3B:
			Op::DecR16(context, RegisterType16::REG_SP);
			break;
		case 0x3C:
			Op::IncR8(context, RegisterType8::REG_A);
			break;
		case 0x3D:
			Op::DecR8(context, RegisterType8::REG_A);
			break;
		case 0x3E:
			Op::LoadN8(context, RegisterType8::REG_A);
			break;
		case 0x3F:
			Op::ComplementCarryFlag(context);
			break;
		case 0x40:
			Op::LoadR8(context, RegisterType8::REG_B, RegisterType8::REG_B);
			break;
		case 0x41:
			Op::LoadR8(context, RegisterType8::REG_B, RegisterType8::REG_C);
			break;
		case 0x42:
			Op::LoadR8(context, RegisterType8::REG_B, RegisterType8::REG_D);
			break;
		case 0x43:
			Op::LoadR8(context, RegisterType8::REG_B, RegisterType8::REG_E);
			break;
		case 0x44:
			Op::LoadR8(context, RegisterType8::REG_B, RegisterType8::REG_H);
			break;
		case 0x45:
			Op::LoadR8(context, RegisterType8::REG_B, RegisterType8::REG_L);
			break;
		case 0x46:
			Op::LoadIndirectR16(context, RegisterType8::REG_B, RegisterType16::REG_HL);
			break;
		case 0x47:
			Op::LoadR8(context, RegisterType8::REG_B, RegisterType8::REG_A);
			break;
		case 0x48:
			Op::LoadR8(context, RegisterType8::REG_C, RegisterType8::REG_B);
			break;
		case 0x49:
			Op::LoadR8(context, RegisterType8::REG_C, RegisterType8::REG_C);
			break;
		case 0x4A:
			Op::LoadR8(context, RegisterType8::REG_C, RegisterType8::REG_D);
			break;
		case 0x4B:
			Op::LoadR8(context, RegisterType8::REG_C, RegisterType8::REG_E);
			break;
		case 0x4C:
			Op::LoadR8(context, RegisterType8::REG_C, RegisterType8::REG_H);
			break;
		case 0x4D:
			Op::LoadR8(context, RegisterType8::REG_C, RegisterType8::REG_L);
			break;
		case 0x4E:
			Op::LoadIndirectR16(context, RegisterType8::REG_C, RegisterType16::REG_HL);
			break;
		case 0x4F:
			Op::LoadR8(context, RegisterType8::REG_C, RegisterType8::REG_A);
			break;
		case 0x50:
			Op::LoadR8(context, RegisterType8::REG_D, RegisterType8::REG_B);
			break;
		case 0x51:
			Op::LoadR8(context, RegisterType8::REG_D, RegisterType8::REG_C);
			break;
		case 0x52:
			Op::LoadR8(context, RegisterType8::REG_D, RegisterType8::REG_D);
			break;
		case 0x53:
			Op::LoadR8(context, RegisterType8::REG_D, RegisterType8::REG_E);
			break;
		case 0x54:
			Op::LoadR8(context, RegisterType8::REG_D, RegisterType8::REG_H);
			break;
		case 0x55:
			Op::LoadR8(context, RegisterType8::REG_D, RegisterType8::REG_L);
			break;
		case 0x56:
			Op::LoadIndirectR16(context, RegisterType8::REG_D, RegisterType16::REG_HL);
			break;
		case 0x57:
			Op::LoadR8(context, RegisterType8::REG_D, RegisterType8::REG_A);
			break;
		case 0x58:
			Op::LoadR8(context, RegisterType8::REG_E, RegisterType8::REG_B);
			break;
		case 0x59:
			Op::LoadR8(context, RegisterType8::REG_E, RegisterType8::REG_C);
			break;
		case 0x5A:
			Op::LoadR8(context, RegisterType8::REG_E, RegisterType8::REG_D);
			break;
		case 0x5B:
			Op::LoadR8(context, RegisterType8::REG_E, RegisterType8::REG_E);
			break;
		case 0x5C:
			Op::LoadR8(context, RegisterType8::REG_E, RegisterType8::REG_H);
			break;
		case 0x5D:
			Op::LoadR8(context, RegisterType8::REG_E, RegisterType8::REG_L);
			break;
		case 0x5E:
			Op::LoadIndirectR16(context, RegisterType8::REG_E, RegisterType16::REG_HL);
			break;
		case 0x5F:
			Op::LoadR8(context, RegisterType8::REG_E, RegisterType8::REG_A);
			break;
		case 0x60:
			Op::LoadR8(context, RegisterType8::REG_H, RegisterType8::REG_B);
			break;
		case 0x61:
			Op::LoadR8(context, RegisterType8::REG_H, RegisterType8::REG_C);
			break;
		case 0x62:
			Op::LoadR8(context, RegisterType8::REG_H, RegisterType8::REG_D);
			break;
		case 0x63:
			Op::LoadR8(context, RegisterType8::REG_H, RegisterType8::REG_E);
			break;
		case 0x64:
			Op::LoadR8(context, RegisterType8::REG_H, RegisterType8::REG_H);
			break;
		case 0x65:
			Op::LoadR8(context, RegisterType8::REG_H, RegisterType8::REG_L);
			break;
		case 0x66:
			Op::LoadIndirectR16(context, RegisterType8::REG_H, RegisterType16::REG_HL);
			break;
		case 0x67:
			Op::LoadR8(context, RegisterType8::REG_H, RegisterType8::REG_A);
			break;
		case 0x68:
			Op::LoadR8(context, RegisterType8::REG_L, RegisterType8::REG_B);
			break;
		case 0x69:
			Op::LoadR8(context, RegisterType8::REG_L, RegisterType8::REG_C);
			break;
		case 0x6A:
			Op::LoadR8(context, RegisterType8::REG_L, RegisterType8::REG_D);
			break;
		case 0x6B:
			Op::LoadR8(context, RegisterType8::REG_L, RegisterType8::REG_E);
			break;
		case 0x6C:
			Op::LoadR8(context, RegisterType8::REG_L, RegisterType8::REG_H);
			break;
		case 0x6D:
			Op::LoadR8(context, RegisterType8::REG_L, RegisterType8::REG_L);
			break;
		case 0x6E:
			Op::LoadIndirectR16(context, RegisterType8::REG_L, RegisterType16::REG_HL);
			break;
		case 0x6F:
			Op::LoadR8(context, RegisterType8::REG_L, RegisterType8::REG_A);
			break;
		case 0x70:
			Op::StoreR8(context, RegisterType8::REG_B, RegisterType16::REG_HL);
			break;
		case 0x71:
			Op::StoreR8(context, RegisterType8::REG_C, RegisterType16::REG_HL);
			break;
		case 0x72:
			Op::StoreR8(context, RegisterType8::REG_D, RegisterType16::REG_HL);
			break;
		case 0x73:
			Op::StoreR8(context, RegisterType8::REG_E, RegisterType16::REG_HL);
			break;
		case 0x74:
			Op::StoreR8(context, RegisterType8::REG_H, RegisterType16::REG_HL);
			break;
		case 0x75:
			Op::StoreR8(context, RegisterType8::REG_L, RegisterType16::REG_HL);
			break;
		case 0x76:
			Op::Halt(context);
			break;
		case 0x77:
			Op::StoreR8(context, RegisterType8::REG_A, RegisterType16::REG_HL);
			break;
		case 0x78:
			Op::LoadR8(context, RegisterType8::REG_A, RegisterType8::REG_B);
			break;
		case 0x79:
			Op::LoadR8(context, RegisterType8::REG_A, RegisterType8::REG_C);
			break;
		case 0x7A:
			Op::LoadR8(context, RegisterType8::REG_A, RegisterType8::REG_D);
			break;
		case 0x7B:
			Op::LoadR8(context, RegisterType8::REG_A, RegisterType8::REG_E);
			break;
		case 0x7C:
			Op::LoadR8(context, RegisterType8::REG_A, RegisterType8::REG_H);
			break;
		case 0x7D:
			Op::LoadR8(context, RegisterType8::REG_A, RegisterType8::REG_L);
			break;
		case 0x7E:
			Op::LoadIndirectR16(context, RegisterType8::REG_A, RegisterType16::REG_HL);
			break;
		case 0x7F:
			Op::LoadR8(context, RegisterType8::REG_A, RegisterType8::REG_A);
			break;
		case 0x80:
			Op::AddR8(context, RegisterType8::REG_B);
			break;
		case 0x81:
			Op::AddR8(context, RegisterType8::REG_C);
			break;
		case 0x82:
			Op::AddR8(context, RegisterType8::REG_D);
			break;
		case 0x83:
			Op::AddR8(context, RegisterType8::REG_E);
			break;
		case 0x84:
			Op::AddR8(context, RegisterType8::REG_H);
			break;
		case 0x85:
			Op::AddR8(context, RegisterType8::REG_L);
			break;
		case 0x86:
			Op::AddIndirectHL(context);
			break;
		case 0x87:
			Op::AddR8(context, RegisterType8::REG_A);
			break;
		case 0x88:
			Op::AddCarryR8(context, RegisterType8::REG_B);
			break;
		case 0x89:
			Op::AddCarryR8(context, RegisterType8::REG_C);
			break;
		case 0x8A:
			Op::AddCarryR8(context, RegisterType8::REG_D);
			break;
		case 0x8B:
			Op::AddCarryR8(context, RegisterType8::REG_E);
			break;
		case 0x8C:
			Op::AddCarryR8(context, RegisterType8::REG_H);
			break;
		case 0x8D:
			Op::AddCarryR8(context, RegisterType8::REG_L);
			break;
		case 0x8e:
			Op::AddCarryIndirectHL(context);
			break;
		case 0x8F:
			Op::AddCarryR8(context, RegisterType8::REG_A);
			break;
		case 0x90:
			Op::SubR8(context, RegisterType8::REG_B);
			break;
		case 0x91:
			Op::SubR8(context, RegisterType8::REG_C);
			break;
		case 0x92:
			Op::SubR8(context, RegisterType8::REG_D);
			break;
		case 0x93:
			Op::SubR8(context, RegisterType8::REG_E);
			break;
		case 0x94:
			Op::SubR8(context, RegisterType8::REG_H);
			break;
		case 0x95:
			Op::SubR8(context, RegisterType8::REG_L);
			break;
		case 0x96:
			Op::SubIndirectHL(context);
			break;
		case 0x97:
			Op::SubR8(context, RegisterType8::REG_A);
			break;
		case 0x98:
			Op::SubCarryR8(context, RegisterType8::REG_B);
			break;
		case 0x99:
			Op::SubCarryR8(context, RegisterType8::REG_C);
			break;
		case 0x9A:
			Op::SubCarryR8(context, RegisterType8::REG_D);
			break;
		case 0x9B:
			Op::SubCarryR8(context, RegisterType8::REG_E);
			break;
		case 0x9C:
			Op::SubCarryR8(context, RegisterType8::REG_H);
			break;
		case 0x9D:
			Op::SubCarryR8(context, RegisterType8::REG_L);
			break;
		case 0x9E:
			Op::SubCarryIndirectHL(context);
			break;
		case 0x9F:
			Op::SubCarryR8(context, RegisterType8::REG_A);
			break;
		case 0xA0:
			Op::AndR8(context, RegisterType8::REG_B);
			break;
		case 0xA1:
			Op::AndR8(context, RegisterType8::REG_C);
			break;
		case 0xA2:
			Op::AndR8(context, RegisterType8::REG_D);
			break;
		case 0xA3:
			Op::AndR8(context, RegisterType8::REG_E);
			break;
		case 0xA4:
			Op::AndR8(context, RegisterType8::REG_H);
			break;
		case 0xA5:
			Op::AndR8(context, RegisterType8::REG_L);
			break;
		case 0xA6:
			Op::AndIndirectHL(context);
			break;
		case 0xA7:
			Op::AndR8(context, RegisterType8::REG_A);
			break;
		case 0xA8:
			Op::XorR8(context, RegisterType8::REG_B);
			break;
		case 0xA9:
			Op::XorR8(context, RegisterType8::REG_C);
			break;
		case 0xAA:
			Op::XorR8(context, RegisterType8::REG_D);
			break;
		case 0xAB:
			Op::XorR8(context, RegisterType8::REG_E);
			break;
		case 0xAC:
			Op::XorR8(context, RegisterType8::REG_H);
			break;
		case 0xAD:
			Op::XorR8(context, RegisterType8::REG_L);
			break;
		case 0xAE:
			Op::XorR16(context, RegisterType16::REG_HL);
			break;
		case 0xAF:
			Op::XorR8(context, RegisterType8::REG_A);
			break;
		case 0xB0:
			Op::OrR8(context, RegisterType8::REG_B);
			break;
		case 0xB1:
			Op::OrR8(context, RegisterType8::REG_C);
			break;
		case 0xB2:
			Op::OrR8(context, RegisterType8::REG_D);
			break;
		case 0xB3:
			Op::OrR8(context, RegisterType8::REG_E);
			break;
		case 0xB4:
			Op::OrR8(context, RegisterType8::REG_H);
			break;
		case 0xB5:
			Op::OrR8(context, RegisterType8::REG_L);
			break;
		case 0xB6:
			Op::OrHL(context);
			break;
		case 0xB7:
			Op::OrR8(context, RegisterType8::REG_A);
			break;
		case 0xB8:
			Op::CompareR8(context, RegisterType8::REG_B);
			break;
		case 0xB9:
			Op::CompareR8(context, RegisterType8::REG_C);
			break;
		case 0xBA:
			Op::CompareR8(context, RegisterType8::REG_D);
			break;
		case 0xBB:
			Op::CompareR8(context, RegisterType8::REG_E);
			break;
		case 0xBC:
			Op::CompareR8(context, RegisterType8::REG_H);
			break;
		case 0xBD:
			Op::CompareR8(context, RegisterType8::REG_L);
			break;
		case 0xBE:
			Op::CompareIndirectHL(context);
			break;
		case 0xBF:
			Op::CompareR8(context, RegisterType8::REG_A);
			break;
		case 0xC0:
			Op::ReturnFlagNotSet(context, CpuFlag::Zero);
			break;
		case 0xC1:
			Op::PopR16(context, RegisterType16::REG_BC);
			break;
		case 0xC2:
			Op::JumpFlagN16(context, CpuFlag::Zero, false);
			break;
		case 0xC3:
			Op::JumpN16(context);
			break;
		case 0xC4:
			Op::CallN16FlagNotSet(context, CpuFlag::Zero);
			break;
		case 0xC5:
			Op::PushR16(context, RegisterType16::REG_BC);
			break;
		case 0xC6:
			Op::AddN8(context);
			break;
		case 0xC7:
			Op::Rst(context, 0x0);
			break;
		case 0xC8:
			Op::ReturnFlagSet(context, CpuFlag::Zero);
			break;
		case 0xC9:
			Op::Return(context);
			break;
		case 0xCA:
			Op::JumpFlagN16(context, CpuFlag::Zero, true);
			break;
		case 0xCB:
			Op::ExtendedPrefix(context);
			break;
		case 0xCC:
			Op::CallN16FlagSet(context, CpuFlag::Zero);
			break;
		case 0xCD:
			Op::CallN16(context);
			break;
		case 0xCE:
			Op::AddCarryN8(context);
			break;
		case 0xCF:
			Op::Rst(context, 0x08);
			break;
		case 0xD0:
			Op::ReturnFlagNotSet(context, CpuFlag::Carry);
			break;
		case 0xD1:
			Op::PopR16(context, RegisterType16::REG_DE);
			break;
		case 0xD2:
			Op::JumpFlagN16(context, CpuFlag::Carry, false);
			break;
		case 0xD4:
			Op::CallN16FlagNotSet(context, CpuFlag::Carry);
			break;
		case 0xD5:
			Op::PushR16(context, RegisterType16::REG_DE);
			break;
		case 0xD6:
			Op::SubN8(context);
			break;
		case 0xD7:
			Op::Rst(context, 0x10);
			break;
		case 0xD8:
			Op::ReturnFlagSet(context, CpuFlag::Carry);
			break;
		case 0xD9:
			Op::ReturnInterrupt(context);
			break;
		case 0xDA:
			Op::JumpFlagN16(context, CpuFlag::Carry, true);
			break;
		case 0xDC:
			Op::CallN16FlagSet(context, CpuFlag::Carry);
			break;
		case 0xDE:
			Op::SubCarryN8(context);
			break;
		case 0xDF:
			Op::Rst(context, 0x18);
			break;
		case 0xE0:
			Op::StoreIO(context);
			break;
		case 0xE1:
			Op::PopR16(context, RegisterType16::REG_HL);
			break;
		case 0xEA:
			Op::StoreIndirectR8(context, RegisterType8::REG_A);
			break;
		case 0xE2:
			Op::StoreIndirectAC(context);
			break;
		case 0xE5:
			Op::PushR16(context, RegisterType16::REG_HL);
			break;
		case 0xE6:
			Op::AndN8(context);
			break;
		case 0xE7:
			Op::Rst(context, 0x20);
			break;
		case 0xE8:
			Op::AddSP(context);
			break;
		case 0xE9:
			Op::JumpHL(context);
			break;
		case 0xEE:
			Op::XorN8(context);
			break;
		case 0xEF:
			Op::Rst(context, 0x28);
			break;
		case 0xF0:
			Op::LoadIO(context);
			break;
		case 0xF1:
			Op::PopR16(context, RegisterType16::REG_AF);
			break;
		case 0xF2:
			Op::LoadIndirectAC(context);
			break;
		case 0xF3:
			Op::DisableInterrupts(context);
			break;
		case 0xF5:
			Op::PushR16(context, RegisterType16::REG_AF);
			break;
		case 0xF6:
			Op::OrN8(context);
			break;
		case 0xF7:
			Op::Rst(context, 0x30);
			break;
		case 0xF8:
			Op::LoadHLFromSPRelative(context);
			break;
		case 0xF9:
			Op::LoadHLFromSP(context);
			break;
		case 0xFA:
			Op::LoadIndirectR8(context, RegisterType8::REG_A);
			break;
		case 0xFB:
			Op::EnableInterrupts(context);
			break;
		case 0xFE:
			Op::CompareN8(context);
			break;
		case 0xFF:
			Op::Rst(context, 0x38);
			break;
		default:
			throw std::exception(std::format("Instruction not implemented: 0x{:x}", opcode).c_str());
	}
}
