#include "Emulator.h"
#include "Cpu.h"
#include "Bus.h"
#include "Instructions.h"

#include <iostream>
#include <exception>
#include <string>

Emulator* Emulator::Instance = nullptr;

Emulator::Emulator()
{
	Instance = this;

	m_Cpu = std::make_unique<Cpu>();
	m_Timer = std::make_unique<Timer>();
	m_Ram = std::make_unique<Ram>();
	m_Cartridge = std::make_unique<Cartridge>();
	m_Display = std::make_unique<Display>();
	m_Ppu = std::make_unique<Ppu>();
	m_Dma = std::make_unique<Dma>();
	m_Joypad = std::make_unique<Joypad>();

	m_DebugFile.open("debug.txt");
	m_Context.cpu = m_Cpu.get();
}

Emulator::~Emulator()
{
	m_DebugFile.close();
}

bool Emulator::LoadRom(const std::string& path)
{
	if (!m_Cartridge->Load(const_cast<char*>(path.c_str())))
	{
		std::cerr << "Unable to load cartidge\n";
		return false;
	}

	uint8_t checksum_result = 0x1;
	// bool checksum = CartridgeChecksum(m_Context.cartridge.get(), &checksum_result);

	// Set program counter to 0x100 to skip boot rom
	m_Cpu->ProgramCounter = 0x100;

	// Default flags based on the cartridge info
	if (checksum_result == 0x0)
	{
		m_Cpu->SetFlag(CpuFlag::Carry, false);
		m_Cpu->SetFlag(CpuFlag::HalfCarry, false);
	}
	else
	{
		m_Cpu->SetFlag(CpuFlag::Carry, true);
		m_Cpu->SetFlag(CpuFlag::HalfCarry, true);
	}


	m_Timer->Init();
	m_Ppu->Init();

	return true;
}

void Emulator::Stop()
{
	m_Running = false;
}

void Emulator::SetHalt(bool value)
{
	m_Halted = value;
}

void Emulator::Tick()
{
	// Fetch
	uint16_t current_pc = m_Cpu->ProgramCounter;

	const uint8_t opcode = this->ReadBus(m_Cpu->ProgramCounter);
	m_CurrentOpCode = opcode;

	/*std::string debug_format = std::format("OP:{:X} PC:{:X} AF:{:X} BC:{:X} DE:{:X} HL:{:X} SP:{:X}",
											opcode,
											m_Context.cpu->ProgramCounter,
											m_Context.cpu->GetRegister(RegisterType16::REG_AF),
											m_Context.cpu->GetRegister(RegisterType16::REG_BC),
											m_Context.cpu->GetRegister(RegisterType16::REG_DE),
											m_Context.cpu->GetRegister(RegisterType16::REG_HL),
											m_Context.cpu->StackPointer);*/

											// std::cout << debug_format << '\n';
											// m_DebugFile << debug_format << '\n';

	// Execute
	if (!m_Halted)
	{
		m_Cpu->Execute(&m_Context, opcode);
	}
	else
	{
		m_Context.cycles += 4;

		if (m_Cpu->GetInterruptFlags())
		{
			m_Halted = false;
		}
	}

	// Tick timer
	for (int i = 0; i < m_Context.cycles; ++i)
	{
		for (int n = 0; n < 4; ++n)
		{
			m_Timer->Tick();
			m_Ppu->Tick();
		}

		m_Dma->Tick();
	}

	m_Context.cycles = 0;

	// Check flag
	m_Cpu->HandleInterrupts();

	// Debug
	{
		uint8_t data = this->ReadBus(0xFF02);
		if (data == 0x81)
		{
			uint8_t c = this->ReadBus(0xFF01);

			m_DebugMessage += static_cast<char>(c);
			this->WriteBus(0xFF02, 0);
		}

		if (!m_DebugMessage.empty())
		{
			// std::cout << "\tDEBUG: " << m_DebugMessage << '\n';
		}
	}
}

uint8_t Emulator::GetOpCode() const
{
	return m_CurrentOpCode;
}

uint8_t Emulator::ReadIO(uint16_t address)
{
	if (address == 0xFF00)
	{
		return m_Joypad->GamepadGetOutput();
	}

	if (address == 0xFF01)
	{
		return m_SerialData[0];
	}

	if (address == 0xFF02)
	{
		return m_SerialData[1];
	}

	if (((address >= 0xFF04) && (address <= 0xFF07)))
	{
		return m_Timer->Read(address);
	}

	if (address == 0xFF0F)
	{
		return m_Cpu->GetInterruptFlags();
	}

	if (((address >= 0xFF10) && (address <= 0xFF3F)))
	{
		//ignore sound
		return 0;
	}

	if (((address >= 0xFF40) && (address <= 0xFF4B)))
	{
		return m_Display->Read(address);
	}

	std::cout << "Unsupported ReadBus 0x" << std::hex << address << '\n';
	return 0;
}

void Emulator::WriteIO(uint16_t address, uint8_t value)
{
	if (address == 0xFF00)
	{
		m_Joypad->Write(value);
		return;
	}

	if (address == 0xFF01)
	{
		m_SerialData[0] = value;
		return;
	}

	if (address == 0xFF02)
	{
		m_SerialData[1] = value;
		return;
	}

	if (((address >= 0xFF04) && (address <= 0xFF07)))
	{
		m_Timer->Write(address, value);
		return;
	}

	if (address == 0xFF0F)
	{
		m_Cpu->SetInterrupt(value);
		return;
	}

	if (((address >= 0xFF10) && (address <= 0xFF3F)))
	{
		// Ignore sound
		return;
	}

	if (((address >= 0xFF40) && (address <= 0xFF4B)))
	{
		m_Display->Write(address, value);
		return;
	}

	std::cout << "Unsupported WriteBus 0x" << std::hex << address << '\n';
}

uint8_t Emulator::ReadBus(uint16_t address)
{
	if (address < 0x8000)
	{
		// ROM Data
		return m_Cartridge->Read(address);
	}
	else if (address < 0xA000)
	{
		// Char/Map Data
		return m_Ppu->ReadVideoRam(address);
	}
	else if (address < 0xC000)
	{
		// Cartridge RAM
		return m_Cartridge->Read(address);
	}
	else if (address < 0xE000)
	{
		// WRAM (Working RAM)
		return m_Ram->ReadWorkRam(address);
	}
	else if (address < 0xFE00)
	{
		// Reserved echo ram
		return 0;
	}
	else if (address < 0xFEA0)
	{
		// OAM
		if (m_Dma->IsTransferring())
		{
			return 0xFF;
		}

		return m_Ppu->ReadOam(address);

		return 0;
	}
	else if (address < 0xFF00)
	{
		// Reserved unusable
		return 0;
	}
	else if (address < 0xFF80)
	{
		// IO Registers
		return this->ReadIO(address);
	}
	else if (address == 0xFFFF)
	{
		// CPU interrupts
		return m_Cpu->GetInterruptEnable();
	}

	return m_Ram->ReadHighRam(address);
}

void Emulator::WriteBus(uint16_t address, uint8_t value)
{
	if (address < 0x8000)
	{
		// ROM Data
		m_Cartridge->Write(address, value);
	}
	else if (address < 0xA000)
	{
		// Char/Map Data
		m_Ppu->WriteVideoRam(address, value);
	}
	else if (address < 0xC000)
	{
		// EXT-RAM
		m_Cartridge->Write(address, value);
	}
	else if (address < 0xE000)
	{
		// WRAM
		m_Ram->WriteWorkRam(address, value);
	}
	else if (address < 0xFE00)
	{
		// Reserved echo ram
	}
	else if (address < 0xFEA0)
	{
		// OAM
		if (m_Dma->IsTransferring())
		{
			return;
		}

		m_Ppu->WriteOam(address, value);
	}
	else if (address < 0xFF00)
	{
		// Unusable reserved
	}
	else if (address < 0xFF80)
	{
		// IO Registers
		return this->WriteIO(address, value);
	}
	else if (address == 0xFFFF)
	{
		// CPU interrupts
		m_Cpu->SetInterruptEnable(value);
	}
	else
	{
		m_Ram->WriteHighRam(address, value);
	}
}

uint16_t Emulator::ReadBus16(uint16_t address)
{
	uint16_t lo = this->ReadBus(address);
	uint16_t hi = this->ReadBus(address + 1);

	return lo | (hi << 8);
}

void Emulator::WriteBus16(uint16_t address, uint16_t value)
{
	this->WriteBus(address + 1, (value >> 8) & 0xFF);
	this->WriteBus(address, value & 0xFF);
}

void Emulator::StackPush(uint8_t data)
{
	m_Cpu->StackPointer--;
	WriteBus(m_Cpu->StackPointer, data);
}

void Emulator::StackPush16(uint16_t data)
{
	StackPush((data >> 8) & 0xFF);
	StackPush(data & 0xFF);
}

uint8_t Emulator::StackPop()
{
	return this->ReadBus(m_Cpu->StackPointer++);
}

uint16_t Emulator::StackPop16()
{
	uint16_t lo = this->StackPop();
	uint16_t hi = this->StackPop();

	return (hi << 8) | lo;
}