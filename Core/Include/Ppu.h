#pragma once

#include <cstdint>
#include "Display.h"
#include <vector>
#include <array>
#include <queue>
#include "HighTimer.h"

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

enum class FetchTileByte
{
	ByteLow,
	ByteHigh
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

struct OamPipelineData
{
	OamData* oam = nullptr;
	uint8_t byte_low;
	uint8_t byte_high;
};

struct PipelineContext
{
	FetchState pipeline_state = FetchState::Tile;
	uint8_t line_x = 0;
	uint8_t pushed_x = 0;
	uint8_t fetch_x = 0;

	uint8_t background_window_tile = 0;
	uint8_t background_window_byte_low = 0;
	uint8_t background_window_byte_high = 0;

	std::vector<OamPipelineData> fetched_entries;
	std::queue<uint32_t> pixel_queue;
	uint8_t fifo_x;
};

struct PpuContext
{
	uint32_t dot_ticks = 0;
	uint8_t window_line_counter = 0;

	std::vector<uint32_t> video_buffer;
	std::vector<uint8_t> video_ram;
	std::array<OamData, 40> oam_ram;
	std::vector<OamData> objects_per_line;

	PipelineContext pipeline;
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

	void SetVideoRamBank(uint8_t value);
	inline uint8_t GetVideoRamBank() const { return m_VramBank; }

	inline PpuContext* GetContext() { return &m_Context; }

	const uint16_t ScreenResolutionY = 144;
	const uint16_t ScreenResolutionX = 160;

	// Save state
	void SaveState(std::fstream* file);
	void LoadState(std::fstream* file);

private:
	PpuContext m_Context = {};

	const uint16_t m_LinesPerFrame = 154;
	const uint16_t m_DotTicksPerLine = 456;

	// Modes
	void UpdateOam();
	void PixelTransfer();
	void VBlank();
	void HBlank();

	// Pipeline
	void PipelineProcess();
	bool PipelineAddPixel();

	void LoadSpriteTile();
	void FetchTileData(FetchTileByte tile_byte);

	uint32_t FetchSpritePixels(uint32_t color, bool background_pixel_transparent);
	void LoadSpriteData(FetchTileByte tile_byte);
	void LoadWindowTile();

	void PixelFetcher();
	void PushPixelToVideoBuffer();

	bool IsWindowVisible();
	bool IsWindowInView(int pixel_x);
	void IncrementLY();

	// Limit frame rate
	void LimitFrameRate();
	HighTimer m_Timer;
	float m_TargetFrameTime = 1.0f / 60.0f;
	int m_FramesPerSecond = 0;
	int m_TotalFrames = 0;

	int m_FrameCount = 0;
	float m_TimeElapsed = 0.0f;

	// Bank
	uint8_t m_VramBank = 0;
};