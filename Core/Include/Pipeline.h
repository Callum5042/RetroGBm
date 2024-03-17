#pragma once

#include <vector>
#include <queue>
#include <cstdint>

class Display;
class Cartridge;
class Ppu;

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
	FetchState pipeline_state = FetchState::Tile;
	uint8_t line_x = 0;
	uint8_t pushed_x = 0;
	uint8_t fetch_x = 0;

	BackgroundWindowAttribute background_window_attribute;
	uint8_t background_window_tile = 0;
	uint8_t background_window_byte_low = 0;
	uint8_t background_window_byte_high = 0;

	std::vector<OamPipelineData> fetched_entries;
	std::queue<uint32_t> pixel_queue;
	uint8_t fifo_x;
};

class Pipeline
{
	Ppu* m_Ppu = nullptr;
	Display* m_Display = nullptr;
	Cartridge* m_Cartridge = nullptr;

public:
	Pipeline(Ppu* ppu, Display* display, Cartridge* cartridge);
	virtual ~Pipeline() = default;

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
};