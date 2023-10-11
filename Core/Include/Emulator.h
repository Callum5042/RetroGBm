#pragma once

#include <filesystem>
#include <memory>
#include <unordered_map>
#include <functional>

class Cpu;
struct CartridgeInfo;

struct EmulatorContext
{
	int cycles = 0;
	std::unique_ptr<CartridgeInfo> cartridge = nullptr;
	std::unique_ptr<Cpu> cpu = nullptr;
	std::vector<uint8_t> video_ram;
	std::vector<uint8_t> work_ram;
};

class Emulator
{
public:
	Emulator();
	virtual ~Emulator() = default;

	bool LoadRom(const std::filesystem::path& path);

	void Run();

	uint8_t GetOpCode() const;

private:
	bool m_Running = false;

	EmulatorContext m_Context;

	std::string Execute(const uint8_t opcode);

	uint8_t m_CurrentOpCode = 0x0;

	// std::unordered_map<uint8_t, std::function<std::string()>> m_OpCodeTable;
};