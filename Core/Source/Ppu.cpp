#include "Pch.h"
#include "Ppu.h"
#include "Cpu.h"
#include "Display.h"
#include "Emulator.h"
#include <algorithm>
#include <chrono>
#include <thread>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

Ppu::Ppu()
{
	m_Bus = Emulator::Instance;
	m_Cpu = Emulator::Instance->GetCpu();
	m_Display = Emulator::Instance->GetDisplay();
}

Ppu::Ppu(IBus* bus, Cpu* cpu, Display* display) : m_Bus(bus), m_Cpu(cpu), m_Display(display)
{
}

void Ppu::Init()
{
	m_Context.video_ram.resize(0x8000);
	std::fill(m_Context.video_ram.begin(), m_Context.video_ram.end(), 0);

	m_Context.video_buffer.resize(ScreenResolutionX * ScreenResolutionY * sizeof(uint32_t));
	std::fill(m_Context.video_buffer.begin(), m_Context.video_buffer.end(), 0);



	m_Display->Init();
	m_Display->SetLcdMode(LcdMode::OAM);
}

void Ppu::Tick()
{
	m_Context.dot_ticks++;

	switch (m_Display->GetLcdMode())
	{
		case LcdMode::OAM:
			UpdateOam();
			break;
		case LcdMode::PixelTransfer:
			PixelTransfer();
			break;
		case LcdMode::HBlank:
			HBlank();
			break;
		case LcdMode::VBlank:
			VBlank();
			break;
	}
}

void Ppu::WriteVideoRam(uint16_t address, uint8_t value)
{
	m_Context.video_ram[address - 0x8000] = value;
}

uint8_t Ppu::ReadVideoRam(uint16_t address)
{
	return m_Context.video_ram[address - 0x8000];
}

void Ppu::UpdateOam()
{
	// OAM takes 80 ticks
	if (m_Context.dot_ticks >= 80)
	{
		m_Display->SetLcdMode(LcdMode::PixelTransfer);
	}
}

void Ppu::PixelTransfer()
{
	if (m_Display->IsBackgroundEnabled())
	{
		// Divide by 8
		int position_x = (m_PixelX + m_Display->context.scx) & 0xFF;
		int position_y = (m_Display->context.ly + m_Display->context.scy) & 0xFF;

		// Fetch tile
		uint16_t base_address = m_Display->GetBackgroundTileBaseAddress();
		uint8_t tile_id = m_Bus->ReadBus(base_address + (position_x >> 3) + (position_y >> 3) * 32);

		// Fetch tile data
		uint16_t offset_x = (tile_id << 4);
		uint16_t offset_y = ((position_y & 0x7) << 1);

		uint16_t tile_address = m_Display->GetBackgroundTileData();
		uint8_t byte1 = m_Bus->ReadBus(tile_address + offset_x + offset_y);
		uint8_t byte2 = m_Bus->ReadBus(tile_address + offset_x + offset_y + 1);

		const unsigned long tile_colours[4] = { 0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000 };
		uint8_t high = (static_cast<bool>(byte1 & (1 << (7 - (position_x % 8))))) << 1;
		uint8_t low = (static_cast<bool>(byte2 & (1 << (7 - (position_x % 8))))) << 0;

		uint32_t colour = tile_colours[high | low];
		m_Context.video_buffer[m_PixelX + (m_Display->context.ly * ScreenResolutionX)] = colour;
	}
	else
	{
		m_Context.video_buffer[m_PixelX + (m_Display->context.ly * ScreenResolutionX)] = 0xFFFFFFFF;
	}

	// Check if we are at end of the line
	m_PixelX++;
	if (m_PixelX > ScreenResolutionX)
	{
		m_PixelX = 0;
		IncrementLY();
		m_Display->SetLcdMode(LcdMode::HBlank);
	}
}

void Ppu::HBlank()
{
	// Each line takes 456 ticks
	if (m_Context.dot_ticks >= 456)
	{
		m_Context.dot_ticks = 0;

		if (m_Display->context.ly < 144)
		{
			m_Display->SetLcdMode(LcdMode::OAM);
		}
		else
		{
			m_Cpu->RequestInterrupt(InterruptFlag::VBlank);
			m_Display->SetLcdMode(LcdMode::VBlank);
		}
	}
}

void Ppu::VBlank()
{
	if (m_Context.dot_ticks >= 456)
	{
		m_Context.dot_ticks = 0;
		IncrementLY();

		// 154 total lines in a frame (144 on screen, 10 extra for VBlank mode) (4560 dot ticks)
		if (m_Display->context.ly >= 154)
		{
			m_Display->context.ly = 0;
			m_Display->SetLcdMode(LcdMode::OAM);
		}
	}
}

bool Ppu::CanAccessVRam()
{
	LcdMode mode = m_Display->GetLcdMode();
	return (mode != LcdMode::PixelTransfer);
}

bool Ppu::CanAccessOAM()
{
	LcdMode mode = m_Display->GetLcdMode();
	if (mode == LcdMode::OAM || mode == LcdMode::PixelTransfer)
	{
		return false;
	}

	return true;
}

void Ppu::IncrementLY()
{
	m_Display->context.ly++;

	if (m_Display->context.ly == m_Display->context.lyc)
	{
		m_Display->context.stat |= 0b100;
		m_Cpu->RequestInterrupt(InterruptFlag::STAT);
	}
	else
	{
		m_Display->context.stat &= ~0b100;
	}
}