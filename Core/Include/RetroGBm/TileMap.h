#pragma once

#include <cstdint>
#include <optional>
#include <vector>

// Decoding of the two 32 x 32 tile maps stored in VRAM at $9800-$9BFF and $9C00-$9FFF.
// https://gbdev.io/pandocs/Tile_Maps.html
namespace TileMap
{
	constexpr int Columns = 32;
	constexpr int Rows = 32;
	constexpr int Entries = Columns * Rows;
	constexpr int Width = Columns * 8;
	constexpr int Height = Rows * 8;

	constexpr uint16_t LowMap = 0x9800;
	constexpr uint16_t HighMap = 0x9C00;

	// LCDC.3 selects the background map and LCDC.6 the window map
	uint16_t BackgroundMap(uint8_t lcdc);
	uint16_t WindowMap(uint8_t lcdc);

	// LCDC.4 picks how the background and window find their tiles:
	// set, IDs 0-255 count up from $8000; clear, IDs are signed (-128 to 127) from $9000.
	enum class Addressing { Unsigned, Signed };
	Addressing AddressingFromLcdc(uint8_t lcdc);

	// Index of the tile an ID uses within a bank of tile data (0-383)
	int TileIndex(uint8_t id, Addressing addressing);

	// Address of the first byte of the tile an ID uses
	uint16_t TileAddress(uint8_t id, Addressing addressing);

	// CGB attribute byte, stored in VRAM bank 1 at the same address as the tile ID.
	// https://gbdev.io/pandocs/Tile_Maps.html#bg-map-attributes-cgb-mode-only
	struct Attributes
	{
		uint8_t palette = 0;
		uint8_t bank = 0;
		bool flip_x = false;
		bool flip_y = false;
		bool priority = false;
	};

	Attributes DecodeAttributes(uint8_t value);

	struct Image
	{
		// Width x Height colour indices (0-3), row by row
		std::vector<uint8_t> colours;

		// Width x Height background palette numbers (0-7), row by row. All 0 without attributes.
		std::vector<uint8_t> palettes;
	};

	// Decodes a map of Entries tile IDs into an image. attributes is Entries bytes, or null for DMG.
	// bank0 and bank1 are TileData::BytesPerBank bytes from $8000; bank1 may be null when there are no attributes.
	Image Decode(const uint8_t* ids, const uint8_t* attributes, const uint8_t* bank0, const uint8_t* bank1, Addressing addressing);

	struct Rect
	{
		int x = 0;
		int y = 0;
		int width = 0;
		int height = 0;

		bool IsEmpty() const { return width <= 0 || height <= 0; }
		bool operator==(const Rect& other) const = default;
	};

	// A rectangle on the map, which wraps at the edges, split into up to four pieces that lie inside it
	std::vector<Rect> Wrap(const Rect& rect);

	// The screen (160 x 144) as the background map sees it, at SCX, SCY
	Rect Viewport(uint8_t scx, uint8_t scy);

	// The part of the screen the window covers, in screen pixels. Empty when the window is off (LCDC.5) or out of view.
	Rect WindowOnScreen(uint8_t lcdc, uint8_t wx, uint8_t wy);

	// The part of the window map shown on screen. Empty when the window isn't shown.
	Rect WindowOnMap(uint8_t lcdc, uint8_t wx, uint8_t wy);

	struct Point
	{
		int x = 0;
		int y = 0;
		bool operator==(const Point& other) const = default;
	};

	// Where the top-left of a map entry lands on screen for SCX, SCY, or nothing when no part of it is visible.
	// The position can be negative for an entry cut by the left or top edge.
	std::optional<Point> ScreenPosition(int column, int row, uint8_t scx, uint8_t scy);
}
