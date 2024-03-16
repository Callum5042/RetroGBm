#pragma once

#include <cstdint>
#include <vector>

class Cpu;
class Display;

struct PixelProcessorContext
{
	// Frame dots is dots for the whole frame. HBlank should always be 456 and frame total should always be 70224 
	int dots = 0;
	int frame_dots = 0;

	int window_line = 0;

	uint8_t video_ram_bank = 0;
	std::vector<uint8_t> video_ram;
};

class PixelProcessor
{
	Display* m_Display = nullptr;
	Cpu* m_Cpu = nullptr;

public:
	PixelProcessor(Display* display);
	PixelProcessor(Display* display, Cpu* cpu);

	void Init();
	void Tick();

	// Modes
	void UpdateOam();
	void UpdatePixelTransfer();
	void UpdateHBlank();
	void UpdateVBlank();

	// Read and write to VRAM
	void WriteVideoRam(uint16_t address, uint8_t value);

	uint8_t ReadVideoRam(uint16_t address);
	uint8_t PipelineReadVideoRam(uint16_t address, int bank);

	// TODO: Move this to Display.h
	bool IsVideoRamAccessable();

	// Get and set VRAM registers (0xFF4F)
	uint8_t GetVideoRamBank() const;
	void SetVideoRamBank(uint8_t value);

	// Get context
	inline const PixelProcessorContext* GetContext() { return &m_Context; }

private:
	PixelProcessorContext m_Context;
};