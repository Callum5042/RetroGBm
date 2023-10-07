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
		case 0xC3:
			return Op::JumpN16(&m_Context);
		case 0xAF:
			return Op::XorR8(&m_Context, RegisterType8::REG_A);
		case 0x21:
			return Op::LoadN16(&m_Context, RegisterType16::REG_HL);
		default:
			throw std::exception(std::format("Instruction not implemented: 0x{:x}", opcode).c_str());
	}
}