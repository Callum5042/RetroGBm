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

	// Print cartridge info
	std::cout << "Cartidge loaded\n";
	std::cout << "> Title: " << m_Context.cartridge->title << '\n';
	std::cout << "> Cartridge Type: " << static_cast<int>(m_Context.cartridge->header.cartridge_type) << '\n';
	std::cout << "> ROM size: " << m_Context.cartridge->header.rom_size << '\n';
	std::cout << "> ROM banks: " << m_Context.cartridge->header.rom_banks << '\n';
	std::cout << "> RAM size: " << m_Context.cartridge->header.ram_size << '\n';
	std::cout << "> License: " << m_Context.cartridge->header.license << '\n';
	std::cout << "> Version: " << m_Context.cartridge->header.version << '\n';
	std::cout << "> Checksum: " << (CartridgeChecksum(m_Context.cartridge.get()) ? "Passed" : "Failed") << '\n' << '\n';

	// Set program counter to 0x100 to skip boot rom
	m_Context.cpu->ProgramCounter = 0x100;

	m_Running = true;
	return true;
}

void Emulator::Run()
{
	while (m_Running)
	{
		const int CYCLES_PER_SCANLINE = 80;
		if (m_Context.cycles >= CYCLES_PER_SCANLINE)
		{
			m_Context.cycles = 0;
		}
		else
		{
			// Fetch
			std::cout << "0x" << std::hex << m_Context.cpu->ProgramCounter << ": ";
			const uint8_t opcode = ReadFromBus(m_Context.cartridge.get(), m_Context.cpu->ProgramCounter++);

			// Execute
			std::string opcode_name = Execute(opcode);

			// Display CPU details
			std::cout << opcode_name << " - " << m_Context.cpu->Details() << '\n';
		}
	}
}

std::string Emulator::Execute(const uint8_t opcode)
{
	switch (opcode)
	{
		case 0x0:
			return Op::Nop(&m_Context);
		case 0x01:
			return Op::LoadN16(&m_Context, RegisterType16::REG_BC);
		case 0x06:
			return Op::LoadN8(&m_Context, RegisterType8::REG_B);
		case 0x0E:
			return Op::LoadN8(&m_Context, RegisterType8::REG_C);
		case 0x11:
			return Op::LoadN16(&m_Context, RegisterType16::REG_DE);
		case 0x16:
			return Op::LoadN8(&m_Context, RegisterType8::REG_D);
		case 0x1E:
			return Op::LoadN8(&m_Context, RegisterType8::REG_E);
		case 0x26:
			return Op::LoadN8(&m_Context, RegisterType8::REG_H);
		case 0x2E:
			return Op::LoadN8(&m_Context, RegisterType8::REG_L);
		case 0x3E:
			return Op::LoadN8(&m_Context, RegisterType8::REG_A);
		case 0xC3:
			return Op::JumpN16(&m_Context);
		case 0x21:
			return Op::LoadN16(&m_Context, RegisterType16::REG_HL);
		case 0x22:
			return Op::LoadIncrementHL(&m_Context);
		case 0x31:
			return Op::LoadN16(&m_Context, RegisterType16::REG_SP);
		case 0x32:
			return Op::LoadDecrementHL(&m_Context);
		case 0x40:
			return Op::LoadR8(&m_Context, RegisterType8::REG_B, RegisterType8::REG_B);
		case 0x41:
			return Op::LoadR8(&m_Context, RegisterType8::REG_B, RegisterType8::REG_C);
		case 0x42:
			return Op::LoadR8(&m_Context, RegisterType8::REG_B, RegisterType8::REG_D);
		case 0x43:
			return Op::LoadR8(&m_Context, RegisterType8::REG_B, RegisterType8::REG_E);
		case 0x44:
			return Op::LoadR8(&m_Context, RegisterType8::REG_B, RegisterType8::REG_H);
		case 0x45:
			return Op::LoadR8(&m_Context, RegisterType8::REG_B, RegisterType8::REG_L);
		case 0x47:
			return Op::LoadR8(&m_Context, RegisterType8::REG_B, RegisterType8::REG_A);
		case 0x48:
			return Op::LoadR8(&m_Context, RegisterType8::REG_C, RegisterType8::REG_B);
		case 0x49:
			return Op::LoadR8(&m_Context, RegisterType8::REG_C, RegisterType8::REG_C);
		case 0x4A:
			return Op::LoadR8(&m_Context, RegisterType8::REG_C, RegisterType8::REG_D);
		case 0x4B:
			return Op::LoadR8(&m_Context, RegisterType8::REG_C, RegisterType8::REG_E);
		case 0x4C:
			return Op::LoadR8(&m_Context, RegisterType8::REG_C, RegisterType8::REG_H);
		case 0x4D:
			return Op::LoadR8(&m_Context, RegisterType8::REG_C, RegisterType8::REG_L);
		case 0x4F:
			return Op::LoadR8(&m_Context, RegisterType8::REG_C, RegisterType8::REG_A);
		case 0x50:
			return Op::LoadR8(&m_Context, RegisterType8::REG_D, RegisterType8::REG_B);
		case 0x51:
			return Op::LoadR8(&m_Context, RegisterType8::REG_D, RegisterType8::REG_C);
		case 0x52:
			return Op::LoadR8(&m_Context, RegisterType8::REG_D, RegisterType8::REG_D);
		case 0x53:
			return Op::LoadR8(&m_Context, RegisterType8::REG_D, RegisterType8::REG_E);
		case 0x54:
			return Op::LoadR8(&m_Context, RegisterType8::REG_D, RegisterType8::REG_H);
		case 0x55:
			return Op::LoadR8(&m_Context, RegisterType8::REG_D, RegisterType8::REG_L);
		case 0x57:
			return Op::LoadR8(&m_Context, RegisterType8::REG_D, RegisterType8::REG_A);
		case 0x58:
			return Op::LoadR8(&m_Context, RegisterType8::REG_E, RegisterType8::REG_B);
		case 0x59:
			return Op::LoadR8(&m_Context, RegisterType8::REG_E, RegisterType8::REG_C);
		case 0x5A:
			return Op::LoadR8(&m_Context, RegisterType8::REG_E, RegisterType8::REG_D);
		case 0x5B:
			return Op::LoadR8(&m_Context, RegisterType8::REG_E, RegisterType8::REG_E);
		case 0x5C:
			return Op::LoadR8(&m_Context, RegisterType8::REG_E, RegisterType8::REG_H);
		case 0x5D:
			return Op::LoadR8(&m_Context, RegisterType8::REG_E, RegisterType8::REG_L);
		case 0x5F:
			return Op::LoadR8(&m_Context, RegisterType8::REG_E, RegisterType8::REG_A);
		case 0x60:
			return Op::LoadR8(&m_Context, RegisterType8::REG_H, RegisterType8::REG_B);
		case 0x61:
			return Op::LoadR8(&m_Context, RegisterType8::REG_H, RegisterType8::REG_C);
		case 0x62:
			return Op::LoadR8(&m_Context, RegisterType8::REG_H, RegisterType8::REG_D);
		case 0x63:
			return Op::LoadR8(&m_Context, RegisterType8::REG_H, RegisterType8::REG_E);
		case 0x64:
			return Op::LoadR8(&m_Context, RegisterType8::REG_H, RegisterType8::REG_H);
		case 0x65:
			return Op::LoadR8(&m_Context, RegisterType8::REG_H, RegisterType8::REG_L);
		case 0x67:
			return Op::LoadR8(&m_Context, RegisterType8::REG_H, RegisterType8::REG_A);
		case 0x68:
			return Op::LoadR8(&m_Context, RegisterType8::REG_L, RegisterType8::REG_B);
		case 0x69:
			return Op::LoadR8(&m_Context, RegisterType8::REG_L, RegisterType8::REG_C);
		case 0x6A:
			return Op::LoadR8(&m_Context, RegisterType8::REG_L, RegisterType8::REG_D);
		case 0x6B:
			return Op::LoadR8(&m_Context, RegisterType8::REG_L, RegisterType8::REG_E);
		case 0x6C:
			return Op::LoadR8(&m_Context, RegisterType8::REG_L, RegisterType8::REG_H);
		case 0x6D:
			return Op::LoadR8(&m_Context, RegisterType8::REG_L, RegisterType8::REG_L);
		case 0x6F:
			return Op::LoadR8(&m_Context, RegisterType8::REG_L, RegisterType8::REG_A);
		case 0x78:
			return Op::LoadR8(&m_Context, RegisterType8::REG_A, RegisterType8::REG_B);
		case 0x79:
			return Op::LoadR8(&m_Context, RegisterType8::REG_A, RegisterType8::REG_C);
		case 0x7A:
			return Op::LoadR8(&m_Context, RegisterType8::REG_A, RegisterType8::REG_D);
		case 0x7B:
			return Op::LoadR8(&m_Context, RegisterType8::REG_A, RegisterType8::REG_E);
		case 0x7C:
			return Op::LoadR8(&m_Context, RegisterType8::REG_A, RegisterType8::REG_H);
		case 0x7D:
			return Op::LoadR8(&m_Context, RegisterType8::REG_A, RegisterType8::REG_L);
		case 0x7F:
			return Op::LoadR8(&m_Context, RegisterType8::REG_A, RegisterType8::REG_A);
		case 0xA8:
			return Op::XorR8(&m_Context, RegisterType8::REG_B);
		case 0xA9:
			return Op::XorR8(&m_Context, RegisterType8::REG_C);
		case 0xAA:
			return Op::XorR8(&m_Context, RegisterType8::REG_D);
		case 0xAB:
			return Op::XorR8(&m_Context, RegisterType8::REG_E);
		case 0xAC:
			return Op::XorR8(&m_Context, RegisterType8::REG_H);
		case 0xAD:
			return Op::XorR8(&m_Context, RegisterType8::REG_L);
		case 0xAE:
			return Op::XorR16(&m_Context, RegisterType16::REG_HL);
		case 0xAF:
			return Op::XorR8(&m_Context, RegisterType8::REG_A);
		case 0xEE:
			return Op::XorN8(&m_Context);
		default:
			throw std::exception(std::format("Instruction not implemented: 0x{:x}", opcode).c_str());
	}
}