#pragma once

#include <cstdint>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <array>

class Ppu;
class BaseCartridge;

class IDisplayOutput;

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

	uint8_t priority_mode;
};

struct PaletteData
{
	uint8_t red = 0;
	uint8_t green = 0;
	uint8_t blue = 0;
	uint8_t alpha = 0;
};

enum class LcdMode
{
	HBlank = 0,
	VBlank = 1,
	OAM = 2,
	PixelTransfer = 3,
};

struct FixedPalette
{
	uint32_t bg_colour0;
	uint32_t bg_colour1;
	uint32_t bg_colour2;
	uint32_t bg_colour3;

	uint32_t obj0_colour0;
	uint32_t obj0_colour1;
	uint32_t obj0_colour2;
	uint32_t obj0_colour3;

	uint32_t obj1_colour0;
	uint32_t obj1_colour1;
	uint32_t obj1_colour2;
	uint32_t obj1_colour3;
};

class Display
{
	friend class Ppu;
	BaseCartridge* m_Cartridge = nullptr;
	IDisplayOutput* m_DisplayOutput = nullptr;

public:
	Display();
	Display(BaseCartridge* cartridge, IDisplayOutput* display_output);
	virtual ~Display() = default;

	void Init();

	uint8_t Read(uint16_t address);
	void Write(uint16_t address, uint8_t value);

	bool IsLcdEnabled();
	bool IsWindowEnabled();
	bool IsBackgroundEnabled();
	bool IsObjectEnabled();
	bool IsObjectPriorityModeSet();

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

	void SetObjectPriorityMode(uint8_t value);
	inline uint8_t GetObjectPriorityMode() const { return m_Context.priority_mode; }

	inline DisplayContext* GetContext() { return &m_Context; }

	uint32_t GetColourFromBackgroundPalette(uint8_t palette, uint8_t index);
	uint32_t GetColourFromObjectPalette(uint8_t palette, uint8_t index);

	// Save state
	void SaveState(std::fstream* file);
	void LoadState(std::fstream* file);

	bool IsWindowVisible();

	const uint16_t ScreenResolutionY = 144;
	const uint16_t ScreenResolutionX = 160;

	std::vector<uint8_t> m_BackgroundColourPalettes;

	void* GetVideoBuffer();
	int GetVideoBufferSize();
	void SetVideoBufferPixel(int x, int y, uint32_t data);

private:
	DisplayContext m_Context = {};

	// CGB Palettes
	uint8_t m_BackgroundPaletteIndex = 0;
	bool m_AutoIncrementBackgroundAddress = false;
	uint8_t m_BackgroundPaletteAddress = 0;

	uint8_t m_ObjectPaletteIndex = 0;
	bool m_AutoIncrementObjectAddress = false;
	uint8_t m_ObjectPaletteAddress = 0;
	std::vector<uint8_t> m_ObjectColourPalettes;

	// Default palettes
	std::unordered_map<uint8_t, FixedPalette> m_FixedPalettes;
	void InitFixedPalettes();
	void SetFixedPalette(uint8_t hash);

	// Video buffers
	std::vector<uint32_t> m_VideoBuffer;
	std::vector<uint32_t> m_BlankVideoBuffer;

	void UpdateDisplay();
};