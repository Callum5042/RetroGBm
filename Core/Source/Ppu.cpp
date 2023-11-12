#include "Pch.h"
#include "Ppu.h"
#include "Cpu.h"
#include "Display.h"
#include "Emulator.h"
#include <algorithm>
#include <chrono>
#include <thread>

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

	m_Context.oam_ram.resize(40);
	std::fill(m_Context.oam_ram.begin(), m_Context.oam_ram.end(), OamData());

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

void Ppu::WriteOam(uint16_t address, uint8_t value)
{
	if (address >= 0xFE00)
	{
		address -= 0xFE00;
	}

	uint8_t* ptr = reinterpret_cast<uint8_t*>(m_Context.oam_ram.data());
	ptr[address] = value;
}

uint8_t Ppu::ReadOam(uint16_t address)
{
	if (address >= 0xFE00)
	{
		address -= 0xFE00;
	}

	uint8_t* ptr = reinterpret_cast<uint8_t*>(m_Context.oam_ram.data());
	return ptr[address];
}

void Ppu::UpdateOam()
{
	// OAM takes 80 ticks
	if (m_Context.dot_ticks >= 80)
	{
		m_Display->SetLcdMode(LcdMode::PixelTransfer);
	}

	// Search and order OAMA per line
	if (m_Context.dot_ticks == 1)
	{
		// Ensure queue is empty
		m_Context.objects.clear();

		// Find all objects on the current scan line
		uint8_t sprite_height = m_Display->GetObjectHeight();
		for (auto& oam : m_Context.oam_ram)
		{
			if ((oam.position_y <= m_Display->context.ly + 16) && (oam.position_y + sprite_height > m_Display->context.ly + 16))
			{
				m_Context.objects.push_back(oam);
			}
		}

		// Sort by priority and X position
		// TODO
	}
}

void Ppu::PixelTransfer()
{
	// Background
	if (m_Display->IsBackgroundEnabled())
	{
		// Divide by 8
		int position_x = (m_PixelX + m_Display->context.scx) & 0xFF;
		int position_y = (m_Display->context.ly + m_Display->context.scy) & 0xFF;

		// Fetch tile
		uint16_t base_address = m_Display->GetBackgroundTileBaseAddress();
		uint8_t tile_id = m_Bus->ReadBus(base_address + (position_x >> 3) + (position_y >> 3) * 32);

		// Check if we are getting tiles from block 1
		if (m_Display->GetBackgroundAndWindowTileData() == 0x8800)
		{
			tile_id += 128;
		}

		// Fetch tile data
		uint16_t offset_x = (tile_id << 4);
		uint16_t offset_y = ((position_y & 0x7) << 1);

		uint16_t tile_address = m_Display->GetBackgroundAndWindowTileData();
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

	// Window
	if (m_Display->IsWindowEnabled())
	{
		if (m_Display->IsWindowInView(m_PixelX))
		{
			// Divide by 8
			int position_x = (m_PixelX - m_Display->context.wx + 7) & 0xFF;
			int position_y = (m_Display->context.ly - m_Display->context.wy) & 0xFF; // (m_WindowLineCounter - m_Display->context.wy) & 0xFF;

			// Fetch tile
			uint16_t base_address = m_Display->GetWindowTileBaseAddress();
			uint8_t tile_id = m_Bus->ReadBus(base_address + (position_x >> 3) + (position_y >> 3) * 32);

			// Check if we are getting tiles from block 1
			if (m_Display->GetBackgroundAndWindowTileData() == 0x8800)
			{
				tile_id += 128;
			}

			// Fetch tile data
			uint16_t offset_x = (tile_id << 4);
			uint16_t offset_y = ((position_y & 0x7) << 1);

			uint16_t tile_address = m_Display->GetBackgroundAndWindowTileData();
			uint8_t byte1 = m_Bus->ReadBus(tile_address + offset_x + offset_y);
			uint8_t byte2 = m_Bus->ReadBus(tile_address + offset_x + offset_y + 1);

			const unsigned long tile_colours[4] = { 0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000 };
			uint8_t high = (static_cast<bool>(byte1 & (1 << (7 - (position_x % 8))))) << 1;
			uint8_t low = (static_cast<bool>(byte2 & (1 << (7 - (position_x % 8))))) << 0;

			uint32_t colour = tile_colours[high | low];
			m_Context.video_buffer[m_PixelX + (m_Display->context.ly * ScreenResolutionX)] = colour;
		}
	}

	// Objects
	if (m_Display->IsObjectEnabled() && !m_Context.objects.empty())
	{
		for (auto& oam : m_Context.objects)
		{
			int current_ly = m_Display->context.ly;
			uint8_t sprite_height = m_Display->GetObjectHeight();

			int position_x = (m_PixelX - oam.position_x + 8); // (oam.position_x - m_PixelX) & 0xFF;
			int sp_x = (oam.position_x - 8) + (m_Display->context.scx % 8);

			// if ((sp_x >= m_PixelX && sp_x < m_PixelX + 8) || ((sp_x + 8) >= m_PixelX && (sp_x + 8) < m_PixelX + 8))
			if (m_PixelX >= oam.position_x - 8 && m_PixelX < oam.position_x)
			{
				// Check Y orientation for which direction to load the pixels
				uint8_t ty = ((current_ly + 16) - oam.position_y) * 2;
				if (oam.flip_y)
				{
					ty = ((sprite_height * 2) - 2) - ty;
				}

				// Check X oreientation
				uint8_t bit = position_x % 8;
				if (!oam.flip_x)
				{
					bit = (7 - (position_x % 8));
				}

				uint8_t tile_index = oam.tile_id;
				if (sprite_height == 16)
				{
					// Remove last bit
					tile_index &= ~(1);
				}

				uint8_t byte1 = m_Bus->ReadBus(0x8000 + (tile_index * 16) + ty + 0);
				uint8_t byte2 = m_Bus->ReadBus(0x8000 + (tile_index * 16) + ty + 1);

				uint8_t high = (static_cast<bool>(byte1 & (1 << bit))) << 1;
				uint8_t low = (static_cast<bool>(byte2 & (1 << bit))) << 0;

				if ((high | low))
				{
					uint32_t colour = (oam.dmg_palette ? m_Display->context.sprite2_palette[high | low] : m_Display->context.sprite1_palette[high | low]);
					m_Context.video_buffer[m_PixelX + (m_Display->context.ly * ScreenResolutionX)] = colour;
				}
			}
		}
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
			m_WindowLineCounter = 0;

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

	// Internal window line counter
	if (m_Display->IsWindowEnabled() && m_Display->IsWindowInView(m_PixelX))
	{
		m_WindowLineCounter++;
	}
}