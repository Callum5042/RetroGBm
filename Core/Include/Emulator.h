#pragma once

#include <filesystem>
#include <memory>
#include "Cartridge.h"

class Cpu;

class Emulator
{
public:
	Emulator();
	virtual ~Emulator() = default;

	bool LoadRom(const std::filesystem::path& path);

	void Run();

private:
	bool m_Running = false;

	CartridgeInfo m_CartridgeInfo;
	std::unique_ptr<Cpu> m_Cpu = nullptr;

	const uint8_t ReadFromBus();
};