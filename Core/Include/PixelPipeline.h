#pragma once

#include <vector>
#include <deque>
#include <cstdint>

class Display;
class Cartridge;
class PixelProcessor;

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

struct BackgroundWindowAttribute
{
	uint8_t colour_palette;
	uint8_t bank;
	bool flip_x;
	bool flip_y;
	bool priority;
};

struct OamData
{
	uint8_t position_y;
	uint8_t position_x;
	uint8_t tile_id;

	// Attribute data
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

struct PipelineContext
{
	// Queues and OAM
	std::vector<OamPipelineData> fetched_entries;
	std::deque<uint32_t> pixel_queue;

	// Internal data
	FetchState pipeline_state = FetchState::Tile;
	uint8_t scanline_x = 0;
	uint8_t pushed_x = 0;
	uint8_t fetch_x = 0;
	uint8_t fifo_x;

	int internal_dots = 0;
	bool fetch_window = false;

	// Background tile data
	BackgroundWindowAttribute background_window_attribute;
	uint8_t background_window_tile_id = 0;
	uint8_t background_window_byte_low = 0;
	uint8_t background_window_byte_high = 0;
};

class PixelPipeline
{
	PixelProcessor* m_PixelProcessor = nullptr;
	Display* m_Display = nullptr;
	Cartridge* m_Cartridge = nullptr;

public:
	PixelPipeline(PixelProcessor* ppu, Display* display, Cartridge* cartridge);
	virtual ~PixelPipeline() = default;

	void PipelineProcess();

	inline PipelineContext* GetContext() { return &m_Context; }

private:
	PipelineContext m_Context;

	void PixelFetcher();
	void PushPixelToVideoBuffer();

	void FetchTileData(FetchTileByte tile_byte);

	void LoadSpriteData(FetchTileByte tile_byte);

	void LoadSpriteTile();
	void LoadWindowTile();


	bool PipelineAddPixel();
	uint32_t FetchSpritePixels(uint32_t color, bool background_pixel_transparent);

	bool IsWindowInView(int pixel_x);

	// Fetch
	void FetchBackgroundTileId();
	void FetchWindowTileId();
	void FetchObjectTileId();
};