#include "Pch.h"
#include "Emulator.h"
#include "Cpu.h"

#include <iostream>
#include <vector>
#include <thread>

#include "Cpu.h"
#include "Ppu.h"
#include "Ram.h"
#include "Dma.h"
#include "Timer.h"
#include "Cartridge.h"
#include "Joypad.h"
#include "Display.h"
using namespace std::chrono_literals;

Emulator* Emulator::Instance = nullptr;

Emulator::Emulator()
{
	Instance = this;

	m_Cartridge = std::make_unique<Cartridge>();
	m_Cpu = std::make_unique<Cpu>(m_Cartridge.get());
	m_Timer = std::make_unique<Timer>();
	m_Ram = std::make_unique<Ram>();
	m_Display = std::make_unique<Display>();
	m_Ppu = std::make_unique<Ppu>();
	m_Dma = std::make_unique<Dma>();
	m_Joypad = std::make_unique<Joypad>();

	m_Context.cpu = m_Cpu.get();
	m_Context.bus = this;
}

Emulator::~Emulator()
{
	m_TraceLog.close();
}

bool Emulator::LoadRom(const std::string& path)
{
	if (!m_Cartridge->Load(const_cast<char*>(path.c_str())))
	{
		std::cerr << "Unable to load cartidge\n";
		return false;
	}

	m_Cpu->Init();
	m_Timer->Init();
	m_Ppu->Init();

	m_Running = true;
	return true;
}

bool Emulator::LoadRom(const std::vector<uint8_t>& filedata)
{
	m_Cartridge->Load(filedata);
	m_Cpu->Init();
	m_Timer->Init();
	m_Ppu->Init();

	m_Running = true;
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

int Emulator::GetFPS()
{
	return m_Ppu->GetFPS();
}

void Emulator::Pause(bool pause)
{
	m_Paused = pause;
}

void Emulator::ToggleTraceLog(bool enable)
{
	if (enable)
	{
		m_EnableTraceLog = true;
		m_TraceLog.open("retrogbm-tracelog.txt");
	}
	else
	{
		m_EnableTraceLog = false;
		m_TraceLog.close();
	}
}

void Emulator::Tick()
{
	std::lock_guard<std::mutex> lock(m_EmulatorMutex);

	if (m_Paused)
	{
		std::this_thread::sleep_for(100ms);
		return;
	}

	// Fetch
	uint16_t current_pc = m_Cpu->ProgramCounter;

	const uint8_t opcode = this->ReadBus(m_Cpu->ProgramCounter);
	m_CurrentOpCode = opcode;

	// Execute
	if (!m_Halted)
	{
		if (IsTraceLogEnabled())
		{
			/*std::string debug_format = std::format("OP:{:X},PC:{:X},AF:{:X},BC:{:X},DE:{:X},HL:{:X},SP:{:X}",
												   opcode,
												   m_Context.cpu->ProgramCounter,
												   m_Context.cpu->GetRegister(RegisterType16::REG_AF),
												   m_Context.cpu->GetRegister(RegisterType16::REG_BC),
												   m_Context.cpu->GetRegister(RegisterType16::REG_DE),
												   m_Context.cpu->GetRegister(RegisterType16::REG_HL),
												   m_Context.cpu->StackPointer);

			m_TraceLog << debug_format << std::endl;*/
		}

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

	// Check flag
	m_Cpu->HandleInterrupts();

	// Do deferred machine cycles
	Cycle(m_Context.cycles);
	m_Context.cycles = 0;

	// Debug
	//{
	//	uint8_t data = this->ReadBus(0xFF02);
	//	if (data == 0x81)
	//	{
	//		uint8_t c = this->ReadBus(0xFF01);

	//		m_DebugMessage += static_cast<char>(c);
	//		this->WriteBus(0xFF02, 0);
	//	}

	//	if (!m_DebugMessage.empty())
	//	{
	//		// std::cout << "\tDEBUG: " << m_DebugMessage << '\n';
	//	}
	//}
}

void Emulator::Cycle(int machine_cycles)
{
	for (int i = 0; i < m_Context.cycles; ++i)
	{
		for (int n = 0; n < 4; ++n)
		{
			m_Timer->Tick();
			m_Ppu->Tick();
		}

		m_Dma->Tick();
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
	else if (address == 0xFF01)
	{
		return m_SerialData[0];
	}
	else if (address == 0xFF02)
	{
		return m_SerialData[1];
	}
	else if (((address >= 0xFF04) && (address <= 0xFF07)))
	{
		return m_Timer->Read(address);
	}
	else if (address == 0xFF0F)
	{
		return m_Cpu->GetInterruptFlags();
	}
	else if (((address >= 0xFF10) && (address <= 0xFF3F)))
	{
		//ignore sound
		return 0;
	}
	else if (((address >= 0xFF40) && (address <= 0xFF4B)))
	{
		return m_Display->Read(address);
	}
	else if (address == 0xFF4F)
	{
		return m_Ppu->GetVideoRamBank();
	}
	else if (address == 0xFF55)
	{
		return m_Dma->GetLengthModeStart();
	}
	else if (address == 0xFF6C)
	{
		return m_Display->GetObjectPriorityMode();
	}
	else if (address == 0xFF70)
	{
		return m_Ram->GetWorkRamBank();
	}

	std::cout << "Unsupported ReadIO 0x" << std::hex << address << '\n';
	return 0xFF;
}

void Emulator::WriteIO(uint16_t address, uint8_t value)
{
	if (address == 0xFF00)
	{
		m_Joypad->Write(value);
		return;
	}
	else if (address == 0xFF01)
	{
		m_SerialData[0] = value;
		return;
	}
	else if (address == 0xFF02)
	{
		m_SerialData[1] = value;
		return;
	}
	else if (((address >= 0xFF04) && (address <= 0xFF07)))
	{
		m_Timer->Write(address, value);
		return;
	}
	else if (address == 0xFF0F)
	{
		m_Cpu->SetInterrupt(value);
		return;
	}
	else if (((address >= 0xFF10) && (address <= 0xFF3F)))
	{
		// Ignore sound
		return;
	}
	else if (((address >= 0xFF40) && (address <= 0xFF4B)))
	{
		m_Display->Write(address, value);
		return;
	}
	else if (address == 0xFF4F)
	{
		m_Ppu->SetVideoRamBank(value);
		return;
	}
	else if (address == 0xFF51 || address == 0xFF52)
	{
		m_Dma->SetSource(address, value);
		return;
	}
	else if (address == 0xFF53 || address == 0xFF54)
	{
		m_Dma->SetDestination(address, value);
		return;
	}
	else if (address == 0xFF55)
	{
		m_Dma->StartCGB(value);
		return;
	}
	else if (address >= 0xFF68 && address <= 0xFF6B)
	{
		m_Display->Write(address, value);
		return;
	}
	else if (address == 0xFF6C)
	{
		m_Display->SetObjectPriorityMode(value);
		return;
	}
	else if (address == 0xFF70)
	{
		m_Ram->SetWorkRamBank(value);
		return;
	}

	std::cout << "Unsupported WriteIO 0x" << std::hex << address << '\n';
}

uint8_t Emulator::ReadBus(uint16_t address)
{
	if (address >= 0x0000 && address <= 0x7FFF)
	{
		// ROM Data
		return m_Cartridge->Read(address);
	}
	else if (address >= 0x8000 && address <= 0x9FFF)
	{
		// VRAM (Video RAM)
		return m_Ppu->ReadVideoRam(address);
	}
	else if (address >= 0xA000 && address <= 0xBFFF)
	{
		// Cartridge RAM
		return m_Cartridge->Read(address);
	}
	else if (address >= 0xC000 && address <= 0xDFFF)
	{
		// WRAM (Work RAM)
		return m_Ram->ReadWorkRam(address);
	}
	else if (address >= 0xE000 && address <= 0xFDFF)
	{
		// Reserved echo RAM
		return 0;
	}
	else if (address >= 0xFE00 && address <= 0xFE9F)
	{
		// OAM
		if (m_Dma->IsTransferring())
		{
			return 0xFF;
		}

		return m_Ppu->ReadOam(address);
	}
	else if (address >= 0xFEA0 && address <= 0xFEFF)
	{
		// Reserved unusable
		return 0;
	}
	else if (address >= 0xFF00 && address <= 0xFF7F)
	{
		// IO Registers
		return this->ReadIO(address);
	}
	else if (address >= 0xFF80 && address <= 0xFFFE)
	{
		// HRAM (High RAM)
		return m_Ram->ReadHighRam(address);
	}
	else if (address == 0xFFFF)
	{
		// CPU interrupts
		return m_Cpu->GetInterruptEnable();
	}

	std::cout << "Unsupported ReadBus: 0x{:x}" << address << '\n';
	return 0xFF;
}

void Emulator::WriteBus(uint16_t address, uint8_t value)
{
	if (address >= 0x0000 && address <= 0x7FFF)
	{
		// ROM Data
		m_Cartridge->Write(address, value);
		return;
	}
	else if (address >= 0x8000 && address <= 0x9FFF)
	{
		// VRAM (Video RAM)
		m_Ppu->WriteVideoRam(address, value);
		return;
	}
	else if (address >= 0xA000 && address <= 0xBFFF)
	{
		// Cartridge RAM
		m_Cartridge->Write(address, value);
		return;
	}
	else if (address >= 0xC000 && address <= 0xDFFF)
	{
		// WRAM (Work RAM)
		m_Ram->WriteWorkRam(address, value);
		return;
	}
	else if (address >= 0xE000 && address <= 0xFDFF)
	{
		// Reserved echo ram
	}
	else if (address >= 0xFE00 && address <= 0xFE9F)
	{
		// OAM
		if (m_Dma->IsTransferring())
		{
			return;
		}

		m_Ppu->WriteOam(address, value);
		return;
	}
	else if (address >= 0xFEA0 && address <= 0xFEFF)
	{
		// Unusable reserved
		return;
	}
	else if (address >= 0xFF00 && address <= 0xFF7F)
	{
		// IO Registers
		this->WriteIO(address, value);
		return;
	}
	else if (address >= 0xFF80 && address <= 0xFFFE)
	{
		m_Ram->WriteHighRam(address, value);
		return;
	}
	else if (address == 0xFFFF)
	{
		// CPU interrupts
		m_Cpu->SetInterruptEnable(value);
		return;
	}

	std::cout << "Unsupported WriteBus: 0x" << address << '\n';
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

void Emulator::SaveState()
{
	std::lock_guard<std::mutex> lock(m_EmulatorMutex);
	std::fstream file(m_Cartridge->GetCartridgeInfo()->title + ".state", std::ios::binary | std::ios::out);

	m_Cpu->SaveState(&file);
	m_Timer->SaveState(&file);
	m_Ram->SaveState(&file);
	m_Cartridge->SaveState(&file);

	m_Display->SaveState(&file);
	m_Ppu->SaveState(&file);
	m_Dma->SaveState(&file);
}

void Emulator::LoadState()
{
	std::lock_guard<std::mutex> lock(m_EmulatorMutex);
	std::fstream file(m_Cartridge->GetCartridgeInfo()->title + ".state", std::ios::binary | std::ios::in);

	m_Cpu->LoadState(&file);
	m_Timer->LoadState(&file);
	m_Ram->LoadState(&file);
	m_Cartridge->LoadState(&file);

	m_Display->LoadState(&file);
	m_Ppu->LoadState(&file);
	m_Dma->LoadState(&file);
}

void* Emulator::GetVideoBuffer()
{
	return m_Ppu->GetVideoBuffer();
}

int Emulator::GetVideoPitch()
{
	return sizeof(uint32_t) * m_Ppu->ScreenResolutionX;
}