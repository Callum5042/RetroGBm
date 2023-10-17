#include "Emulator.h"
#include "Cpu.h"
#include <iostream>
#include <exception>
#include <string>
#include "Bus.h"
#include "Instructions.h"

Emulator::Emulator()
{
	m_Context.cpu = std::make_unique<Cpu>();
	m_Context.cartridge = std::make_unique<CartridgeInfo>();

	m_Context.work_ram.resize(1024 * 8);
	std::fill(m_Context.work_ram.begin(), m_Context.work_ram.end(), 0x0);

	m_Context.video_ram.resize(1024 * 8);
	std::fill(m_Context.video_ram.begin(), m_Context.video_ram.end(), 0x0);

	std::fill(m_Context.high_ram.begin(), m_Context.high_ram.end(), 0x0);

	// Build opcode table
	/*m_OpCodeTable[0x0] = [&]() { return Op::Nop(&m_Context); };
	m_OpCodeTable[0xC3] = [&]() { return Op::JumpN16(&m_Context); };
	m_OpCodeTable[0xAF] = [&]() { return Op::Xor(&m_Context, RegisterType8::REG_A, RegisterType8::REG_A); };
	m_OpCodeTable[0x21] = [&]() { return Op::LoadN16(&m_Context, RegisterType16::REG_HL); };*/
}

bool Emulator::LoadRom(const std::filesystem::path& path)
{
	if (!LoadCartridge(path, m_Context.cartridge.get()))
	{
		std::cerr << "Unable to load cartidge\n";
		return false;
	}

	uint8_t checksum_result = 0x0;
	bool checksum = CartridgeChecksum(m_Context.cartridge.get(), &checksum_result);

	// Print cartridge info
	/*std::cout << "Cartidge loaded\n";
	std::cout << "> Title: " << m_Context.cartridge->title << '\n';
	std::cout << "> Cartridge Type: " << m_Context.cartridge->header.cartridge_type << std::format(" (0x{:x})", static_cast<int>(m_Context.cartridge->header.cartridge_type_code)) << '\n';
	std::cout << "> ROM size: " << m_Context.cartridge->header.rom_size << '\n';
	std::cout << "> ROM banks: " << m_Context.cartridge->header.rom_banks << '\n';
	std::cout << "> RAM size: " << m_Context.cartridge->header.ram_size << '\n';
	std::cout << "> License: " << m_Context.cartridge->header.license << '\n';
	std::cout << "> Version: " << m_Context.cartridge->header.version << '\n';
	std::cout << "> Checksum: " << std::format("(0x{:x}) ", checksum_result) << (checksum ? "Passed" : "Failed") << '\n' << '\n';*/

	// Set program counter to 0x100 to skip boot rom
	m_Context.cpu->ProgramCounter = 0x100;

	m_Context.timer.div = 0xAB;
	m_Context.timer.tac = 0xF8;
	m_Context.timer.tma = 0x0;
	m_Context.timer.tima = 0x0;

	m_Context.display.lcdc = 0x91;
	m_Context.display.stat = 0x85;
	m_Context.display.scx = 0x0;
	m_Context.display.scy = 0x0;
	m_Context.display.ly = 0x0;
	m_Context.display.lyc = 0x0;
	m_Context.display.dma = 0xFF;
	m_Context.display.bgp = 0xFC;
	m_Context.display.obp0 = 0x0;
	m_Context.display.obp1 = 0x0;
	m_Context.display.wy = 0x0;
	m_Context.display.wx = 0x0;

	if (checksum_result == 0x0)
	{
		m_Context.cpu->SetFlag(CpuFlag::Carry, false);
		m_Context.cpu->SetFlag(CpuFlag::HalfCarry, false);
	}
	else
	{
		m_Context.cpu->SetFlag(CpuFlag::Carry, true);
		m_Context.cpu->SetFlag(CpuFlag::HalfCarry, true);
	}

	m_Running = true;
	return true;
}

void Emulator::Tick()
{
	// Fetch
	uint16_t current_pc = m_Context.cpu->ProgramCounter;

	const uint8_t opcode = ReadFromBus(&m_Context, m_Context.cpu->ProgramCounter);
	m_CurrentOpCode = opcode;

	std::string cpu_details = m_Context.cpu->Details();

	static int count = 0;
	count++;

	std::string opcode_format = std::format("Opcode: 0x{:x} - {} - B:${:x} C:${:x} D:${:x} E:${:x} H:${:x} L:${:x} A:${:x} F:{}{}{}{}",
											opcode,
											count,
											m_Context.cpu->GetRegister(RegisterType8::REG_B),
											m_Context.cpu->GetRegister(RegisterType8::REG_C),
											m_Context.cpu->GetRegister(RegisterType8::REG_D),
											m_Context.cpu->GetRegister(RegisterType8::REG_E),
											m_Context.cpu->GetRegister(RegisterType8::REG_H),
											m_Context.cpu->GetRegister(RegisterType8::REG_L),
											m_Context.cpu->GetRegister(RegisterType8::REG_A),
											m_Context.cpu->GetFlag(CpuFlag::Zero) ? "Z" : "-",
											m_Context.cpu->GetFlag(CpuFlag::Subtraction) ? "N" : "-",
											m_Context.cpu->GetFlag(CpuFlag::HalfCarry) ? "H" : "-",
											m_Context.cpu->GetFlag(CpuFlag::Carry) ? "C" : "-");

	std::cout << opcode_format << '\n';

	// Execute
	std::string opcode_name = Execute(opcode);

	// Tick timer
	for (int i = 0; i < m_Context.cycles; ++i)
	{
		m_Context.ticks++;

		uint16_t prev_div = m_Context.timer.div;

		m_Context.timer.div++;

		bool timer_update = false;

		switch (m_Context.timer.tac & (0b11))
		{
			case 0b00:
				timer_update = (prev_div & (1 << 9)) && (!(m_Context.timer.div & (1 << 9)));
				break;
			case 0b01:
				timer_update = (prev_div & (1 << 3)) && (!(m_Context.timer.div & (1 << 3)));
				break;
			case 0b10:
				timer_update = (prev_div & (1 << 5)) && (!(m_Context.timer.div & (1 << 5)));
				break;
			case 0b11:
				timer_update = (prev_div & (1 << 7)) && (!(m_Context.timer.div & (1 << 7)));
				break;
		}

		if (timer_update && m_Context.timer.tac & (1 << 2))
		{
			m_Context.timer.tima++;

			if (m_Context.timer.tima == 0xFF)
			{
				m_Context.timer.tima = m_Context.timer.tma;
				m_Context.cpu->RequestInterrupt(InterruptFlag::Timer);
			}
		}
	}

	m_Context.cycles = 0;

	// Display CPU details

	/*std::cout << std::hex << m_Context.ticks << ": - " << "0x" << std::hex << current_pc << ": " << " - 0x" << std::hex << (int)opcode << " - ";
	std::cout << std::setw(30) << std::left << opcode_name << std::right << std::right << cpu_details << '\n';*/

	// Debug
	{
		uint8_t data = ReadFromBus(&m_Context, 0xFF02);
		if (data == 0x81)
		{
			uint8_t c = ReadFromBus(&m_Context, 0xFF01);

			m_DebugMessage += static_cast<char>(c);
			WriteToBus(&m_Context, 0xFF02, 0);
		}

		if (!m_DebugMessage.empty())
		{
			std::cout << "DEBUG: " << m_DebugMessage << '\n';
		}
	}
}

uint8_t Emulator::GetOpCode() const
{
	return m_CurrentOpCode;
}

std::string Emulator::Execute(const uint8_t opcode)
{
	switch (opcode)
	{
		case 0x0:
			return Op::Nop(&m_Context);
		case 0x01:
			return Op::LoadN16(&m_Context, RegisterType16::REG_BC);
		//case 0x02:
		//	return Op::StoreR8(&m_Context, RegisterType8::REG_A, RegisterType16::REG_BC);
		case 0x03:
			return Op::IncR16(&m_Context, RegisterType16::REG_BC);
		//case 0x04:
		//	return Op::IncR8(&m_Context, RegisterType8::REG_B);
		//case 0x05:
		//	return Op::DecR8(&m_Context, RegisterType8::REG_B);
		//case 0x06:
		//	return Op::LoadN8(&m_Context, RegisterType8::REG_B);
		//case 0x09:
		//	return Op::AddR16(&m_Context, RegisterType16::REG_BC);
		//case 0x0A:
		//	return Op::LoadIndirectR16(&m_Context, RegisterType8::REG_A, RegisterType16::REG_BC);
		//case 0x0B:
		//	return Op::DecR16(&m_Context, RegisterType16::REG_BC);
		//case 0x0C:
		//	return Op::IncR8(&m_Context, RegisterType8::REG_C);
		case 0x0D:
			return Op::DecR8(&m_Context, RegisterType8::REG_C);
		case 0x0E:
			return Op::LoadN8(&m_Context, RegisterType8::REG_C);
		//case 0x10:
		//	return Op::Stop(&m_Context);
		case 0x11:
			return Op::LoadN16(&m_Context, RegisterType16::REG_DE);
		case 0x12:
			return Op::StoreR8(&m_Context, RegisterType8::REG_A, RegisterType16::REG_DE);
		//case 0x13:
		//	return Op::IncR16(&m_Context, RegisterType16::REG_DE);
		case 0x14:
			return Op::IncR8(&m_Context, RegisterType8::REG_D);
		//case 0x15:
		//	return Op::DecR8(&m_Context, RegisterType8::REG_D);
		//case 0x16:
		//	return Op::LoadN8(&m_Context, RegisterType8::REG_D);
		case 0x18:
			return Op::JumpRelativeN8(&m_Context);
		//case 0x19:
		//	return Op::AddR16(&m_Context, RegisterType16::REG_DE);
		//case 0x1A:
		//	return Op::LoadIndirectR16(&m_Context, RegisterType8::REG_A, RegisterType16::REG_DE);
		//case 0x1B:
		//	return Op::DecR16(&m_Context, RegisterType16::REG_DE);
		case 0x1C:
			return Op::IncR8(&m_Context, RegisterType8::REG_E);
		//case 0x1D:
		//	return Op::DecR8(&m_Context, RegisterType8::REG_E);
		//case 0x1E:
		//	return Op::LoadN8(&m_Context, RegisterType8::REG_E);
		case 0x20:
			return Op::JumpRelativeFlagNotSet(&m_Context, CpuFlag::Zero);
		case 0x21:
			return Op::LoadN16(&m_Context, RegisterType16::REG_HL);
		//case 0x22:
		//	return Op::StoreIncrementHL(&m_Context);
		//case 0x24:
		//	return Op::IncR8(&m_Context, RegisterType8::REG_H);
		case 0x23:
			return Op::IncR16(&m_Context, RegisterType16::REG_HL);
		//case 0x25:
		//	return Op::DecR8(&m_Context, RegisterType8::REG_H);
		//case 0x26:
		//	return Op::LoadN8(&m_Context, RegisterType8::REG_H);
		case 0x28:
			return Op::JumpRelativeFlagSet(&m_Context, CpuFlag::Zero);
		//case 0x29:
		//	return Op::AddR16(&m_Context, RegisterType16::REG_HL);
		case 0x2A:
			return Op::LoadIncrementHL(&m_Context);
		//case 0x2B:
		//	return Op::DecR16(&m_Context, RegisterType16::REG_HL);
		//case 0x2C:
		//	return Op::IncR8(&m_Context, RegisterType8::REG_L);
		//case 0x2D:
		//	return Op::DecR8(&m_Context, RegisterType8::REG_L);
		//case 0x2E:
		//	return Op::LoadN8(&m_Context, RegisterType8::REG_L);
		//case 0x30:
		//	return Op::JumpRelativeFlagN8(&m_Context, CpuFlag::Carry, false);
		//case 0x33:
		//	return Op::IncR16(&m_Context, RegisterType16::REG_SP);
		//case 0x38:
		//	return Op::JumpRelativeFlagN8(&m_Context, CpuFlag::Carry, true);
		case 0x3E:
			return Op::LoadN8(&m_Context, RegisterType8::REG_A);
		case 0x31:
			return Op::LoadN16(&m_Context, RegisterType16::REG_SP);
		//case 0x32:
		//	return Op::StoreDecrementHL(&m_Context);
		//case 0x36:
		//	return Op::StoreN8(&m_Context, RegisterType16::REG_HL);
		//case 0x39:
		//	return Op::AddR16(&m_Context, RegisterType16::REG_SP);
		//case 0x3A:
		//	return Op::LoadDecrementHL(&m_Context);
		//case 0x3B:
		//	return Op::DecR16(&m_Context, RegisterType16::REG_SP);
		//case 0x3C:
		//	return Op::IncR8(&m_Context, RegisterType8::REG_A);
		//case 0x3D:
		//	return Op::DecR8(&m_Context, RegisterType8::REG_A);
		//case 0x40:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_B, RegisterType8::REG_B);
		//case 0x41:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_B, RegisterType8::REG_C);
		//case 0x42:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_B, RegisterType8::REG_D);
		//case 0x43:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_B, RegisterType8::REG_E);
		//case 0x44:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_B, RegisterType8::REG_H);
		//case 0x45:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_B, RegisterType8::REG_L);
		//case 0x46:
		//	return Op::LoadIndirectR16(&m_Context, RegisterType8::REG_B, RegisterType16::REG_HL);
		case 0x47:
			return Op::LoadR8(&m_Context, RegisterType8::REG_B, RegisterType8::REG_A);
		//case 0x48:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_C, RegisterType8::REG_B);
		//case 0x49:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_C, RegisterType8::REG_C);
		//case 0x4A:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_C, RegisterType8::REG_D);
		//case 0x4B:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_C, RegisterType8::REG_E);
		//case 0x4C:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_C, RegisterType8::REG_H);
		//case 0x4D:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_C, RegisterType8::REG_L);
		//case 0x4E:
		//	return Op::LoadIndirectR16(&m_Context, RegisterType8::REG_C, RegisterType16::REG_HL);
		//case 0x4F:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_C, RegisterType8::REG_A);
		//case 0x50:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_D, RegisterType8::REG_B);
		//case 0x51:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_D, RegisterType8::REG_C);
		//case 0x52:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_D, RegisterType8::REG_D);
		//case 0x53:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_D, RegisterType8::REG_E);
		//case 0x54:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_D, RegisterType8::REG_H);
		//case 0x55:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_D, RegisterType8::REG_L);
		//case 0x56:
		//	return Op::LoadIndirectR16(&m_Context, RegisterType8::REG_D, RegisterType16::REG_HL);
		//case 0x57:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_D, RegisterType8::REG_A);
		//case 0x58:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_E, RegisterType8::REG_B);
		//case 0x59:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_E, RegisterType8::REG_C);
		//case 0x5A:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_E, RegisterType8::REG_D);
		//case 0x5B:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_E, RegisterType8::REG_E);
		//case 0x5C:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_E, RegisterType8::REG_H);
		//case 0x5D:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_E, RegisterType8::REG_L);
		//case 0x5E:
		//	return Op::LoadIndirectR16(&m_Context, RegisterType8::REG_E, RegisterType16::REG_HL);
		//case 0x5F:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_E, RegisterType8::REG_A);
		//case 0x60:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_H, RegisterType8::REG_B);
		//case 0x61:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_H, RegisterType8::REG_C);
		//case 0x62:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_H, RegisterType8::REG_D);
		//case 0x63:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_H, RegisterType8::REG_E);
		//case 0x64:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_H, RegisterType8::REG_H);
		//case 0x65:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_H, RegisterType8::REG_L);
		//case 0x66:
		//	return Op::LoadIndirectR16(&m_Context, RegisterType8::REG_H, RegisterType16::REG_HL);
		//case 0x67:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_H, RegisterType8::REG_A);
		//case 0x68:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_L, RegisterType8::REG_B);
		//case 0x69:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_L, RegisterType8::REG_C);
		//case 0x6A:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_L, RegisterType8::REG_D);
		//case 0x6B:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_L, RegisterType8::REG_E);
		//case 0x6C:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_L, RegisterType8::REG_H);
		//case 0x6D:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_L, RegisterType8::REG_L);
		//case 0x6E:
		//	return Op::LoadIndirectR16(&m_Context, RegisterType8::REG_L, RegisterType16::REG_HL);
		//case 0x6F:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_L, RegisterType8::REG_A);
		//case 0x70:
		//	return Op::StoreR8(&m_Context, RegisterType8::REG_B, RegisterType16::REG_HL);
		//case 0x71:
		//	return Op::StoreR8(&m_Context, RegisterType8::REG_C, RegisterType16::REG_HL);
		//case 0x72:
		//	return Op::StoreR8(&m_Context, RegisterType8::REG_D, RegisterType16::REG_HL);
		//case 0x73:
		//	return Op::StoreR8(&m_Context, RegisterType8::REG_E, RegisterType16::REG_HL);
		//case 0x74:
		//	return Op::StoreR8(&m_Context, RegisterType8::REG_H, RegisterType16::REG_HL);
		//case 0x75:
		//	return Op::StoreR8(&m_Context, RegisterType8::REG_L, RegisterType16::REG_HL);
		//case 0x77:
		//	return Op::StoreR8(&m_Context, RegisterType8::REG_A, RegisterType16::REG_HL);
		case 0x78:
			return Op::LoadR8(&m_Context, RegisterType8::REG_A, RegisterType8::REG_B);
		//case 0x79:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_A, RegisterType8::REG_C);
		//case 0x7A:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_A, RegisterType8::REG_D);
		//case 0x7B:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_A, RegisterType8::REG_E);
		case 0x7C:
			return Op::LoadR8(&m_Context, RegisterType8::REG_A, RegisterType8::REG_H);
		case 0x7D:
			return Op::LoadR8(&m_Context, RegisterType8::REG_A, RegisterType8::REG_L);
		//case 0x7E:
		//	return Op::LoadIndirectR16(&m_Context, RegisterType8::REG_A, RegisterType16::REG_HL);
		//case 0x7F:
		//	return Op::LoadR8(&m_Context, RegisterType8::REG_A, RegisterType8::REG_A);
		//case 0x80:
		//	return Op::AddR8(&m_Context, RegisterType8::REG_B);
		//case 0x81:
		//	return Op::AddR8(&m_Context, RegisterType8::REG_C);
		//case 0x82:
		//	return Op::AddR8(&m_Context, RegisterType8::REG_D);
		//case 0x83:
		//	return Op::AddR8(&m_Context, RegisterType8::REG_E);
		//case 0x84:
		//	return Op::AddR8(&m_Context, RegisterType8::REG_H);
		//case 0x85:
		//	return Op::AddR8(&m_Context, RegisterType8::REG_L);
		//case 0x86:
		//	return Op::AddIndirectHL(&m_Context);
		//case 0x87:
		//	return Op::AddR8(&m_Context, RegisterType8::REG_A);
		//case 0xA8:
		//	return Op::XorR8(&m_Context, RegisterType8::REG_B);
		//case 0xA9:
		//	return Op::XorR8(&m_Context, RegisterType8::REG_C);
		//case 0xAA:
		//	return Op::XorR8(&m_Context, RegisterType8::REG_D);
		//case 0xAB:
		//	return Op::XorR8(&m_Context, RegisterType8::REG_E);
		//case 0xAC:
		//	return Op::XorR8(&m_Context, RegisterType8::REG_H);
		//case 0xAD:
		//	return Op::XorR8(&m_Context, RegisterType8::REG_L);
		//case 0xAE:
		//	return Op::XorR16(&m_Context, RegisterType16::REG_HL);
		//case 0xAF:
		//	return Op::XorR8(&m_Context, RegisterType8::REG_A);
		case 0xB1:
			return Op::OrR8(&m_Context, RegisterType8::REG_C);
		//case 0xB8:
		//	return Op::CompareR8(&m_Context, RegisterType8::REG_B);
		//case 0xB9:
		//	return Op::CompareR8(&m_Context, RegisterType8::REG_C);
		//case 0xBA:
		//	return Op::CompareR8(&m_Context, RegisterType8::REG_D);
		//case 0xBB:
		//	return Op::CompareR8(&m_Context, RegisterType8::REG_E);
		//case 0xBC:
		//	return Op::CompareR8(&m_Context, RegisterType8::REG_H);
		//case 0xBD:
		//	return Op::CompareR8(&m_Context, RegisterType8::REG_L);
		//case 0xBE:
		//	return Op::CompareIndirectHL(&m_Context);
		//case 0xBF:
		//	return Op::CompareR8(&m_Context, RegisterType8::REG_A);
		//case 0xC0:
		//	return Op::ReturnCondition(&m_Context, CpuFlag::Zero, false);
		case 0xC1:
			return Op::PopR16(&m_Context, RegisterType16::REG_BC);
		//case 0xC2:
		//	return Op::JumpFlagN16(&m_Context, CpuFlag::Zero, false);
		case 0xC3:
			return Op::JumpN16(&m_Context);
		case 0xC4:
			return Op::CallN16FlagNotSet(&m_Context, CpuFlag::Zero);
		case 0xC5:
			return Op::PushR16(&m_Context, RegisterType16::REG_BC);
		//case 0xC6:
		//	return Op::AddN8(&m_Context);
		//case 0xC8:
		//	return Op::ReturnCondition(&m_Context, CpuFlag::Zero, true);
		case 0xC9:
			return Op::Return(&m_Context);
		//case 0xCA:
		//	return Op::JumpFlagN16(&m_Context, CpuFlag::Zero, true);
		//case 0xCC:
		//	return Op::CallN16Condition(&m_Context, CpuFlag::Zero, true);
		case 0xCD:
			return Op::CallN16(&m_Context);
		//case 0xD0:
		//	return Op::ReturnCondition(&m_Context, CpuFlag::Carry, false);
		//case 0xD1:
		//	return Op::PopR16(&m_Context, RegisterType16::REG_DE);
		//case 0xD2:
		//	return Op::JumpFlagN16(&m_Context, CpuFlag::Carry, false);
		//case 0xD4:
		//	return Op::CallN16Condition(&m_Context, CpuFlag::Carry, false);
		//case 0xD5:
		//	return Op::PushR16(&m_Context, RegisterType16::REG_DE);
		//case 0xD8:
		//	return Op::ReturnCondition(&m_Context, CpuFlag::Carry, true);
		//case 0xDA:
		//	return Op::JumpFlagN16(&m_Context, CpuFlag::Carry, true);
		//case 0xDC:
		//	return Op::CallN16Condition(&m_Context, CpuFlag::Carry, true);
		case 0xE0:
			return Op::StoreFF00(&m_Context);
		case 0xE1:
			return Op::PopR16(&m_Context, RegisterType16::REG_HL);
		case 0xEA:
			return Op::StoreIndirectR8(&m_Context, RegisterType8::REG_A);
		//case 0xE2:
		//	return Op::StoreIndirectAC(&m_Context);
		case 0xE5:
			return Op::PushR16(&m_Context, RegisterType16::REG_HL);
		case 0xE6:
			return Op::AndN8(&m_Context);
		//case 0xE8:
		//	return Op::AddSP(&m_Context);
		//case 0xE9:
		//	return Op::JumpHL(&m_Context);
		//case 0xEE:
		//	return Op::XorN8(&m_Context);
		case 0xF0:
			return Op::LoadFF00(&m_Context);
		case 0xF1:
			return Op::PopR16(&m_Context, RegisterType16::REG_AF);
		//case 0xF2:
		//	return Op::LoadIndirectAC(&m_Context);
		case 0xF3:
			return Op::DisableInterrupts(&m_Context);
		case 0xF5:
			return Op::PushR16(&m_Context, RegisterType16::REG_AF);
		case 0xFA:
			return Op::LoadIndirectR8(&m_Context, RegisterType8::REG_A);
		//case 0xFB:
		//	return Op::EnableInterrupts(&m_Context);
		case 0xFE:
			return Op::CompareN8(&m_Context);
		default:
			throw std::exception(std::format("Instruction not implemented: 0x{:x}", opcode).c_str());
	}
}