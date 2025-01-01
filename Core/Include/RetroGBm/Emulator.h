#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <array>
#include <string>
#include <fstream>
#include <mutex>
#include <ctime>
#include <chrono>
#include <map>

class Cpu;
class Ppu;
class Ram;
class Dma;
class Timer;
class Cartridge;
class Joypad;
class Display;
class IBus;
class BaseCartridge;
class Apu;

struct EmulatorContext
{
	Cpu* cpu = nullptr;
	IBus* bus = nullptr;

	uint64_t ticks = 0;
	int cycles = 0;
};

class IBus
{
public:
	IBus() = default;
	virtual ~IBus() = default;

	virtual uint8_t ReadBus(uint16_t address) = 0;
	virtual void WriteBus(uint16_t address, uint8_t value) = 0;
};

class Emulator : public IBus
{
public:
	Emulator();
	Emulator(std::unique_ptr<BaseCartridge> cartridge);
	virtual ~Emulator();

	static Emulator* Instance;
	bool LoadRom(const std::string& path);
	bool LoadRom(const std::vector<uint8_t>& filedata);

	void Tick();
	void Cycle(int machine_cycles);

	void Stop();
	void SetHalt(bool value);
	void SetSpeedMode();
	inline bool IsRunning() { return m_Running; }
	int GetFPS();

	void Pause(bool pause);
	inline bool IsPaused() const { return m_Paused; }

	uint8_t GetOpCode() const;
	inline EmulatorContext* GetContext() { return &m_Context; }

	// Bus
	uint8_t ReadBus(uint16_t address) override;
	void WriteBus(uint16_t address, uint8_t value) override;

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
	inline Ram* GetRam() { return m_Ram.get(); }
	inline Joypad* GetJoypad() { return m_Joypad.get(); }
	inline BaseCartridge* GetCartridge() { return m_Cartridge.get(); }

	void* GetVideoBuffer();
	int GetVideoPitch();

	inline bool IsDoubleSpeedMode() const { return m_DoubleSpeedMode; }

	// Trace log
	void ToggleTraceLog(bool enable);
	inline bool IsTraceLogEnabled() const { return m_EnableTraceLog; }

	// Save states
	void SaveState(const std::string& filepath);
	void LoadState(const std::string& filepath);

	// Halt bug
	bool m_HaltBug = false;
	bool m_HaltNoJump = false;

	void SetBatteryPath(const std::string& path);

private:
	std::mutex m_EmulatorMutex;
	bool m_Paused = false;

	char m_SerialData[2] = { 0, 0 };

	uint8_t m_CurrentOpCode = 0x0;

	std::string m_DebugMessage;

	// Subcomponents
	std::unique_ptr<Cpu> m_Cpu;
	std::unique_ptr<Timer> m_Timer;
	std::unique_ptr<Ram> m_Ram;
	std::unique_ptr<BaseCartridge> m_Cartridge;
	std::unique_ptr<Display> m_Display;
	std::unique_ptr<Ppu> m_Ppu;
	std::unique_ptr<Dma> m_Dma;
	std::unique_ptr<Joypad> m_Joypad;
	std::unique_ptr<Apu> m_Apu;

	bool m_Running = false;
	bool m_Halted = false;
	uint8_t m_DoubleSpeedMode = 0;

	bool m_EnableTraceLog = false;
	std::ofstream m_TraceLog;

	std::string m_BatteryPath;

	bool GetSaveStateDateCreated(const std::string& filepath, time_t* dateCreated, double* time_played);

	// Save state timestamps
	std::chrono::steady_clock::time_point m_CurrentTimeStamp;
	std::map<std::string, std::chrono::steady_clock::time_point> m_StateTimestamps;
};