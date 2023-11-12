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

struct pixel_fifo_context
{
	FetchState pipeline_state;
	uint8_t line_x;
	uint8_t pushed_x;
	uint8_t fetch_x;

	uint8_t background_window_tile = 0;
	uint8_t background_window_byte_low = 0;
	uint8_t background_window_byte_high = 0;

	uint8_t fetch_oam_data[6];
	uint8_t fifo_x;

	std::queue<uint32_t> pixel_queue;
};

struct OamData
{
	uint8_t position_y;
	uint8_t position_x;
	uint8_t tile_id;

	uint8_t gcb_palette : 3;
	bool bank : 1;
	bool dmg_palette : 1;
	bool flip_x : 1;
	bool flip_y : 1;
	bool priority : 1;
};

struct PpuContext
{
	pixel_fifo_context pfc;

	std::vector<OamData> fetched_oam_data;
	std::vector<OamData> fetched_entries;
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
	Cpu* m_Cpu = nullptr;
	Display* m_Display = nullptr;

public:
	Ppu();
	Ppu(IBus* bus, Cpu* cpu, Display* display);
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
	float m_TargetFrameTime = 1000.0f / 60.0f;

	std::chrono::high_resolution_clock m_FpsClock;
	std::chrono::high_resolution_clock::time_point m_StartFrame;
	std::chrono::high_resolution_clock::time_point m_EndFrame;


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

	PpuContext context = {};

	const uint16_t ScreenResolutionY = 144;
	const uint16_t ScreenResolutionX = 160;

	const uint16_t m_LinesPerFrame = 154;
	const uint16_t m_DotTicksPerLine = 456;

private:
	bool IsWindowInView(int pixel_x);
};