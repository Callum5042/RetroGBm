#pragma once

#include <array>
#include <cstdint>
#include <vector>

// Decoding of the 40 objects (sprites) in OAM at $FE00-$FE9F, and of which ones the PPU draws on each line.
// https://gbdev.io/pandocs/OAM.html
namespace Oam
{
	constexpr int Count = 40;
	constexpr int BytesPerObject = 4;
	constexpr int Size = Count * BytesPerObject;
	constexpr uint16_t Address = 0xFE00;

	// The PPU picks at most this many objects for each line
	constexpr int ObjectsPerLine = 10;

	constexpr int ScreenWidth = 160;
	constexpr int ScreenHeight = 144;

	// Attribute byte. CGB uses the bank and the 3-bit palette, DMG the 1-bit palette (OBP0 or OBP1).
	// https://gbdev.io/pandocs/OAM.html#byte-3--attributesflags
	struct Attributes
	{
		bool priority = false;
		bool flip_y = false;
		bool flip_x = false;
		uint8_t dmg_palette = 0;
		uint8_t bank = 0;
		uint8_t cgb_palette = 0;
	};

	Attributes DecodeAttributes(uint8_t value);

	struct Object
	{
		uint8_t y = 0;
		uint8_t x = 0;
		uint8_t tile = 0;
		uint8_t flags = 0;

		// Top-left on screen, which is off the top or left edge for small values of Y or X
		int ScreenX() const { return static_cast<int>(x) - 8; }
		int ScreenY() const { return static_cast<int>(y) - 16; }
	};

	// Object index (0-39) from the Size bytes of OAM
	Object Decode(const uint8_t* oam, int index);

	// Address of the first of the object's four bytes
	uint16_t ObjectAddress(int index);

	// LCDC.2 picks 8 x 16 objects; otherwise they are 8 x 8
	int HeightFromLcdc(uint8_t lcdc);

	// The tile at the top of the object. 8 x 16 objects ignore bit 0 and use that tile and the next.
	uint8_t FirstTile(uint8_t tile, int height);

	// Address in VRAM of the first tile's data. Objects always count tiles up from $8000.
	uint16_t TileAddress(uint8_t tile, int height);

	// Whether the object covers any line of the screen, so the PPU counts it towards that line's limit
	bool IsOnLines(const Object& object, int height);

	// Whether any of the object can be seen: it's on a line and X is 1-167
	bool IsVisible(const Object& object, int height);

	// The object as 8 x height colour indices (0-3, 0 being transparent), row by row, with any flips applied.
	// bank0 and bank1 are TileData::BytesPerBank bytes from $8000; bank1 may be null on DMG, where the bank bit is ignored.
	std::vector<uint8_t> DecodeImage(const Object& object, int height, const uint8_t* bank0, const uint8_t* bank1);

	// Which objects the PPU draws on each line, found the way its OAM scan does: the first ObjectsPerLine
	// objects in OAM order that cover the line, whatever their X.
	struct LineScan
	{
		// How many objects cover each line, including those past the limit
		std::array<uint8_t, ScreenHeight> objects = {};

		// For each object, the lines it covers but where the limit was already reached, in order
		std::array<std::vector<uint8_t>, Count> dropped;

		// Whether a line has more than ObjectsPerLine objects, so some are dropped
		bool IsOverLimit(int line) const { return objects[line] > ObjectsPerLine; }
	};

	LineScan ScanLines(const uint8_t* oam, int height);
}
