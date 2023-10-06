#include "Emulator.h"
#include "Cpu.h"
#include <iostream>
#include <exception>
#include <string>

Emulator::Emulator()
{
	m_Cpu = std::make_unique<Cpu>();
}

bool Emulator::LoadRom(const std::filesystem::path& path)
{
	if (!LoadCartridge(path, &m_CartridgeInfo))
	{
		std::cerr << "Unable to load cartidge\n";
		return false;
	}

	// Print cartridge info
	std::cout << "Cartidge loaded\n";
	std::cout << "> Title: " << m_CartridgeInfo.title << '\n';
	std::cout << "> Cartridge Type: " << static_cast<int>(m_CartridgeInfo.header.cartridge_type) << '\n';
	std::cout << "> ROM size: " << m_CartridgeInfo.header.rom_size << '\n';
	std::cout << "> ROM banks: " << m_CartridgeInfo.header.rom_banks << '\n';
	std::cout << "> RAM size: " << m_CartridgeInfo.header.ram_size << '\n';
	std::cout << "> License: " << m_CartridgeInfo.header.license << '\n';
	std::cout << "> Version: " << m_CartridgeInfo.header.version << '\n';
	std::cout << "> Checksum: " << (CartridgeChecksum(m_CartridgeInfo) ? "Passed" : "Failed") << '\n' << '\n';

	// Set program counter to 0x100 to skip boot rom
	m_Cpu->ProgramCounter = 0x100;

	m_Running = true;
	return true;
}

void Emulator::Run()
{
	while (m_Running)
	{
		// Fetch
		// const uint16_t pc = m_Cpu->ProgramCounter;
		std::cout << "0x" << std::hex << m_Cpu->ProgramCounter << ": ";

		const uint8_t opcode = ReadFromBus();
		std::string opcode_name = "NOT SET";

		// Execute instruction
		if (opcode == 0x0)
		{
			// NOP
			opcode_name = "NOP (0x0)";
		}
		else if (opcode == 0xC3)
		{
			// JP a16
			uint8_t low = ReadFromBus();
			uint8_t high = ReadFromBus();
			uint16_t data = low | (high << 8);

			m_Cpu->ProgramCounter = data;
			opcode_name = std::format("JP a16 (0xC3 0x{:x} 0x{:x})", low, high);
		}
		else if (opcode == 0xAF)
		{
			// XOR A, A
			uint8_t reg = m_Cpu->GetRegister(RegisterType8::REG_A);
			m_Cpu->SetRegister(RegisterType8::REG_A, reg ^ reg);
			m_Cpu->SetFlag(CpuFlag::Zero, true);

			opcode_name = "XOR (0xAF)";
		}
		else if (opcode == 0x21)
		{
			// LD HL, n16
			uint8_t low = ReadFromBus();
			uint8_t high = ReadFromBus();
			m_Cpu->SetRegister(RegisterType16::REG_HL, high, low);

			opcode_name = std::format("LD HL, n16 (0xC3 0x{:x} 0x{:x})", low, high);
		}
		else
		{
			throw std::exception(std::format("Instruction not implemented: 0x{:x}", opcode).c_str());
		}

		// Display CPU details
		std::cout << opcode_name << " - " << m_Cpu->Details() << '\n';
	}
}

const uint8_t Emulator::ReadFromBus()
{
	// TODO: Refactor this into a bus
	if (m_Cpu->ProgramCounter < 0x8000)
	{
		// Read from ROM
		return m_CartridgeInfo.data[m_Cpu->ProgramCounter++];
	}

	throw std::exception("Not implemented");
}