#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <fstream>

#include "HighTimer.h"
#include "PixelPipeline.h"

class Cpu;
class Display;
class Cartridge;

struct PixelProcessorContext
{
	// Frame dots is dots for the whole frame. HBlank should always be 456 and frame total should always be 70224 
	int dots = 0;
	int window_line = 0;

	// VRAM
	uint8_t video_ram_bank = 0;
	std::vector<uint8_t> video_ram;

	// OAM RAM
	std::array<OamData, 40> oam_ram;
	std::vector<OamData> objects_per_line;
};

class PixelProcessor
{
	Display* m_Display = nullptr;
	Cpu* m_Cpu = nullptr;
	Cartridge* m_Cartridge = nullptr;

public:
	PixelProcessor(Display* display);
	PixelProcessor(Display* display, Cpu* cpu);
	PixelProcessor(Display* display, Cpu* cpu, Cartridge* cartridge);

	void Init();
	void Tick();

	// Save states
	void SaveState(std::fstream* file);
	void LoadState(std::fstream* file);

	// Modes
	void UpdateOam();
	void UpdatePixelTransfer();
	void UpdateHBlank();
	void UpdateVBlank();

	// Read and write to VRAM
	void WriteVideoRam(uint16_t address, uint8_t value);

	uint8_t ReadVideoRam(uint16_t address);
	uint8_t PipelineReadVideoRam(uint16_t address, int bank);

	// Read and write to OAM RAM
	void WriteOam(uint16_t address, uint8_t value);
	uint8_t ReadOam(uint16_t address);

	// TODO: Move this to Display.h
	bool IsVideoRamAccessable();

	// Get and set VRAM registers (0xFF4F)
	uint8_t GetVideoRamBank() const;
	void SetVideoRamBank(uint8_t value);

	// Get context
	inline const PixelProcessorContext* GetContext() { return &m_Context; }

	// Get FPS
	inline int GetFPS() const { return m_FramesPerSecond; }

private:
	PixelProcessorContext m_Context;

	std::unique_ptr<PixelPipeline> m_Pipeline = nullptr;

	void IncrementLY();
	void CheckLYCFlag();

	// Limit frame rate
	void LimitFrameRate();
	HighTimer m_Timer;
	int m_FrameCount = 0;
	float m_TimeElapsed = 0.0f;

	double m_TargetFrameTime = 1.0f / 60.0f;
	int m_FramesPerSecond = 0;
	int m_TotalFrames = 0;
};