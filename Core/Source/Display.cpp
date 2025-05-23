#include "RetroGBm/Pch.h"
#include "RetroGBm/Display.h"
#include "RetroGBm/Cpu.h"
#include "RetroGBm/Emulator.h"
#include "RetroGBm/Dma.h"
#include "RetroGBm/Ppu.h"
#include "RetroGBm/Cartridge/BaseCartridge.h"
#include "RetroGBm/Logger.h"


#include <cstdint>
#include <sstream>

namespace
{
	static uint16_t ConvertRGB888ToRGB555(uint32_t rgb888)
	{
		// Extract red, green, and blue components
		uint8_t r = (rgb888 >> 16) & 0xFF;
		uint8_t g = (rgb888 >> 8) & 0xFF;
		uint8_t b = rgb888 & 0xFF;

		// Compress each component from 8 bits to 5 bits
		uint16_t r5 = (r >> 3) & 0x1F;
		uint16_t g5 = (g >> 3) & 0x1F;
		uint16_t b5 = (b >> 3) & 0x1F;

		// Combine components into a 16-bit RGB555 value
		uint16_t rgb555 = (b5 << 10) | (g5 << 5) | r5;
		return rgb555;
	}
}

Display::Display(IDisplayOutput* display_output) : m_DisplayOutput(display_output)
{
	m_Cartridge = Emulator::Instance->GetCartridge();
}

Display::Display(BaseCartridge* cartridge, IDisplayOutput* display_output) : m_Cartridge(cartridge), m_DisplayOutput(display_output)
{
}

void Display::Init()
{
	m_VideoBuffer.resize(ScreenResolutionY * ScreenResolutionX);
	std::fill(m_VideoBuffer.begin(), m_VideoBuffer.end(), 0x0);

	m_BlankVideoBuffer.resize(ScreenResolutionY * ScreenResolutionX);
	std::fill(m_BlankVideoBuffer.begin(), m_BlankVideoBuffer.end(), 0xFFFFFFFF);

	// Initialise registers to defaults (after bootrom)
	m_Context.lcdc = 0x91;
	m_Context.stat = 0x85;
	m_Context.scx = 0x0;
	m_Context.scy = 0x0;
	m_Context.dma = 0xFF;
	m_Context.ly = 0x0;
	m_Context.lyc = 0x0;
	m_Context.bgp = 0xFC;
	m_Context.obp[0] = 0xFF;
	m_Context.obp[1] = 0xFF;
	m_Context.wy = 0x0;
	m_Context.wx = 0x0;

	// CGB palettes
	m_BackgroundColourPalettes.resize(64);
	std::fill(m_BackgroundColourPalettes.begin(), m_BackgroundColourPalettes.end(), 0);

	m_ObjectColourPalettes.resize(64);
	std::fill(m_ObjectColourPalettes.begin(), m_ObjectColourPalettes.end(), 0);

	// Set default for palette 1
	InitFixedPalettes();

	// TODO: This should come from the hash of the game title and select the palettes from the list in the link
	// https://gbdev.io/pandocs/Power_Up_Sequence.html
	// https://tcrf.net/Notes:Game_Boy_Color_Bootstrap_ROM#Assigned_Palette_Configurations

	SetFixedPalette(m_Cartridge->GetTitleChecksum());
}

uint8_t Display::Read(uint16_t address)
{
	switch (address)
	{
		case 0xFF40:
			return m_Context.lcdc;
		case 0xFF41:
		{
			if (!IsLcdEnabled())
			{
				return (m_Context.stat & 0xFC) | 0x80;
			}

			return m_Context.stat | 0x80;
		}
		case 0xFF42:
			return m_Context.scy;
		case 0xFF43:
			return m_Context.scx;
		case 0xFF44:
		{
			if (!IsLcdEnabled())
			{
				return 0;
			}

			return m_Context.ly;
		}
		case 0xFF45:
			return m_Context.lyc;
		case 0xFF46:
			return m_Context.dma;
		case 0xFF47:
			return m_Context.bgp;
		case 0xFF48:
			return m_Context.obp[0];
		case 0xFF49:
			return m_Context.obp[1];
		case 0xFF4A:
			return m_Context.wy;
		case 0xFF4B:
			return m_Context.wx;
		case 0xFF68:
			return m_BackgroundPaletteIndex;
		case 0xFF69:
			return m_BackgroundColourPalettes[m_BackgroundPaletteAddress];
		case 0xFF6A:
			return m_ObjectPaletteIndex;
		case 0xFF6B:
			return m_ObjectColourPalettes[m_ObjectPaletteAddress];
		default:
			return 0xFF;
	}
}

void Display::Write(uint16_t address, uint8_t value)
{
	switch (address)
	{
		case 0xFF40:
			m_Context.lcdc = value;

			if (!IsLcdEnabled())
			{
				m_Context.ly = 0;
				this->SetLcdMode(LcdMode::OAM);
				m_Context.stat |= 0x80;
			}

			return;
		case 0xFF41:
			m_Context.stat &= ~0xF8;
			m_Context.stat |= (value & 0xF8) | 0x80;
			return;
		case 0xFF42:
			m_Context.scy = value;
			return;
		case 0xFF43:
			m_Context.scx = value;
			return;
		case 0xFF44:
			// Ignore writes to LY
			return;
		case 0xFF45:
			m_Context.lyc = value;
			return;
		case 0xFF46:
			m_Context.dma = value;
			Emulator::Instance->GetDma()->Start(value);
			return;
		case 0xFF47:
			m_Context.bgp = value;
			break;
		case 0xFF48:
			m_Context.obp[0] = value;
			break;
		case 0xFF49:
			m_Context.obp[1] = value;
			break;
		case 0xFF4A:
			m_Context.wy = value;
			return;
		case 0xFF4B:
			m_Context.wx = value;
			return;
	}

	// Update palette colours
	if (address == 0xFF47)
	{
		if (!Emulator::Instance->GetCartridge()->IsColourModeDMG())
		{
			return;
		}

		auto hash = Emulator::Instance->GetCartridge()->GetTitleChecksum();
		if (m_FixedPalettes.find(hash) == m_FixedPalettes.end())
		{
			hash = 0x0;
		}

		const uint32_t palettes[] =
		{ 
			m_FixedPalettes[hash].bg_colour0,
			m_FixedPalettes[hash].bg_colour1,
			m_FixedPalettes[hash].bg_colour2,
			m_FixedPalettes[hash].bg_colour3
		};

		uint16_t colour0 = ConvertRGB888ToRGB555(palettes[(value >> 0) & 0b11]);
		m_BackgroundColourPalettes[(0 * 8) + (0 * 2) + 0] = (colour0 & 0xFF);
		m_BackgroundColourPalettes[(0 * 8) + (0 * 2) + 1] = ((colour0 >> 8) & 0xFF);

		uint16_t colour1 = ConvertRGB888ToRGB555(palettes[(value >> 2) & 0b11]);
		m_BackgroundColourPalettes[(0 * 8) + (1 * 2) + 0] = (colour1 & 0xFF);
		m_BackgroundColourPalettes[(0 * 8) + (1 * 2) + 1] = ((colour1 >> 8) & 0xFF);

		uint16_t colour2 = ConvertRGB888ToRGB555(palettes[(value >> 4) & 0b11]);
		m_BackgroundColourPalettes[(0 * 8) + (2 * 2) + 0] = (colour2 & 0xFF);
		m_BackgroundColourPalettes[(0 * 8) + (2 * 2) + 1] = ((colour2 >> 8) & 0xFF);

		uint16_t colour3 = ConvertRGB888ToRGB555(palettes[(value >> 6) & 0b11]);
		m_BackgroundColourPalettes[(0 * 8) + (3 * 2) + 0] = (colour3 & 0xFF);
		m_BackgroundColourPalettes[(0 * 8) + (3 * 2) + 1] = ((colour3 >> 8) & 0xFF);

		return;
	}
	else if (address == 0xFF48)
	{
		if (!Emulator::Instance->GetCartridge()->IsColourModeDMG())
		{
			return;
		}

		uint8_t hash = Emulator::Instance->GetCartridge()->GetTitleChecksum();
		if (m_FixedPalettes.find(hash) == m_FixedPalettes.end())
		{
			hash = 0x0;
		}

		const uint32_t palettes[] =
		{
			m_FixedPalettes[hash].obj0_colour0,
			m_FixedPalettes[hash].obj0_colour1,
			m_FixedPalettes[hash].obj0_colour2,
			m_FixedPalettes[hash].obj0_colour3
		};

		uint16_t colour0 = ConvertRGB888ToRGB555(palettes[(value >> 0) & 0b11]);
		m_ObjectColourPalettes[(0 * 8) + (0 * 2) + 0] = (colour0 & 0xFF);
		m_ObjectColourPalettes[(0 * 8) + (0 * 2) + 1] = ((colour0 >> 8) & 0xFF);

		uint16_t colour1 = ConvertRGB888ToRGB555(palettes[(value >> 2) & 0b11]);
		m_ObjectColourPalettes[(0 * 8) + (1 * 2) + 0] = (colour1 & 0xFF);
		m_ObjectColourPalettes[(0 * 8) + (1 * 2) + 1] = ((colour1 >> 8) & 0xFF);

		uint16_t colour2 = ConvertRGB888ToRGB555(palettes[(value >> 4) & 0b11]);
		m_ObjectColourPalettes[(0 * 8) + (2 * 2) + 0] = (colour2 & 0xFF);
		m_ObjectColourPalettes[(0 * 8) + (2 * 2) + 1] = ((colour2 >> 8) & 0xFF);

		uint16_t colour3 = ConvertRGB888ToRGB555(palettes[(value >> 6) & 0b11]);
		m_ObjectColourPalettes[(0 * 8) + (3 * 2) + 0] = (colour3 & 0xFF);
		m_ObjectColourPalettes[(0 * 8) + (3 * 2) + 1] = ((colour3 >> 8) & 0xFF);

		return;
	}
	else if (address == 0xFF49)
	{
		if (!Emulator::Instance->GetCartridge()->IsColourModeDMG())
		{
			return;
		}

		auto hash = Emulator::Instance->GetCartridge()->GetTitleChecksum();
		if (m_FixedPalettes.find(hash) == m_FixedPalettes.end())
		{
			hash = 0x0;
		}

		const uint32_t palettes[] =
		{
			m_FixedPalettes[hash].obj1_colour0,
			m_FixedPalettes[hash].obj1_colour1,
			m_FixedPalettes[hash].obj1_colour2,
			m_FixedPalettes[hash].obj1_colour3
		};

		uint16_t colour0 = ConvertRGB888ToRGB555(palettes[(value >> 0) & 0b11]);
		m_ObjectColourPalettes[(1 * 8) + (0 * 2) + 0] = (colour0 & 0xFF);
		m_ObjectColourPalettes[(1 * 8) + (0 * 2) + 1] = ((colour0 >> 8) & 0xFF);

		uint16_t colour1 = ConvertRGB888ToRGB555(palettes[(value >> 2) & 0b11]);
		m_ObjectColourPalettes[(1 * 8) + (1 * 2) + 0] = (colour1 & 0xFF);
		m_ObjectColourPalettes[(1 * 8) + (1 * 2) + 1] = ((colour1 >> 8) & 0xFF);

		uint16_t colour2 = ConvertRGB888ToRGB555(palettes[(value >> 4) & 0b11]);
		m_ObjectColourPalettes[(1 * 8) + (2 * 2) + 0] = (colour2 & 0xFF);
		m_ObjectColourPalettes[(1 * 8) + (2 * 2) + 1] = ((colour2 >> 8) & 0xFF);

		uint16_t colour3 = ConvertRGB888ToRGB555(palettes[(value >> 6) & 0b11]);
		m_ObjectColourPalettes[(1 * 8) + (3 * 2) + 0] = (colour3 & 0xFF);
		m_ObjectColourPalettes[(1 * 8) + (3 * 2) + 1] = ((colour3 >> 8) & 0xFF);

		return;
	}

	// CGB Palettes
	if (address == 0xFF68)
	{
		m_BackgroundPaletteIndex = value;
		m_AutoIncrementBackgroundAddress = (value >> 7) & 0x1;
		m_BackgroundPaletteAddress = value & 0x3F;
		return;
	}
	else if (address == 0xFF69)
	{
		m_BackgroundColourPalettes[m_BackgroundPaletteAddress] = value;
		if (m_AutoIncrementBackgroundAddress)
		{
			m_BackgroundPaletteAddress = (m_BackgroundPaletteAddress + 1) & 0x3F;
		}

		return;
	}

	if (address == 0xFF6A)
	{
		m_ObjectPaletteIndex = value;
		m_AutoIncrementObjectAddress = (value >> 7) & 0x1;
		m_ObjectPaletteAddress = value & 0x3F;
		return;
	}
	else if (address == 0xFF6B)
	{
		m_ObjectColourPalettes[m_ObjectPaletteAddress] = value;
		if (m_AutoIncrementObjectAddress)
		{
			m_ObjectPaletteAddress++;
			m_ObjectPaletteIndex = m_ObjectPaletteAddress;
		}

		return;
	}

	std::stringstream ss;
	ss << "Unsupported DisplayWrite: 0x" << std::hex << address;
	Logger::Warning(ss.str());
}

bool Display::IsLcdEnabled()
{
	// Check if bit 7 is set
	if (m_Context.lcdc & (1 << 7))
	{
		return true;
	}

	return false;
}

bool Display::IsWindowEnabled()
{
	// Check if bit 5 is set
	if (m_Context.lcdc & (1 << 5))
	{
		return true;
	}

	return false;
}

bool Display::IsBackgroundEnabled()
{
	// Check if bit 0 is set
	if (m_Context.lcdc & (1 << 0))
	{
		return true;
	}

	return false;
}

bool Display::IsObjectEnabled()
{
	// Check if bit 1 is set
	if ((m_Context.lcdc & (1 << 1)))
	{
		return true;
	}

	return false;
}

uint8_t Display::GetObjectHeight()
{
	// Check if bit 2 is set
	if ((m_Context.lcdc & (1 << 2)))
	{
		// Object is 8x16
		return 16;
	}

	// Object is 8
	return 8;
}

uint16_t Display::GetBackgroundTileBaseAddress()
{
	// Check if bit 3 is set
	if (m_Context.lcdc & (1 << 3))
	{
		return 0x9C00;
	}

	return 0x9800;
}

uint16_t Display::GetBackgroundAndWindowTileData()
{
	// Check if bit 4 is set
	if (m_Context.lcdc & (1 << 4))
	{
		return 0x8000;
	}

	return 0x8800;
}

uint16_t Display::GetWindowTileBaseAddress()
{
	// Check if bit 6 is set
	if (m_Context.lcdc & (1 << 6))
	{
		return 0x9C00;
	}

	return 0x9800;
}

LcdMode Display::GetLcdMode()
{
	// First 2 bits contain the mode
	// Bit 00 = HBlank - Waiting until the end of the scanline
	// Bit 01 = VBlank - Waiting until the next frame
	// Bit 10 = OAM - Searching for OBJs which overlap this line
	// Bit 11 = PixelTransfer - Sending pixels to the LCD
	return static_cast<LcdMode>(m_Context.stat & 0b11);
}

void Display::SetLcdMode(LcdMode mode)
{
	m_Context.stat &= ~0b11;
	m_Context.stat |= static_cast<uint8_t>(mode);
}

bool Display::IsStatInterruptHBlank()
{
	// Check if bit 3 is set
	if (m_Context.stat & (1 << 3))
	{
		return true;
	}

	return false;
}

bool Display::IsStatInterruptVBlank()
{
	// Check if bit 4 is set
	if (m_Context.stat & (1 << 4))
	{
		return true;
	}

	return false;
}

bool Display::IsStatInterruptOAM()
{
	// Check if bit 5 is set
	if (m_Context.stat & (1 << 5))
	{
		return true;
	}

	return false;
}

bool Display::IsStatInterruptLYC()
{
	// Check if bit 6 is set
	if (m_Context.stat & (1 << 6))
	{
		return true;
	}

	return false;
}

void Display::SetObjectPriorityMode(uint8_t value)
{
	m_Context.priority_mode = value & 0x1;
}

bool Display::IsObjectPriorityModeSet()
{
	// 0 = CGB-style priority, 1 = DMG-style priority
	return (m_Context.priority_mode & 0x1);	
}

void Display::SaveState(std::fstream* file)
{
	file->write(reinterpret_cast<const char*>(&m_Context), sizeof(DisplayContext));

	// Background palette data
	file->write(reinterpret_cast<const char*>(&m_AutoIncrementBackgroundAddress), sizeof(m_AutoIncrementBackgroundAddress));
	file->write(reinterpret_cast<const char*>(&m_BackgroundPaletteAddress), sizeof(m_BackgroundPaletteAddress));

	size_t backgroundColourPalettesSize = m_BackgroundColourPalettes.size();
	file->write(reinterpret_cast<const char*>(&backgroundColourPalettesSize), sizeof(backgroundColourPalettesSize));
	file->write(reinterpret_cast<const char*>(m_BackgroundColourPalettes.data()), sizeof(uint8_t) * backgroundColourPalettesSize);

	// Object palette data
	file->write(reinterpret_cast<const char*>(&m_AutoIncrementObjectAddress), sizeof(m_AutoIncrementObjectAddress));
	file->write(reinterpret_cast<const char*>(&m_ObjectPaletteAddress), sizeof(m_ObjectPaletteAddress));

	size_t objectColourPalettes = m_ObjectColourPalettes.size();
	file->write(reinterpret_cast<const char*>(&objectColourPalettes), sizeof(objectColourPalettes));
	file->write(reinterpret_cast<const char*>(m_ObjectColourPalettes.data()), sizeof(uint8_t) * objectColourPalettes);
}

void Display::LoadState(std::fstream* file)
{
	file->read(reinterpret_cast<char*>(&m_Context), sizeof(DisplayContext));

	// Background palette data
	file->read(reinterpret_cast<char*>(&m_AutoIncrementBackgroundAddress), sizeof(m_AutoIncrementBackgroundAddress));
	file->read(reinterpret_cast<char*>(&m_BackgroundPaletteAddress), sizeof(m_BackgroundPaletteAddress));

	size_t backgroundColourPalettesSize = 0;
	file->read(reinterpret_cast<char*>(&backgroundColourPalettesSize), sizeof(backgroundColourPalettesSize));

	m_BackgroundColourPalettes.resize(backgroundColourPalettesSize);
	file->read(reinterpret_cast<char*>(m_BackgroundColourPalettes.data()), sizeof(uint8_t) * backgroundColourPalettesSize);

	// Object palette data
	file->read(reinterpret_cast<char*>(&m_AutoIncrementObjectAddress), sizeof(m_AutoIncrementObjectAddress));
	file->read(reinterpret_cast<char*>(&m_ObjectPaletteAddress), sizeof(m_ObjectPaletteAddress));

	size_t objectColourPalettes = 0;
	file->read(reinterpret_cast<char*>(&objectColourPalettes), sizeof(objectColourPalettes));

	m_ObjectColourPalettes.resize(objectColourPalettes);
	file->read(reinterpret_cast<char*>(m_ObjectColourPalettes.data()), sizeof(uint8_t) * objectColourPalettes);
}

uint32_t Display::GetColourFromBackgroundPalette(uint8_t palette, uint8_t index)
{
	uint8_t byte0 = m_BackgroundColourPalettes[(palette * 8) + (index * 2) + 0];
	uint8_t byte1 = m_BackgroundColourPalettes[(palette * 8) + (index * 2) + 1];
	uint16_t rgb555 = byte0 | (byte1 << 8);

	// Get components
	uint8_t r = (rgb555 >> 10) & 0x1F; // Red component (5 bits)
	uint8_t g = (rgb555 >> 5) & 0x1F;  // Green component (5 bits)
	uint8_t b = rgb555 & 0x1F;         // Blue component (5 bits)

	// Transform from RGB555 to RGB888
	r = (r << 3) | (r >> 2); // (r * 255) / 31;
	g = (g << 3) | (g >> 2); // (g * 255) / 31;
	b = (b << 3) | (b >> 2); // (b * 255) / 31;

	uint32_t colour = (r << 16) | (g << 8) | b;
	return colour;
} 

uint32_t Display::GetColourFromObjectPalette(uint8_t palette, uint8_t index)
{
	uint8_t byte0 = m_ObjectColourPalettes[(palette * 8) + (index * 2) + 0];
	uint8_t byte1 = m_ObjectColourPalettes[(palette * 8) + (index * 2) + 1];
	uint16_t rgb555 = byte0 | (byte1 << 8);

	// Get components
	uint8_t r = (rgb555 >> 10) & 0x1F; // Red component (5 bits)
	uint8_t g = (rgb555 >> 5) & 0x1F;  // Green component (5 bits)
	uint8_t b = rgb555 & 0x1F;         // Blue component (5 bits)

	// Transform from RGB555 to RGB888
	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	uint32_t colour = (r << 16) | (g << 8) | b;
	return colour;
}

void Display::InitFixedPalettes()
{
	// https://tcrf.net/Notes:Game_Boy_Color_Bootstrap_ROM

	m_FixedPalettes[0] =
	{
		0xFFFFFF, 0x7BFF31, 0x0063C5, 0x000000,
		0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000,
		0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000,
	};

	m_FixedPalettes[0x3C] =
	{
		0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000,
		0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000,
		0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000,
	};

	m_FixedPalettes[0x46] =
	{
		0xB5B5FF, 0xFFFF94, 0xAD5A42, 0x000000,
		0xFFFFFF, 0xFFFFFF, 0xFF8484, 0x943A3A,
		0xFFFFFF, 0xFFFFFF, 0xFF8484, 0x943A3A,
	};

	m_FixedPalettes[0x70] =
	{
		0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000,
		0xFFFFFF, 0x00FF00, 0x318400, 0x004A00,
		0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000,
	};

	m_FixedPalettes[0x14] =
	{
		0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000,
		0xFFFFFF, 0x7BFF31, 0x008400, 0x004A00,
		0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000,
	};

	m_FixedPalettes[0xDB] =
	{
		0xFFFFFF, 0xFFFF00, 0xFF0000, 0x000000,
		0xFFFFFF, 0xFFFF00, 0xFF0000, 0x000000,
		0xFFFFFF, 0xFFFF00, 0xFF0000, 0x000000,
	};

	m_FixedPalettes[0x86] =
	{
		0xFFFF9C, 0x94B5FF, 0x639473, 0x003A3A,
		0xFFC542, 0xFFD600, 0x943A00, 0x4A0000,
		0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000,
	};

	m_FixedPalettes[0xA8] =
	{
		0xFFFF9C, 0x94B5FF, 0x639473, 0x003A3A,
		0xFFC542, 0xFFD600, 0x943A00, 0x4A0000,
		0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000,
	};

	m_FixedPalettes[0xC6] =
	{
		0xFFFFFF, 0xADAD84, 0x42737B, 0x000000,
		0xFFFFFF, 0xFF7300, 0x944200, 0x000000,
		0xFFFFFF, 0x5ABDFF, 0xFF0000, 0x0000FF,
	};
}

void Display::SetFixedPalette(uint8_t hash)
{
	// Check if we have a fixed colour palette to select from otherwise default to 0
	if (m_FixedPalettes.find(hash) == m_FixedPalettes.end())
	{
		hash = 0x0;
	}

	uint8_t palette = 0;

	// Background
	{
		uint16_t colour0 = ConvertRGB888ToRGB555(m_FixedPalettes[hash].bg_colour0);
		m_BackgroundColourPalettes[(palette * 8) + (0 * 2) + 0] = (colour0 & 0xFF);
		m_BackgroundColourPalettes[(palette * 8) + (0 * 2) + 1] = ((colour0 >> 8) & 0xFF);

		uint16_t colour1 = ConvertRGB888ToRGB555(m_FixedPalettes[hash].bg_colour1);
		m_BackgroundColourPalettes[(palette * 8) + (1 * 2) + 0] = (colour1 & 0xFF);
		m_BackgroundColourPalettes[(palette * 8) + (1 * 2) + 1] = ((colour1 >> 8) & 0xFF);

		uint16_t colour2 = ConvertRGB888ToRGB555(m_FixedPalettes[hash].bg_colour2);
		m_BackgroundColourPalettes[(palette * 8) + (2 * 2) + 0] = (colour2 & 0xFF);
		m_BackgroundColourPalettes[(palette * 8) + (2 * 2) + 1] = ((colour2 >> 8) & 0xFF);

		uint16_t colour3 = ConvertRGB888ToRGB555(m_FixedPalettes[hash].bg_colour3);
		m_BackgroundColourPalettes[(palette * 8) + (3 * 2) + 0] = (colour3 & 0xFF);
		m_BackgroundColourPalettes[(palette * 8) + (3 * 2) + 1] = ((colour3 >> 8) & 0xFF);
	}

	// Object 0
	{
		uint16_t colour0 = ConvertRGB888ToRGB555(m_FixedPalettes[hash].obj0_colour0);
		m_ObjectColourPalettes[(palette * 8) + (0 * 2) + 0] = (colour0 & 0xFF);
		m_ObjectColourPalettes[(palette * 8) + (0 * 2) + 1] = ((colour0 >> 8) & 0xFF);

		uint16_t colour1 = ConvertRGB888ToRGB555(m_FixedPalettes[hash].obj0_colour1);
		m_ObjectColourPalettes[(palette * 8) + (1 * 2) + 0] = (colour1 & 0xFF);
		m_ObjectColourPalettes[(palette * 8) + (1 * 2) + 1] = ((colour1 >> 8) & 0xFF);

		uint16_t colour2 = ConvertRGB888ToRGB555(m_FixedPalettes[hash].obj0_colour2);
		m_ObjectColourPalettes[(palette * 8) + (2 * 2) + 0] = (colour2 & 0xFF);
		m_ObjectColourPalettes[(palette * 8) + (2 * 2) + 1] = ((colour2 >> 8) & 0xFF);

		uint16_t colour3 = ConvertRGB888ToRGB555(m_FixedPalettes[hash].obj0_colour3);
		m_ObjectColourPalettes[(palette * 8) + (3 * 2) + 0] = (colour3 & 0xFF);
		m_ObjectColourPalettes[(palette * 8) + (3 * 2) + 1] = ((colour3 >> 8) & 0xFF);
	}

	// Object 1
	{
		palette = 1;

		uint16_t colour0 = ConvertRGB888ToRGB555(m_FixedPalettes[hash].obj1_colour0);
		m_ObjectColourPalettes[(palette * 8) + (0 * 2) + 0] = (colour0 & 0xFF);
		m_ObjectColourPalettes[(palette * 8) + (0 * 2) + 1] = ((colour0 >> 8) & 0xFF);

		uint16_t colour1 = ConvertRGB888ToRGB555(m_FixedPalettes[hash].obj1_colour1);
		m_ObjectColourPalettes[(palette * 8) + (1 * 2) + 0] = (colour1 & 0xFF);
		m_ObjectColourPalettes[(palette * 8) + (1 * 2) + 1] = ((colour1 >> 8) & 0xFF);

		uint16_t colour2 = ConvertRGB888ToRGB555(m_FixedPalettes[hash].obj1_colour2);
		m_ObjectColourPalettes[(palette * 8) + (2 * 2) + 0] = (colour2 & 0xFF);
		m_ObjectColourPalettes[(palette * 8) + (2 * 2) + 1] = ((colour2 >> 8) & 0xFF);

		uint16_t colour3 = ConvertRGB888ToRGB555(m_FixedPalettes[hash].obj1_colour3);
		m_ObjectColourPalettes[(palette * 8) + (3 * 2) + 0] = (colour3 & 0xFF);
		m_ObjectColourPalettes[(palette * 8) + (3 * 2) + 1] = ((colour3 >> 8) & 0xFF);
	}
}

bool Display::IsWindowVisible()
{
	return this->IsWindowEnabled()
		&& this->m_Context.wx >= 0
		&& this->m_Context.wx <= 166
		&& this->m_Context.wy >= 0
		&& this->m_Context.wy < ScreenResolutionY;
}

void* Display::GetVideoBuffer()
{
	if (this->IsLcdEnabled())
	{
		return m_VideoBuffer.data();
	}
	else
	{
		return m_BlankVideoBuffer.data();
	}
}

int Display::GetVideoBufferSize()
{
	return static_cast<int>(m_VideoBuffer.size());
}

void Display::SetVideoBufferPixel(int x, int y, uint32_t data)
{
	int offset = x + (y * ScreenResolutionX);
	m_VideoBuffer[offset] = data;
}

void Display::UpdateDisplay()
{
	int video_pitch = sizeof(uint32_t)* ScreenResolutionX;
	if (this->IsLcdEnabled())
	{
        auto data = m_VideoBuffer.data();
		m_DisplayOutput->Draw(data, video_pitch);
	}
	else
	{
		m_DisplayOutput->Draw(m_BlankVideoBuffer.data(), video_pitch);
	}
}