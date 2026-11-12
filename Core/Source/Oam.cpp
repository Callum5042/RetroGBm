#include "RetroGBm/Pch.h"
#include "RetroGBm/Oam.h"
#include "RetroGBm/TileData.h"

#include <algorithm>

Oam::Attributes Oam::DecodeAttributes(uint8_t value)
{
	Attributes attributes;
	attributes.priority = (value & 0x80) != 0;
	attributes.flip_y = (value & 0x40) != 0;
	attributes.flip_x = (value & 0x20) != 0;
	attributes.dmg_palette = (value >> 4) & 1;
	attributes.bank = (value >> 3) & 1;
	attributes.cgb_palette = value & 0x07;
	return attributes;
}

Oam::Object Oam::Decode(const uint8_t* oam, int index)
{
	const uint8_t* bytes = oam + index * BytesPerObject;
	return Object{ bytes[0], bytes[1], bytes[2], bytes[3] };
}

uint16_t Oam::ObjectAddress(int index)
{
	return static_cast<uint16_t>(Address + index * BytesPerObject);
}

int Oam::HeightFromLcdc(uint8_t lcdc)
{
	return (lcdc & 0x04) ? 16 : 8;
}

uint8_t Oam::FirstTile(uint8_t tile, int height)
{
	return height == 16 ? static_cast<uint8_t>(tile & 0xFE) : tile;
}

uint16_t Oam::TileAddress(uint8_t tile, int height)
{
	return static_cast<uint16_t>(0x8000 + FirstTile(tile, height) * TileData::BytesPerTile);
}

bool Oam::IsOnLines(const Object& object, int height)
{
	const int top = object.ScreenY();
	return top + height > 0 && top < ScreenHeight;
}

bool Oam::IsVisible(const Object& object, int height)
{
	return IsOnLines(object, height) && object.x > 0 && object.x < ScreenWidth + 8;
}

std::vector<uint8_t> Oam::DecodeImage(const Object& object, int height, const uint8_t* bank0, const uint8_t* bank1)
{
	const Attributes attributes = DecodeAttributes(object.flags);
	const uint8_t* bank = (attributes.bank == 1 && bank1 != nullptr) ? bank1 : bank0;
	const int first = FirstTile(object.tile, height);

	// Unflipped, one tile above the other
	std::vector<uint8_t> source(8 * height);
	for (int part = 0; part < height / 8; ++part)
	{
		const std::array<uint8_t, 64> tile = TileData::DecodeTile(bank + (first + part) * TileData::BytesPerTile);
		std::copy(tile.begin(), tile.end(), source.begin() + part * 64);
	}

	// An 8 x 16 object flips as a whole, so its tiles swap places when flipped vertically
	std::vector<uint8_t> image(8 * height);
	for (int y = 0; y < height; ++y)
	{
		const int source_y = attributes.flip_y ? height - 1 - y : y;
		for (int x = 0; x < 8; ++x)
		{
			const int source_x = attributes.flip_x ? 7 - x : x;
			image[y * 8 + x] = source[source_y * 8 + source_x];
		}
	}

	return image;
}

Oam::LineScan Oam::ScanLines(const uint8_t* oam, int height)
{
	LineScan scan;
	std::array<int, ScreenHeight> picked = {};

	for (int index = 0; index < Count; ++index)
	{
		const Object object = Decode(oam, index);
		const int top = std::max(0, object.ScreenY());
		const int bottom = std::min(ScreenHeight, object.ScreenY() + height);

		for (int line = top; line < bottom; ++line)
		{
			++scan.objects[line];

			if (picked[line] < ObjectsPerLine)
			{
				++picked[line];
			}
			else
			{
				scan.dropped[index].push_back(static_cast<uint8_t>(line));
			}
		}
	}

	return scan;
}
