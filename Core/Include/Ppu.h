#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <queue>
#include <memory>

#include "Display.h"
#include "HighTimer.h"
#include "Pipeline.h"

class IBus;
class Cpu;
class Cartridge;

struct PpuContext
{
	uint32_t dot_ticks = 0;
	uint8_t window_line_counter = 0;

	std::vector<uint32_t> video_buffer;
	std::vector<uint32_t> blank_video_buffer;
	std::vector<uint8_t> video_ram;
	std::array<OamData, 40> oam_ram;
	std::vector<OamData> objects_per_line;
};

class Ppu
{
	IBus* m_Bus = nullptr;
	Cpu* m_Cpu = nullptr;
	Display* m_Display = nullptr;
	Cartridge* m_Cartridge = nullptr;

public:
	Ppu();
	Ppu(IBus* bus, Cpu* cpu, Display* display, Cartridge* cartridge);
	virtual ~Ppu() = default;

	void Init();
	void Tick();

	void* GetVideoBuffer();

	// OAM
	void WriteOam(uint16_t address, uint8_t value);
	uint8_t ReadOam(uint16_t address);

	// VRAM
	void WriteVideoRam(uint16_t address, uint8_t value);
	uint8_t ReadVideoRam(uint16_t address);
	uint8_t ReadVideoRam(uint16_t address, uint8_t bank);

	void SetVideoRamBank(uint8_t value);
	inline uint8_t GetVideoRamBank() const { return m_VramBank; }

	inline PpuContext* GetContext() { return &m_Context; }

	const uint16_t ScreenResolutionY = 144;
	const uint16_t ScreenResolutionX = 160;

	inline int GetFPS() { return m_FramesPerSecond; }

	// Save state
	void SaveState(std::fstream* file);
	void LoadState(std::fstream* file);

	bool IsWindowVisible();

private:
	PpuContext m_Context = {};
	std::unique_ptr<Pipeline> m_Pipeline = nullptr;

	const uint16_t m_LinesPerFrame = 154;
	const uint16_t m_DotTicksPerLine = 456;

	// Modes
	void UpdateOam();
	void PixelTransfer();
	void VBlank();
	void HBlank();

	void IncrementLY();
	void CheckLYCFlag();

	// Limit frame rate
	void LimitFrameRate();
	double m_TargetFrameTime = 1.0f / 60.0f;
	int m_FramesPerSecond = 0;
	int m_TotalFrames = 0;

	HighTimer m_Timer;

	int m_FrameCount = 0;
	float m_TimeElapsed = 0.0f;

	// Bank
	uint8_t m_VramBank = 0;
};