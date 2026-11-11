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
}
