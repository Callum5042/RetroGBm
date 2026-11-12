#include "RetroGBm/Pch.h"
#include "RetroGBm/TileMap.h"
#include "RetroGBm/TileData.h"

namespace
{
	constexpr int ScreenWidth = 160;
	constexpr int ScreenHeight = 144;

	// Offset of a pixel from the start of the screen along one axis of the wrapping map, in -7 to 248
	int ScreenOffset(int map_position, int scroll)
	{
		int offset = ((map_position - scroll) % 256 + 256) % 256;
		if (offset > 248)
		{
			offset -= 256;
		}

		return offset;
	}
}

uint16_t TileMap::BackgroundMap(uint8_t lcdc)
{
	return (lcdc & 0x08) ? HighMap : LowMap;
}

uint16_t TileMap::WindowMap(uint8_t lcdc)
{
	return (lcdc & 0x40) ? HighMap : LowMap;
}

TileMap::Addressing TileMap::AddressingFromLcdc(uint8_t lcdc)
{
	return (lcdc & 0x10) ? Addressing::Unsigned : Addressing::Signed;
}

int TileMap::TileIndex(uint8_t id, Addressing addressing)
{
	if (addressing == Addressing::Unsigned || id >= 128)
	{
		return id;
	}

	// IDs 0-127 in signed mode are tiles 256-383, from $9000
	return 256 + id;
}

uint16_t TileMap::TileAddress(uint8_t id, Addressing addressing)
{
	return static_cast<uint16_t>(0x8000 + TileIndex(id, addressing) * TileData::BytesPerTile);
}

TileMap::Attributes TileMap::DecodeAttributes(uint8_t value)
{
	Attributes attributes;
	attributes.palette = value & 0x07;
	attributes.bank = (value >> 3) & 1;
	attributes.flip_x = (value & 0x20) != 0;
	attributes.flip_y = (value & 0x40) != 0;
	attributes.priority = (value & 0x80) != 0;
	return attributes;
}

TileMap::Image TileMap::Decode(const uint8_t* ids, const uint8_t* attributes, const uint8_t* bank0, const uint8_t* bank1, Addressing addressing)
{
	Image image;
	image.colours.resize(Width * Height);
	image.palettes.resize(Width * Height);

	for (int entry = 0; entry < Entries; ++entry)
	{
		const Attributes attr = attributes != nullptr ? DecodeAttributes(attributes[entry]) : Attributes();
		const uint8_t* bank = (attr.bank == 1 && bank1 != nullptr) ? bank1 : bank0;
		const std::array<uint8_t, 64> tile = TileData::DecodeTile(bank + TileIndex(ids[entry], addressing) * TileData::BytesPerTile);

		const int origin_x = (entry % Columns) * 8;
		const int origin_y = (entry / Columns) * 8;
		for (int y = 0; y < 8; ++y)
		{
			const int source_y = attr.flip_y ? 7 - y : y;
			const size_t row = static_cast<size_t>(origin_y + y) * Width + origin_x;
			for (int x = 0; x < 8; ++x)
			{
				const int source_x = attr.flip_x ? 7 - x : x;
				image.colours[row + x] = tile[source_y * 8 + source_x];
				image.palettes[row + x] = attr.palette;
			}
		}
	}

	return image;
}

std::vector<TileMap::Rect> TileMap::Wrap(const Rect& rect)
{
	std::vector<Rect> pieces;
	if (rect.IsEmpty())
	{
		return pieces;
	}

	const int width = std::min(rect.width, Width);
	const int height = std::min(rect.height, Height);
	const int x = ((rect.x % Width) + Width) % Width;
	const int y = ((rect.y % Height) + Height) % Height;

	// Split each axis where it crosses the edge
	const int first_width = std::min(width, Width - x);
	const int first_height = std::min(height, Height - y);
	const int spans_x[2][2] = { { x, first_width }, { 0, width - first_width } };
	const int spans_y[2][2] = { { y, first_height }, { 0, height - first_height } };

	for (const auto& span_y : spans_y)
	{
		for (const auto& span_x : spans_x)
		{
			const Rect piece = { span_x[0], span_y[0], span_x[1], span_y[1] };
			if (!piece.IsEmpty())
			{
				pieces.push_back(piece);
			}
		}
	}

	return pieces;
}

TileMap::Rect TileMap::Viewport(uint8_t scx, uint8_t scy)
{
	return { scx, scy, ScreenWidth, ScreenHeight };
}

TileMap::Rect TileMap::WindowOnScreen(uint8_t lcdc, uint8_t wx, uint8_t wy)
{
	// The window starts at WX - 7, WY and runs to the bottom right of the screen
	if ((lcdc & 0x20) == 0 || wx > 166 || wy >= ScreenHeight)
	{
		return {};
	}

	const int left = std::max(wx - 7, 0);
	return { left, wy, ScreenWidth - left, ScreenHeight - wy };
}

TileMap::Rect TileMap::WindowOnMap(uint8_t lcdc, uint8_t wx, uint8_t wy)
{
	const Rect screen = WindowOnScreen(lcdc, wx, wy);
	if (screen.IsEmpty())
	{
		return {};
	}

	// The window map is drawn from its top left. With WX below 7 the first columns are off the left of the screen.
	const int skipped = wx < 7 ? 7 - wx : 0;
	return { skipped, 0, screen.width, screen.height };
}

std::optional<TileMap::Point> TileMap::ScreenPosition(int column, int row, uint8_t scx, uint8_t scy)
{
	const int x = ScreenOffset(column * 8, scx);
	const int y = ScreenOffset(row * 8, scy);
	if (x >= ScreenWidth || y >= ScreenHeight)
	{
		return std::nullopt;
	}

	return Point{ x, y };
}
