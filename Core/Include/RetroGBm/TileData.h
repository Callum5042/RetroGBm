#pragma once

#include <array>
#include <cstdint>
#include <vector>

// Decoding of the 2bpp tile data stored in VRAM at $8000-$97FF.
// https://gbdev.io/pandocs/Tile_Data.html
namespace TileData
{
	constexpr int BytesPerTile = 16;
	constexpr int TilesPerBank = 384;
	constexpr int BytesPerBank = TilesPerBank * BytesPerTile;

	// A bank is laid out as a sheet of 16 x 24 tiles (128 x 192 pixels)
	constexpr int SheetColumns = 16;
	constexpr int SheetRows = 24;
	constexpr int SheetWidth = SheetColumns * 8;
	constexpr int SheetHeight = SheetRows * 8;

	// Decodes one row of 8 pixels into colour indices (0-3), leftmost pixel first.
	// The first byte of the row holds bit 0 of each index and the second byte holds bit 1.
	std::array<uint8_t, 8> DecodeRow(uint8_t first, uint8_t second);

	// Decodes a 16-byte tile into 64 colour indices, row by row.
	std::array<uint8_t, 64> DecodeTile(const uint8_t* bytes);

	// Decodes a bank of tile data (BytesPerBank bytes, starting at $8000) into a
	// SheetWidth x SheetHeight image of colour indices, row by row, 16 tiles per row.
	std::vector<uint8_t> DecodeSheet(const uint8_t* bytes);

	enum class SheetLayout { SideBySide, Stacked };

	struct Image
	{
		int width = 0;
		int height = 0;

		// Row by row, in whatever 32-bit pixel format the sheets were given in
		std::vector<uint32_t> pixels;
	};

	// Lays out sheets of SheetWidth x SheetHeight pixels next to or above each other, with each pixel
	// scaled up to scale x scale. With grid, a 1px line of grid_colour separates the tiles and surrounds the image.
	Image ComposeSheets(const std::vector<std::vector<uint32_t>>& sheets, SheetLayout layout, int scale, bool grid, uint32_t grid_colour);
}
