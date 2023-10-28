#pragma once

#include <memory>
#include <unordered_map>
#include <functional>
#include <vector>
#include <array>
#include <string>
#include <fstream>

#include "Bus.h"
#include "Cpu.h"
#include "Timer.h"
#include "Ram.h"
#include "Cartridge.h"
#include "Display.h"
#include "Ppu.h"
#include "Dma.h"
#include "Joypad.h"

struct EmulatorContext
{
	Cpu* cpu = nullptr;

	uint64_t ticks = 0;
	int cycles = 0;
};

class Emulator : public IBus
{
public:
	Emulator();
	virtual ~Emulator();

	static Emulator* Instance;
	bool LoadRom(const std::string& path);

	void Tick();

	uint8_t GetOpCode() const;
	inline EmulatorContext* GetContext() { return &m_Context; }

	// Bus
	uint8_t ReadBus(uint16_t address) override;
	void WriteBus(uint16_t address, uint8_t value) override;

	uint16_t ReadBus16(uint16_t address);
	void WriteBus16(uint16_t address, uint16_t value);

	uint8_t ReadIO(uint16_t address);
	void WriteIO(uint16_t address, uint8_t value);

	// Stack
	void StackPush(uint8_t data);
	void StackPush16(uint16_t data);

	uint8_t StackPop();
	uint16_t StackPop16();

	EmulatorContext m_Context;

	inline Cpu* GetCpu() { return m_Cpu.get(); }
	inline Display* GetDisplay() { return m_Display.get(); }
	inline Ppu* GetPpu() { return m_Ppu.get(); }
	inline Dma* GetDma() { return m_Dma.get(); }
	inline Joypad* GetJoypad() { return m_Joypad.get(); }

private:

	char m_SerialData[2] = { 0, 0 };

	uint8_t m_CurrentOpCode = 0x0;

	std::string m_DebugMessage;
	std::ofstream m_DebugFile;

	// Subcomponents
	std::unique_ptr<Cpu> m_Cpu;
	std::unique_ptr<Timer> m_Timer;
	std::unique_ptr<Ram> m_Ram;
	std::unique_ptr<Cartridge> m_Cartridge;
	std::unique_ptr<Display> m_Display;
	std::unique_ptr<Ppu> m_Ppu;
	std::unique_ptr<Dma> m_Dma;
	std::unique_ptr<Joypad> m_Joypad;
};