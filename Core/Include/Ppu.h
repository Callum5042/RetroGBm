#pragma once

#include <cstdint>
#include "Display.h"
#include <vector>
#include <array>
#include <queue>
#include <chrono>

class IBus;
class Cpu;

enum class FetchState
{
	Tile,
	TileDataLow,
	TileDataHigh,
	Idle,
	Push,
};

struct PpuContext
{
	//pixel_fifo_context pfc;

	//uint8_t line_sprite_count; //0 to 10 sprites.
	//OamLineNode* line_sprites; // Linked list of current sprites on line
	//std::array<OamLineNode, 10> line_entry_array;

	//uint8_t fetched_entry_count;
	//OamData fetched_entries[3]; //entries fetched during pipeline.
	//uint8_t window_line;

	//uint32_t current_frame;
	//uint32_t dot_ticks;

	//std::array<OamData, 40> oam_ram;

	uint32_t dot_ticks;
	std::vector<uint32_t> video_buffer;
	std::vector<uint8_t> video_ram;
};

class Ppu
{
	IBus* m_Bus = nullptr;
	Cpu* m_Cpu = nullptr;
	Display* m_Display = nullptr;

public:
	Ppu();
	virtual ~Ppu() = default;

	void Init();
	void Tick();

	// OAM
	/*void WriteOam(uint16_t address, uint8_t value);
	uint8_t ReadOam(uint16_t address);*/

	// VRAM
	void WriteVideoRam(uint16_t address, uint8_t value);
	uint8_t ReadVideoRam(uint16_t address);

	// Modes
	void UpdateOam();
	void PixelTransfer();
	void HBlank();
	void VBlank();

	// Dimensions
	int ScreenResolutionX = 160;
	int ScreenResolutionY = 144;

	PpuContext m_Context;

private:
	int m_PixelX = 0;

	// Accessible areas
	bool CanAccessVRam();
	bool CanAccessOAM();
};