#pragma once

#include <filesystem>
#include <memory>
#include <unordered_map>
#include <functional>
#include <vector>
#include <array>
#include <string>

class Cpu;
struct CartridgeInfo;

struct TimerContext
{
	uint16_t div;
	uint8_t tima;
	uint8_t tma;
	uint8_t tac;
};

struct EmulatorContext
{
	int cycles = 0;
	std::unique_ptr<CartridgeInfo> cartridge = nullptr;
	std::unique_ptr<Cpu> cpu = nullptr;
	std::vector<uint8_t> video_ram;
	std::vector<uint8_t> work_ram;
	std::array<char, 2> serial_data;

	TimerContext timer;
};

class Emulator
{
public:
	Emulator();
	virtual ~Emulator() = default;

	bool LoadRom(const std::filesystem::path& path);

	void Tick();

	uint8_t GetOpCode() const;

private:
	bool m_Running = false;

	EmulatorContext m_Context;

	std::string Execute(const uint8_t opcode);

	uint8_t m_CurrentOpCode = 0x0;

	std::string m_DebugMessage;

	// std::unordered_map<uint8_t, std::function<std::string()>> m_OpCodeTable;
};