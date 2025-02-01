#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <deque>

#include "RetroGBm/Display.h"
#include "RetroGBm/HighTimer.h"

class IBus;
class Cpu;
class BaseCartridge;

enum class FetchState
{
	Tile,
	TileDataLow,
	TileDataHigh,
	Idle,
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
	uint8_t dmg_palette : 1;
	bool flip_x : 1;
	bool flip_y : 1;
	bool priority : 1;
};

struct OamPipelineData
{
	OamData* oam = nullptr;
	uint8_t byte_low = 0;
	uint8_t byte_high = 0;
};

struct BackgroundWindowAttribute
{
	uint8_t colour_palette;
	uint8_t bank;
	bool flip_x;
	bool flip_y;
	bool priority;
};

struct PipelineContext
{
	FetchState pipeline_state = FetchState::Tile;
	uint8_t line_x = 0;
	uint8_t pushed_x = 0;
	uint8_t fetch_x = 0;

	BackgroundWindowAttribute background_window_attribute;
	uint8_t background_window_tile = 0;
	uint8_t background_window_byte_low = 0;
	uint8_t background_window_byte_high = 0;

	std::vector<OamPipelineData> fetched_entries;
	std::deque<uint32_t> pixel_queue;
	uint8_t fifo_x;

	bool fetch_window = false;
};

struct PpuContext
{
	uint32_t dot_ticks = 0;
	uint8_t window_line_counter = 0;

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
	BaseCartridge* m_Cartridge = nullptr;

public:
	Ppu();
	Ppu(IBus* bus, Cpu* cpu, Display* display, BaseCartridge* cartridge);
	virtual ~Ppu() = default;

	void Init();
	void Tick();

	void SetSpeedMultipler(float speed);

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

	inline int GetFPS() { return m_FramesPerSecond; }

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

	void FetchBackgroundTileId();
	void FetchWindowTileId();
	void FetchObjectTileId();

	void FetchTileData(FetchTileByte tile_byte);
	void FetchObjectData(FetchTileByte tile_byte);

	uint32_t FetchSpritePixels(uint32_t color, bool background_pixel_transparent);

	void PixelFetcher();
	void PushPixelToVideoBuffer();

	bool IsWindowInView(int pixel_x);
	void IncrementLY();

	// Limit frame rate
	void LimitFrameRate();
	double m_TargetFrameTime = 1.0f / 60.0f;
	int m_FramesPerSecond = 0;
	int m_TotalFrames = 0;

	HighTimer m_Timer;

	int m_FrameCount = 0;
	float m_TimeElapsed = 0.0f;

	float m_SpeedMultipler = 1.0f;

	// Bank
	uint8_t m_VramBank = 0;

	// WY Register cannot change mid scanline
	int m_WindowY = 0;
};