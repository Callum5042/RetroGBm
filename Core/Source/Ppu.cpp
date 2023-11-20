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
	m_Context.video_buffer.resize(ScreenResolutionY * ScreenResolutionX);
	std::fill(m_Context.video_buffer.begin(), m_Context.video_buffer.end(), 0x0);

	m_Context.video_ram.resize(16384);
	std::fill(m_Context.video_ram.begin(), m_Context.video_ram.end(), 0x0);

	m_Display->Init();
	m_Display->SetLcdMode(LcdMode::OAM);

	m_Timer.Start();
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

void Ppu::WriteVideoRam(uint16_t address, uint8_t value)
{
	m_Context.video_ram[(address - 0x8000) + (m_VramBank * 8192)] = value;
}

uint8_t Ppu::ReadVideoRam(uint16_t address)
{
	return m_Context.video_ram[(address - 0x8000) + (m_VramBank * 8192)];
}

void Ppu::SetVideoRamBank(uint8_t value)
{
	m_VramBank = value & 0b1;
}

void Ppu::UpdateOam()
{
	// Searching for objects takes 80 ticks
	if (m_Context.dot_ticks >= 80)
	{
		m_Context.pipeline = PipelineContext();
		m_Display->SetLcdMode(LcdMode::PixelTransfer);
	}

	// Search and order OAMA per line
	if (m_Context.dot_ticks == 1)
	{
		m_Context.objects_per_line.clear();

		// Find all objects on the current scan line
		uint8_t sprite_height = m_Display->GetObjectHeight();
		for (auto& oam : m_Context.oam_ram)
		{
			if ((oam.position_y + sprite_height > m_Display->m_Context.ly + 16) && (oam.position_y <= m_Display->m_Context.ly + 16))
			{
				m_Context.objects_per_line.push_back(oam);
			}
		}

		// Sort by priority and X position
		std::sort(m_Context.objects_per_line.begin(), m_Context.objects_per_line.end(), [](const OamData& lhs, const OamData& rhs)
		{
			return (lhs.position_x < rhs.position_x);
		});

		// Limit to 10 per row
		if (m_Context.objects_per_line.size() > 10)
		{
			m_Context.objects_per_line.erase(m_Context.objects_per_line.begin() + 10, m_Context.objects_per_line.end());
		}
	}
}

void Ppu::PixelTransfer()
{
	PipelineProcess();

	// Process pixels until we finish the line
	if (m_Context.pipeline.pushed_x >= ScreenResolutionX)
	{
		m_Display->SetLcdMode(LcdMode::HBlank);
		if (m_Display->IsStatInterruptHBlank())
		{
			m_Cpu->RequestInterrupt(InterruptFlag::STAT);
		}
	}
}

void Ppu::VBlank()
{
	if (m_Context.dot_ticks >= m_DotTicksPerLine)
	{
		m_Context.dot_ticks = 0;
		IncrementLY();

		// Keep increasing LY register until we reach the lines per frame
		if (m_Display->m_Context.ly >= m_LinesPerFrame)
		{
			LimitFrameRate();

			m_Display->SetLcdMode(LcdMode::OAM);
			m_Display->m_Context.ly = 0;
			m_Context.window_line_counter = 0;
		}
	}
}

void Ppu::HBlank()
{
	if (m_Context.dot_ticks >= m_DotTicksPerLine)
	{
		m_Context.dot_ticks = 0;
		IncrementLY();

		// Enter VBlank if all the scanlines have been drawn
		if (m_Display->m_Context.ly >= ScreenResolutionY)
		{
			m_Display->SetLcdMode(LcdMode::VBlank);
			m_Cpu->RequestInterrupt(InterruptFlag::VBlank);

			if (m_Display->IsStatInterruptVBlank())
			{
				m_Cpu->RequestInterrupt(InterruptFlag::STAT);
			}
		}
		else
		{
			m_Display->SetLcdMode(LcdMode::OAM);
		}
	}
}

void Ppu::PipelineProcess()
{
	if ((m_Context.dot_ticks & 1))
	{
		PixelFetcher();
	}

	PushPixelToVideoBuffer();
}

bool Ppu::IsWindowVisible()
{
	return m_Display->IsWindowEnabled()
		&& m_Display->m_Context.wx >= 0
		&& m_Display->m_Context.wx <= 166
		&& m_Display->m_Context.wy >= 0
		&& m_Display->m_Context.wy < ScreenResolutionY;
}

void Ppu::IncrementLY()
{
	// Increment LY register every 
	m_Display->m_Context.ly++;
	if (m_Display->m_Context.ly == m_Display->m_Context.lyc)
	{
		m_Display->m_Context.stat |= 0b100;
		if (m_Display->IsStatInterruptLYC())
		{
			m_Cpu->RequestInterrupt(InterruptFlag::STAT);
		}
	}
	else
	{
		m_Display->m_Context.stat &= ~0b100;
	}

	// Internal window line is used for the window tiles Y offset and only incremented when the window is visible
	if (IsWindowVisible() && (m_Display->m_Context.ly > m_Display->m_Context.wy) && (m_Display->m_Context.ly <= m_Display->m_Context.wy + ScreenResolutionY))
	{
		m_Context.window_line_counter++;
	}
}

uint32_t Ppu::FetchSpritePixels(uint32_t color, bool background_pixel_transparent)
{
	for (int i = 0; i < m_Context.pipeline.fetched_entries.size(); i++)
	{
		// Background has priority and there is already a background pixel
		bool priority = m_Context.pipeline.fetched_entries[i].oam->priority;
		if (priority && !background_pixel_transparent)
		{
			continue;
		}

		// Past pixel point already
		int sprite_x = (m_Context.pipeline.fetched_entries[i].oam->position_x - 8) + ((m_Display->m_Context.scx % 8));
		if (sprite_x + 8 < m_Context.pipeline.fifo_x)
		{
			continue;
		}

		int offset = m_Context.pipeline.fifo_x - sprite_x;

		uint8_t bit = (7 - offset);
		if (m_Context.pipeline.fetched_entries[i].oam->flip_x)
		{
			bit = offset;
		}

		uint8_t high = (static_cast<bool>(m_Context.pipeline.fetched_entries[i].byte_low & (1 << bit))) << 0;
		uint8_t low = (static_cast<bool>(m_Context.pipeline.fetched_entries[i].byte_high & (1 << bit))) << 1;
		uint8_t palette_index = high | low;

		// Transparent
		if (palette_index == 0)
		{
			continue;
		}

		// Select pixel colour
		return (m_Context.pipeline.fetched_entries[i].oam->dmg_palette) ? m_Display->m_Context.sprite2_palette[palette_index] : m_Display->m_Context.sprite1_palette[palette_index];
	}

	return color;
}

bool Ppu::PipelineAddPixel()
{
	// Check if the queue is full - max of 8 pixels
	if (m_Context.pipeline.pixel_queue.size() > 8)
	{
		return false;
	}

	// Discard pixels that are not on the screen
	int pixel_x = m_Context.pipeline.fetch_x - (8 - (m_Display->m_Context.scx % 8));
	if (pixel_x < 0)
	{
		return false;
	}

	for (int bit = 7; bit >= 0; bit--)
	{
		// Decode and get pixel colour from palette
		uint8_t data_high = (static_cast<bool>(m_Context.pipeline.background_window_byte_low & (1 << bit))) << 0;
		uint8_t data_low = (static_cast<bool>(m_Context.pipeline.background_window_byte_high & (1 << bit))) << 1;
		uint8_t palette_index = data_high | data_low;

		uint32_t colour = m_Display->m_Context.background_palette[palette_index];

		if (!m_Display->IsBackgroundEnabled())
		{
			colour = m_Display->m_Context.background_palette[0];
		}

		if (m_Display->IsObjectEnabled())
		{
			bool background_transparent = (palette_index == 0);
			colour = FetchSpritePixels(colour, background_transparent);
		}

		m_Context.pipeline.pixel_queue.push(colour);
		m_Context.pipeline.fifo_x++;
	}

	return true;
}

void Ppu::LoadSpriteTile()
{
	for (auto& oam : m_Context.objects_per_line)
	{
		int sp_x = (oam.position_x - 8) + (m_Display->m_Context.scx % 8);

		if ((sp_x >= m_Context.pipeline.fetch_x && sp_x < m_Context.pipeline.fetch_x + 8) || ((sp_x + 8) >= m_Context.pipeline.fetch_x && (sp_x + 8) < m_Context.pipeline.fetch_x + 8))
		{
			OamPipelineData data;
			data.oam = &oam;

			m_Context.pipeline.fetched_entries.push_back(data);
		}

		// Max checking 3 sprites on pixels
		if (m_Context.pipeline.fetched_entries.size() >= 3)
		{
			break;
		}
	}
}

void Ppu::LoadSpriteData(FetchTileByte tile_byte)
{
	int current_ly = m_Display->m_Context.ly;
	uint8_t sprite_height = m_Display->GetObjectHeight();

	for (int i = 0; i < m_Context.pipeline.fetched_entries.size(); i++)
	{
		// Check Y orientation for which direction to load the pixels
		uint8_t tile_y = ((current_ly + 16) - m_Context.pipeline.fetched_entries[i].oam->position_y) * 2;
		if (m_Context.pipeline.fetched_entries[i].oam->flip_y)
		{
			tile_y = ((sprite_height * 2) - 2) - tile_y;
		}

		uint8_t tile_index = m_Context.pipeline.fetched_entries[i].oam->tile_id;
		if (sprite_height == 16)
		{
			// Remove last bit
			tile_index &= ~(1);
		}

		if (tile_byte == FetchTileByte::ByteLow)
		{
			m_Context.pipeline.fetched_entries[i].byte_low = m_Bus->ReadBus(0x8000 + (tile_index * 16) + tile_y + 0);
		}
		else if (tile_byte == FetchTileByte::ByteHigh)
		{
			m_Context.pipeline.fetched_entries[i].byte_high = m_Bus->ReadBus(0x8000 + (tile_index * 16) + tile_y + 1);
		}
	}
}

void Ppu::LoadWindowTile()
{
	if (IsWindowVisible())
	{
		if (IsWindowInView(m_Context.pipeline.fetch_x))
		{
			// Divide by 8
			uint8_t position_x = (m_Context.pipeline.fetch_x - m_Display->m_Context.wx + 7) & 0xFF;
			uint8_t position_y = m_Context.window_line_counter; // (m_Display->m_Context.ly - m_Display->m_Context.wy) & 0xFF;

			// Fetch tile
			uint16_t base_address = m_Display->GetWindowTileBaseAddress();
			m_Context.pipeline.background_window_tile = m_Bus->ReadBus(base_address + (position_x >> 3) + ((position_y >> 3) * 32));

			// Check if we are getting tiles from block 1
			if (m_Display->GetBackgroundAndWindowTileData() == 0x8800)
			{
				m_Context.pipeline.background_window_tile += 128;
			}
		}
	}
}

void Ppu::PixelFetcher()
{
	switch (m_Context.pipeline.pipeline_state)
	{
		case FetchState::Tile:
		{
			m_Context.pipeline.fetched_entries.clear();

			// Load background/window tile
			if (m_Display->IsBackgroundEnabled())
			{
				// Divide by 8
				int position_y = (m_Display->m_Context.ly + m_Display->m_Context.scy) & 0xFF;
				int position_x = (m_Context.pipeline.fetch_x + m_Display->m_Context.scx) & 0xFF;

				// Fetch tile
				uint16_t base_address = m_Display->GetBackgroundTileBaseAddress();
				m_Context.pipeline.background_window_tile = m_Bus->ReadBus(base_address + (position_x >> 3) + (position_y >> 3) * 32);

				if (m_Display->GetBackgroundAndWindowTileData() == 0x8800)
				{
					m_Context.pipeline.background_window_tile += 128;
				}

				// Load window tile
				LoadWindowTile();
			}

			// Load sprite tile
			if (m_Display->IsObjectEnabled())
			{
				LoadSpriteTile();
			}

			m_Context.pipeline.pipeline_state = FetchState::TileDataLow;
			m_Context.pipeline.fetch_x += 8;
			break;
		}

		case FetchState::TileDataLow:
		{
			FetchTileData(FetchTileByte::ByteLow);
			LoadSpriteData(FetchTileByte::ByteLow);

			m_Context.pipeline.pipeline_state = FetchState::TileDataHigh;
			break;
		}

		case FetchState::TileDataHigh:
		{
			FetchTileData(FetchTileByte::ByteHigh);
			LoadSpriteData(FetchTileByte::ByteHigh);

			m_Context.pipeline.pipeline_state = FetchState::Idle;
			break;
		}

		case FetchState::Idle:
		{
			m_Context.pipeline.pipeline_state = FetchState::Push;
			break;
		}

		case FetchState::Push:
		{
			if (PipelineAddPixel())
			{
				m_Context.pipeline.pipeline_state = FetchState::Tile;
			}

			break;
		}
	}
}

void Ppu::PushPixelToVideoBuffer()
{
	if (m_Context.pipeline.pixel_queue.size() > 8)
	{
		uint32_t pixel_data = m_Context.pipeline.pixel_queue.front();
		m_Context.pipeline.pixel_queue.pop();

		if (m_Context.pipeline.line_x >= (m_Display->m_Context.scx % 8))
		{
			m_Context.video_buffer[m_Context.pipeline.pushed_x + (m_Display->m_Context.ly * ScreenResolutionX)] = pixel_data;
			m_Context.pipeline.pushed_x++;
		}

		m_Context.pipeline.line_x++;
	}
}

bool Ppu::IsWindowInView(int pixel_x)
{
	const int ScreenResolutionX = 160;
	const int ScreenResolutionY = 144;

	if (m_Display->m_Context.ly >= m_Display->m_Context.wy && m_Display->m_Context.ly < m_Display->m_Context.wy + ScreenResolutionY)
	{
		if ((pixel_x >= m_Display->m_Context.wx - 7) && (pixel_x < m_Display->m_Context.wx + ScreenResolutionX - 7))
		{
			return true;
		}
	}

	return false;
}

void Ppu::FetchTileData(FetchTileByte tile_byte)
{
	// Calculate tile offset
	uint16_t offset_x = (m_Context.pipeline.background_window_tile << 4);
	uint16_t offset_y = ((m_Display->m_Context.ly + m_Display->m_Context.scy) % 8) * 2;

	if (IsWindowVisible() && IsWindowInView(m_Context.pipeline.fetch_x))
	{
		offset_y = ((m_Context.window_line_counter & 0x7) << 1);
	}

	// Fetch tile data
	uint16_t base_address = m_Display->GetBackgroundAndWindowTileData();
	if (tile_byte == FetchTileByte::ByteLow)
	{
		m_Context.pipeline.background_window_byte_low = m_Bus->ReadBus(base_address + offset_x + offset_y);
	}
	else if (tile_byte == FetchTileByte::ByteHigh)
	{
		m_Context.pipeline.background_window_byte_high = m_Bus->ReadBus(base_address + offset_x + offset_y + 1);
	}
}

void Ppu::LimitFrameRate()
{
	m_Timer.Tick();
	m_FrameCount++;

	// Compute averages over one second period
	if ((m_Timer.TotalTime() - m_TimeElapsed) >= 1.0f)
	{
		m_FramesPerSecond = m_FrameCount;
		float mspf = 1000.0f / m_FramesPerSecond;

		// Set total frame count
		m_TotalFrames += m_FrameCount;

		// Reset for next average.
		m_FrameCount = 0;
		m_TimeElapsed += 1.0f;
	}

	// Limit framerate to match target rate
	if (m_Timer.DeltaTime() < m_TargetFrameTime)
	{
		const std::chrono::duration<double, std::milli> elapsed(m_TargetFrameTime - m_Timer.DeltaTime());
		std::this_thread::sleep_for(elapsed);
	}
}

void Ppu::SaveState(std::fstream* file)
{
	size_t videoram_size = m_Context.video_ram.size();
	file->write(reinterpret_cast<const char*>(&videoram_size), sizeof(size_t));
	file->write(reinterpret_cast<const char*>(m_Context.video_ram.data()), videoram_size * sizeof(uint8_t));
}

void Ppu::LoadState(std::fstream* file)
{
	size_t videoram_size = 0;
	file->read(reinterpret_cast<char*>(&videoram_size), sizeof(size_t));

	m_Context.video_ram.resize(videoram_size);
	file->read(reinterpret_cast<char*>(m_Context.video_ram.data()), videoram_size * sizeof(uint8_t));
}