#include "RetroGBm/Pch.h"
#include "RetroGBm/Emulator.h"
#include "RetroGBm/Cpu.h"

#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <fstream>

#include "RetroGBm/Cpu.h"
#include "RetroGBm/Ram.h"
#include "RetroGBm/Dma.h"
#include "RetroGBm/Timer.h"
#include "RetroGBm/Joypad.h"
#include "RetroGBm/Display.h"
#include "RetroGBm/Ppu.h"
#include "RetroGBm/Apu.h"

#include "RetroGBm/Cartridge/BaseCartridge.h"

using namespace std::chrono_literals;

Emulator* Emulator::Instance = nullptr;

Emulator::Emulator()
{
	Instance = this;

	// TODO: Do I still need this here?
	m_Cpu = std::make_unique<Cpu>(m_Cartridge.get());
	m_Timer = std::make_unique<Timer>();
	m_Ram = std::make_unique<Ram>();
	m_Display = std::make_unique<Display>();
	m_Joypad = std::make_unique<Joypad>();

	m_Ppu = std::make_unique<Ppu>(this, m_Cpu.get(), m_Display.get(), m_Cartridge.get());
	m_Dma = std::make_unique<Dma>();
	m_Apu = std::make_unique<Apu>();

	m_Context.cpu = m_Cpu.get();
	m_Context.bus = this;
}

Emulator::Emulator(std::unique_ptr<BaseCartridge> cartridge)
{
	Instance = this;

	m_Cartridge = std::move(cartridge);
	m_Cpu = std::make_unique<Cpu>(m_Cartridge.get());
	m_Timer = std::make_unique<Timer>();
	m_Ram = std::make_unique<Ram>();
	m_Display = std::make_unique<Display>();
	m_Ppu = std::make_unique<Ppu>();
	m_Dma = std::make_unique<Dma>();
	m_Joypad = std::make_unique<Joypad>();
	m_Apu = std::make_unique<Apu>();

	m_Context.cpu = m_Cpu.get();
	m_Context.bus = this;
}

Emulator::~Emulator()
{
	m_TraceLog.close();
}

bool Emulator::LoadRom(const std::string& path)
{
	std::ifstream file(path, std::ios::binary);
	if (!file.is_open())
	{
		return false;
	}

	std::vector<uint8_t> data;
	data.clear();
	data.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

	return LoadRom(data);
}

bool Emulator::LoadRom(const std::vector<uint8_t>& filedata)
{
	// Allocate memory
	m_Cartridge = LoadCartridgeFromMemory(filedata);
	m_Cpu = std::make_unique<Cpu>(m_Cartridge.get());
	m_Timer = std::make_unique<Timer>();
	m_Ram = std::make_unique<Ram>();
	m_Display = std::make_unique<Display>();
	m_Joypad = std::make_unique<Joypad>();

	m_Ppu = std::make_unique<Ppu>(this, m_Cpu.get(), m_Display.get(), m_Cartridge.get());
	m_Dma = std::make_unique<Dma>();
	m_Apu = std::make_unique<Apu>();

	m_Context.cpu = m_Cpu.get();
	m_Context.bus = this;

	// Initialise subsystems
	m_Cpu->Init();
	m_Timer->Init();
	m_Ppu->Init();
	m_Apu->Init();

	// Load RAM if cartridge has a battery
	if (m_Cartridge->HasBattery())
	{
		{
			std::string filename = m_BatteryPath + m_Cartridge->GetCartridgeData().title + ".save";
			std::ifstream battery(filename, std::ios::in | std::ios::binary);

			// Discard is currently required it will break current battery saves if removed
			uint8_t discard = 0;
			battery.read(reinterpret_cast<char*>(&discard), 1);

			std::vector<uint8_t> external_ram;
			external_ram.resize(m_Cartridge->GetExternalRam().size());

			battery.read(reinterpret_cast<char*>(external_ram.data()), external_ram.size());
			m_Cartridge->SetExternalRam(std::move(external_ram));

			battery.close();
		}

		// Set write
		m_Cartridge->SetWriteRamCallback([&]
		{
			std::string filename = m_BatteryPath + m_Cartridge->GetCartridgeData().title + ".save";
			std::ofstream battery(filename, std::ios::out | std::ios::binary);

			uint8_t discard = 0;
			battery.write(reinterpret_cast<char*>(&discard), 1);

			std::vector<uint8_t> external_ram = m_Cartridge->GetExternalRam();
			battery.write(reinterpret_cast<char*>(external_ram.data()), external_ram.size());

			battery.close();
		});
	}

	m_Running = true;
	return true;
}

void Emulator::Stop()
{
	m_Running = false;
}

void Emulator::SetSpeedMode()
{
	if (m_Cartridge->GetCartridgeData().colour_mode == ColourModeV2::CGB)
	{
		if ((m_DoubleSpeedMode & 0x1) == 1)
		{
			// Enter double speed mode
			m_DoubleSpeedMode = 0x80;
		}
		else
		{
			// Enter single speed mode
			m_DoubleSpeedMode = 0x0;
		}
	}
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
	Cycle(1);

	m_CurrentOpCode = opcode;

	// Execute
	if (!m_Halted)
	{
#ifdef _MSC_VER
		if (IsTraceLogEnabled())
		{
			std::string debug_format = std::format("OP:{:X},PC:{:X},AF:{:X},BC:{:X},DE:{:X},HL:{:X},SP:{:X}",
												   opcode,
												   m_Context.cpu->ProgramCounter,
												   m_Context.cpu->GetRegister(RegisterType16::REG_AF),
												   m_Context.cpu->GetRegister(RegisterType16::REG_BC),
												   m_Context.cpu->GetRegister(RegisterType16::REG_DE),
												   m_Context.cpu->GetRegister(RegisterType16::REG_HL),
												   m_Context.cpu->StackPointer);

			m_TraceLog << debug_format << std::endl;
		}
#endif

		if (m_HaltBug)
		{
			m_Cpu->ProgramCounter -= 1;
			m_HaltBug = false;
		}

		m_Cpu->Execute(&m_Context, opcode);
	}
	else
	{
		Cycle(1);

		// Check if we have interrupts
		if (!m_Cpu->GetInterruptMasterFlag())
		{
			uint8_t flags = m_Cpu->GetInterruptFlags();
			uint8_t enabled = m_Cpu->GetInterruptEnable();

			if (flags != 0 && ((flags & enabled) != 0))
			{
				m_Halted = false;
			}
		}
	}

	// Check flag
	m_Cpu->HandleInterrupts();

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
	for (int i = 0; i < machine_cycles; ++i)
	{
		for (int n = 0; n < 4; ++n)
		{
			m_Timer->Tick();
			if (IsDoubleSpeedMode())
			{
				m_Timer->Tick();
			}

			m_Ppu->Tick();
		}

		m_Dma->Tick();
		if (IsDoubleSpeedMode())
		{
			m_Dma->Tick();
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
		return m_Apu->Read(address);
	}
	else if (((address >= 0xFF40) && (address <= 0xFF4B)))
	{
		return m_Display->Read(address);
	}
	else if (address == 0xFF4D)
	{
		if (m_Cartridge->GetCartridgeData().colour_mode == ColourModeV2::CGB)
		{
			return m_DoubleSpeedMode;
		}

		return 0xFF;
	}
	else if (address == 0xFF4F)
	{
		return m_Ppu->GetVideoRamBank();
	}
	else if (address >= 0xFF51 && address <= 0xFF54)
	{
		return 0xFF;
	}
	else if (address == 0xFF55)
	{
		return m_Dma->GetHDMA5();
	}
	else if (address >= 0xFF68 && address <= 0xFF6B)
	{
		return m_Display->Read(address);
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
		m_Apu->Write(address, value);
		return;
	}
	else if (((address >= 0xFF40) && (address <= 0xFF4B)))
	{
		m_Display->Write(address, value);
		return;
	}
	else if (address == 0xFF4D)
	{
		if (m_Cartridge->GetCartridgeData().colour_mode == ColourModeV2::CGB)
		{
			m_DoubleSpeedMode |= value & 0x1;
		}

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
		return m_Ram->ReadEchoRam(address);
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
		uint8_t high_nibble = address & 0xF0;
		return (high_nibble | high_nibble >> 4);
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
		m_Ram->WriteEchoRam(address, value);
		return;
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

void Emulator::SaveState(const std::string& filepath)
{
	std::lock_guard<std::mutex> lock(m_EmulatorMutex);
	std::fstream file(filepath + m_Cartridge->GetCartridgeData().title + ".state", std::ios::binary | std::ios::out);

	m_Cpu->SaveState(&file);
	m_Timer->SaveState(&file);
	m_Ram->SaveState(&file);
	m_Cartridge->SaveState(&file);

	m_Display->SaveState(&file);
	m_Ppu->SaveState(&file);
	m_Dma->SaveState(&file);
}

void Emulator::LoadState(const std::string& filepath)
{
	std::lock_guard<std::mutex> lock(m_EmulatorMutex);
	std::fstream file(filepath + m_Cartridge->GetCartridgeData().title + ".state", std::ios::binary | std::ios::in);

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
	return m_Display->GetVideoBuffer();
}

int Emulator::GetVideoPitch()
{
	return sizeof(uint32_t) * m_Display->ScreenResolutionX;
}

void Emulator::SetBatteryPath(const std::string& path)
{
	m_BatteryPath = path;
}