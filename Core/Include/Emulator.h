#pragma once

#include <filesystem>
#include <memory>
#include <unordered_map>
#include <functional>
#include <vector>
#include <array>
#include <string>
#include <fstream>

class Cpu;
struct CartridgeInfo;

struct TimerContext
{
	uint16_t div;
	uint8_t tima;
	uint8_t tma;
	uint8_t tac;
};

struct DisplayContext
{
	uint8_t lcdc;
	uint8_t stat;
	uint8_t scy;
	uint8_t scx;
	uint8_t ly;
	uint8_t lyc;
	uint8_t dma;
	uint8_t bgp;
	uint8_t obp0;
	uint8_t obp1;
	uint8_t wy;
	uint8_t wx;
};

struct PpuContext
{
	uint32_t line_ticks = 0;
	std::vector<uint32_t> video_buffer;
};

struct EmulatorContext
{
	uint64_t ticks = 0;
	int cycles = 0;
	std::unique_ptr<CartridgeInfo> cartridge = nullptr;
	std::unique_ptr<Cpu> cpu = nullptr;
	std::vector<uint8_t> video_ram;
	std::vector<uint8_t> work_ram;
	std::array<uint8_t, 127> high_ram;
	std::array<char, 2> serial_data;

	TimerContext timer;
	DisplayContext display;
	PpuContext ppu_context;
};

class Emulator
{
public:
	Emulator();
	virtual ~Emulator();

	bool LoadRom(const std::filesystem::path& path);

	void Tick();

	uint8_t GetOpCode() const;

	inline EmulatorContext* GetContext() { return &m_Context; }

private:
	EmulatorContext m_Context;

	std::string Execute(const uint8_t opcode);

	uint8_t m_CurrentOpCode = 0x0;

	std::string m_DebugMessage;
	std::ofstream m_DebugFile;
};