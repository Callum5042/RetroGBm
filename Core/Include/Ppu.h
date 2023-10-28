#pragma once

#include <cstdint>
#include "Display.h"
#include <vector>
#include <array>
#include <queue>

class IBus;

enum class FetchState
{
	Tile,
	TileDataLow,
	TileDataHigh,
	Idle,
	Push,
};

struct pixel_fifo_context
{
	FetchState current_fetch_state;
	uint8_t line_x;
	uint8_t pushed_x;
	uint8_t fetch_x;
	uint8_t bgw_fetch_data[3];
	uint8_t fetch_oam_data[6]; //oam data..
	uint8_t map_y;
	uint8_t map_x;
	uint8_t tile_y;
	uint8_t fifo_x;

	std::queue<uint32_t> pixel_queue;
};

// The order of these variables are mapped to the memory and cannot be changed
struct OamData
{
	uint8_t y; // Byte 1 - Y Position
	uint8_t x; // Byte 2 - X Position
	uint8_t tile_index; // Byte 3 - Tile Index

	uint8_t flag_cgb_palette : 3; // Byte 4 (bit 0, 1, 2) - CGB palette [CGB Mode Only]: Which of OBP0–7 to use
	bool flag_cgb_bank : 1; // Byte 4 (bit 3) - Bank [CGB Mode Only]: 0 = Fetch tile from VRAM bank 0, 1 = Fetch tile from VRAM bank 1
	bool flag_dmg_palette : 1; // Byte 4 (bit 4) - DMG palette [Non CGB Mode only]: 0 = OBP0, 1 = OBP1
	bool flag_x_flip : 1; // Byte 4 (bit 5) - X flip: 0 = Normal, 1 = Entire OBJ is horizontally mirrored
	bool flag_y_flip : 1; // Byte 4 (bit 6) - Y flip: 0 = Normal, 1 = Entire OBJ is vertically mirrored
	bool flag_priority : 1; // Byte 4 (bit 7) - Priority: 0 = No, 1 = BG and Window colors 1–3 are drawn over this OBJ
};

struct OamLineNode
{
	OamData entry;
	struct OamLineNode* next;
};

struct PpuContext
{
	pixel_fifo_context pfc;

	uint8_t line_sprite_count; //0 to 10 sprites.
	OamLineNode* line_sprites; // Linked list of current sprites on line
	std::array<OamLineNode, 10> line_entry_array;

	uint8_t fetched_entry_count;
	OamData fetched_entries[3]; //entries fetched during pipeline.
	uint8_t window_line;

	uint32_t current_frame;
	uint32_t dot_ticks;

	std::array<OamData, 40> oam_ram;
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
	void WriteOam(uint16_t address, uint8_t value);
	uint8_t ReadOam(uint16_t address);

	// VRAM
	void WriteVideoRam(uint16_t address, uint8_t value);
	uint8_t ReadVideoRam(uint16_t address);

	// Modes
	void UpdateOam();
	void PixelTransfer();
	void VBlank();
	void HBlank();

	// Calculate FPS
	void CalculateFPS();
	uint32_t m_TargetFrameTime = 1000 / 60;
	long m_PreviousFrameTime = 0;
	long m_StartTimer = 0;
	long m_FrameCount = 0;

	// Pipeline
	void PipelineReset();
	void PipelineProcess();
	bool PipelineAddPixel();

	void LoadSpriteTile();
	uint32_t FetchSpritePixels(int bit, uint32_t color, uint8_t bg_color);
	void LoadSpriteData(uint8_t offset);
	void LoadWindowTile();

	void PixelFetcher();
	void PushPixelToVideoBuffer();

	bool IsWindowVisible();
	void IncementLY();
	void LoadSpritesForLine();

	PpuContext context = {};

	const uint16_t ScreenResolutionY = 144;
	const uint16_t ScreenResolutionX = 160;

	const uint16_t m_LinesPerFrame = 154;
	const uint16_t m_DotTicksPerLine = 456;
};