#include "Pch.h"
#include "Display.h"
#include "Cpu.h"
#include "Emulator.h"
#include "Dma.h"

void Display::Init()
{
	// Initialise registers to defaults (after bootrom)
	context.lcdc = 0x91;
	context.stat = 0x85;
	context.scx = 0x0;
	context.scy = 0x0;
	context.dma = 0xFF;
	context.ly = 0x0;
	context.lyc = 0x0;
	context.bgp = 0xFC;
	context.obp[0] = 0xFF;
	context.obp[1] = 0xFF;
	context.wy = 0x0;
	context.wx = 0x0;

	// Set palette colours
	for (int i = 0; i < 4; i++)
	{
		context.background_palette[i] = m_DefaultColours[i];
		context.sprite1_palette[i] = m_DefaultColours[i];
		context.sprite2_palette[i] = m_DefaultColours[i];
	}
}

uint8_t Display::Read(uint16_t address)
{
	switch (address)
	{
		case 0xFF40:
			return context.lcdc;
		case 0xFF41:
			return context.stat;
		case 0xFF42:
			return context.scy;
		case 0xFF43:
			return context.scx;
		case 0xFF44:
			return context.ly;
		case 0xFF45:
			return context.lyc;
		case 0xFF46:
			return context.dma;
		case 0xFF47:
			return context.bgp;
		case 0xFF48:
			return context.obp[0];
		case 0xFF49:
			return context.obp[1];
		case 0xFF4A:
			return context.wy;
		case 0xFF4B:
			return context.wx;
		default:
			return 0xFF;
	}
}

void Display::Write(uint16_t address, uint8_t value)
{
	switch (address)
	{
		case 0xFF40:
			context.lcdc = value;
			break;
		case 0xFF41:
			context.stat = value;
			break;
		case 0xFF42:
			context.scy = value;
			break;
		case 0xFF43:
			context.scx = value;
			break;
		case 0xFF44:
			context.ly = value;
			break;
		case 0xFF45:
			context.lyc = value;
			break;
		case 0xFF46:
			context.dma = value;
			Emulator::Instance->GetDma()->Start(value);
			break;
		case 0xFF47:
			context.bgp = value;
			break;
		case 0xFF48:
			context.obp[0] = value;
			break;
		case 0xFF49:
			context.obp[1] = value;
			break;
		case 0xFF4A:
			context.wy = value;
			break;
		case 0xFF4B:
			context.wx = value;
			break;
	}

	// Update palette colours
	if (address == 0xFF47)
	{
		context.background_palette[0] = m_DefaultColours[(value >> 0) & 0b11];
		context.background_palette[1] = m_DefaultColours[(value >> 2) & 0b11];
		context.background_palette[2] = m_DefaultColours[(value >> 4) & 0b11];
		context.background_palette[3] = m_DefaultColours[(value >> 6) & 0b11];
	}
	else if (address == 0xFF48)
	{
		// Lower two bits are ignored because color index 0 is transparent for OBJs
		value &= 0b11111100;

		context.sprite1_palette[0] = m_DefaultColours[(value >> 0) & 0b11];
		context.sprite1_palette[1] = m_DefaultColours[(value >> 2) & 0b11];
		context.sprite1_palette[2] = m_DefaultColours[(value >> 4) & 0b11];
		context.sprite1_palette[3] = m_DefaultColours[(value >> 6) & 0b11];
	}
	else if (address == 0xFF49)
	{
		// Lower two bits are ignored because color index 0 is transparent for OBJs
		value &= 0b11111100;

		context.sprite2_palette[0] = m_DefaultColours[(value >> 0) & 0b11];
		context.sprite2_palette[1] = m_DefaultColours[(value >> 2) & 0b11];
		context.sprite2_palette[2] = m_DefaultColours[(value >> 4) & 0b11];
		context.sprite2_palette[3] = m_DefaultColours[(value >> 6) & 0b11];
	}
}

bool Display::IsLcdEnabled()
{
	// Check if bit 7 is set
	if (context.lcdc & (1 << 7))
	{
		return true;
	}

	return false;
}

bool Display::IsWindowEnabled()
{
	// Check if bit 5 is set
	if (context.lcdc & (1 << 5))
	{
		return true;
	}

	return false;
}

bool Display::IsBackgroundEnabled()
{
	// Check if bit 0 is set
	if (context.lcdc & (1 << 0))
	{
		return true;
	}

	return false;
}

bool Display::IsObjectEnabled()
{
	// Check if bit 1 is set
	if ((context.lcdc & (1 << 1)))
	{
		return true;
	}

	return false;
}

uint8_t Display::GetObjectHeight()
{
	// Check if bit 2 is set
	if ((context.lcdc & (1 << 2)))
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
	if (context.lcdc & (1 << 3))
	{
		return 0x9C00;
	}

	return 0x9800;
}

uint16_t Display::GetBackgroundTileData()
{
	// Check if bit 4 is set
	if (context.lcdc & (1 << 4))
	{
		return 0x8000;
	}

	return 0x8800;
}

uint16_t Display::GetWindowTileBaseAddress()
{
	// Check if bit 6 is set
	if (context.lcdc & (1 << 6))
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
	return static_cast<LcdMode>(context.stat & 0b11);
}

void Display::SetLcdMode(LcdMode mode)
{
	context.stat &= ~0b11;
	context.stat |= static_cast<uint8_t>(mode);
}

bool Display::IsStatInterruptHBlank()
{
	// Check if bit 3 is set
	if (context.stat & (1 << 3))
	{
		return true;
	}

	return false;
}

bool Display::IsStatInterruptVBlank()
{
	// Check if bit 4 is set
	if (context.stat & (1 << 4))
	{
		return true;
	}

	return false;
}

bool Display::IsStatInterruptOAM()
{
	// Check if bit 5 is set
	if (context.stat & (1 << 5))
	{
		return true;
	}

	return false;
}

bool Display::IsStatInterruptLYC()
{
	// Check if bit 6 is set
	if (context.stat & (1 << 6))
	{
		return true;
	}

	return false;
}