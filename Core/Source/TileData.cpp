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

TileData::Image TileData::ComposeSheets(const std::vector<std::vector<uint32_t>>& sheets, SheetLayout layout, int scale, bool grid, uint32_t grid_colour)
{
	Image image;
	const int count = static_cast<int>(sheets.size());
	if (count == 0 || scale < 1)
	{
		return image;
	}

	const bool side_by_side = layout == SheetLayout::SideBySide;
	const int columns = SheetColumns * (side_by_side ? count : 1);
	const int rows = SheetRows * (side_by_side ? 1 : count);

	// Each tile is 8 scaled pixels, plus a line after it when there's a grid
	const int border = grid ? 1 : 0;
	const int step = 8 * scale + border;
	image.width = columns * step + border;
	image.height = rows * step + border;
	image.pixels.assign(static_cast<size_t>(image.width) * image.height, grid ? grid_colour : 0);

	for (int s = 0; s < count; ++s)
	{
		const std::vector<uint32_t>& sheet = sheets[s];
		const int first_column = side_by_side ? s * SheetColumns : 0;
		const int first_row = side_by_side ? 0 : s * SheetRows;

		for (int y = 0; y < SheetHeight; ++y)
		{
			const int out_y = border + (first_row + y / 8) * step + (y % 8) * scale;
			for (int x = 0; x < SheetWidth; ++x)
			{
				const int out_x = border + (first_column + x / 8) * step + (x % 8) * scale;
				const uint32_t colour = sheet[static_cast<size_t>(y) * SheetWidth + x];

				for (int dy = 0; dy < scale; ++dy)
				{
					uint32_t* row = image.pixels.data() + static_cast<size_t>(out_y + dy) * image.width + out_x;
					std::fill(row, row + scale, colour);
				}
			}
		}
	}

	return image;
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
