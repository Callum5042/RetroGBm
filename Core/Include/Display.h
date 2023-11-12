#pragma once

#include <cstdint>

struct DisplayContext
{
	// Registers
	uint8_t lcdc;
	uint8_t stat;
	uint8_t scy;
	uint8_t scx;
	uint8_t ly;
	uint8_t lyc;
	uint8_t dma;
	uint8_t bgp;
	uint8_t obp[2];
	uint8_t wy;
	uint8_t wx;

	// Other data
	uint32_t background_palette[4];
	uint32_t sprite1_palette[4];
	uint32_t sprite2_palette[4];
};

enum class LcdMode
{
	HBlank = 0,
	VBlank = 1,
	OAM = 2,
	PixelTransfer = 3,
};

class Display
{
public:
	Display() = default;
	virtual ~Display() = default;

	void Init();

	uint8_t Read(uint16_t address);
	void Write(uint16_t address, uint8_t value);

	bool IsLcdEnabled();
	bool IsWindowEnabled();
	bool IsBackgroundEnabled();
	bool IsObjectEnabled();

	uint8_t GetObjectHeight();
	uint16_t GetBackgroundTileBaseAddress();
	uint16_t GetBackgroundAndWindowTileData();
	uint16_t GetWindowTileBaseAddress();

	bool IsStatInterruptHBlank();
	bool IsStatInterruptVBlank();
	bool IsStatInterruptOAM();
	bool IsStatInterruptLYC();

	void SetLcdMode(LcdMode mode);
	LcdMode GetLcdMode();

	DisplayContext context = {};

private:
	const unsigned long m_DefaultColours[4] = { 0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000 };
};