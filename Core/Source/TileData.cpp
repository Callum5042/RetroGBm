#include "RetroGBm/Pch.h"
#include "RetroGBm/TileData.h"

std::array<uint8_t, 8> TileData::DecodeRow(uint8_t first, uint8_t second)
{
	std::array<uint8_t, 8> row{};
	for (int x = 0; x < 8; ++x)
	{
		const int bit = 7 - x;
		const uint8_t low = (first >> bit) & 1;
		const uint8_t high = (second >> bit) & 1;
		row[x] = static_cast<uint8_t>((high << 1) | low);
	}

	return row;
}

std::array<uint8_t, 64> TileData::DecodeTile(const uint8_t* bytes)
{
	std::array<uint8_t, 64> tile{};
	for (int y = 0; y < 8; ++y)
	{
		const std::array<uint8_t, 8> row = DecodeRow(bytes[y * 2], bytes[y * 2 + 1]);
		std::copy(row.begin(), row.end(), tile.begin() + y * 8);
	}

	return tile;
}

std::vector<uint8_t> TileData::DecodeSheet(const uint8_t* bytes)
{
	std::vector<uint8_t> sheet(SheetWidth * SheetHeight);
	for (int tile = 0; tile < TilesPerBank; ++tile)
	{
		const int origin_x = (tile % SheetColumns) * 8;
		const int origin_y = (tile / SheetColumns) * 8;

		const std::array<uint8_t, 64> pixels = DecodeTile(bytes + tile * BytesPerTile);
		for (int y = 0; y < 8; ++y)
		{
			std::copy(pixels.begin() + y * 8, pixels.begin() + y * 8 + 8, sheet.begin() + (origin_y + y) * SheetWidth + origin_x);
		}
	}

	return sheet;
}
