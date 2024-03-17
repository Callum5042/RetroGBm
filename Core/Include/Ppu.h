#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <queue>
#include <memory>

#include "Display.h"
#include "HighTimer.h"
#include "Pipeline.h"

#include "PixelProcessor.h"

class IBus;
class Cpu;
class Cartridge;

struct PpuContext
{
	/*uint32_t dot_ticks = 0;
	uint8_t window_line_counter = 0;*/
};

class Ppu
{
	Cpu* m_Cpu = nullptr;
	Display* m_Display = nullptr;
	Cartridge* m_Cartridge = nullptr;

public:
	Ppu();
	Ppu(Cpu* cpu, Display* display, Cartridge* cartridge);
	virtual ~Ppu() = default;

	void Init();
	void Tick();

	// OAM
	void WriteOam(uint16_t address, uint8_t value);
	uint8_t ReadOam(uint16_t address);

	// VRAM
	void WriteVideoRam(uint16_t address, uint8_t value);
	uint8_t ReadVideoRam(uint16_t address);
	uint8_t ReadVideoRam(uint16_t address, uint8_t bank);

	// VRAM Bank
	void SetVideoRamBank(uint8_t value);
	inline uint8_t GetVideoRamBank() const { return m_PixelProcessor->GetVideoRamBank(); }

	inline PpuContext* GetContext() { return &m_Context; }
	inline int GetFPS() const { return m_PixelProcessor->GetFPS(); }

	// Save state
	void SaveState(std::fstream* file);
	void LoadState(std::fstream* file);

	bool IsWindowVisible();

private:
	PpuContext m_Context = {};

	std::unique_ptr<Pipeline> m_Pipeline = nullptr;
public:
	// TODO: Make this private
	std::unique_ptr<PixelProcessor> m_PixelProcessor = nullptr;
private:

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
};