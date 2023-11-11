#pragma once

#include <cstdint>
#include "Display.h"
#include <vector>
#include <array>
#include <queue>
#include <chrono>

class IBus;

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
	void VBlank();
	void HBlank();

	// Dimensions
	int ScreenResolutionX = 144;
	int ScreenResolutionY = 160;

	PpuContext m_Context;
};