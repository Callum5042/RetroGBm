#include "RetroGBm/Pch.h"
#include "RetroGBm/Ppu.h"
#include "RetroGBm/Cpu.h"
#include "RetroGBm/Display.h"
#include "RetroGBm/Emulator.h"
#include "RetroGBm/Cartridge/BaseCartridge.h"
#include "RetroGBm/HighTimer.h"
#include "RetroGBm/Dma.h"

#include <algorithm>
#include <chrono>
#include <thread>

Ppu::Ppu()
{
	m_Bus = Emulator::Instance;
	m_Cpu = Emulator::Instance->GetCpu();
	m_Display = Emulator::Instance->GetDisplay();
	m_Cartridge = Emulator::Instance->GetCartridge();
}

Ppu::Ppu(IBus* bus, Cpu* cpu, Display* display, BaseCartridge* cartridge) : m_Bus(bus), m_Cpu(cpu), m_Display(display), m_Cartridge(cartridge)
{
}

void Ppu::Init()
{

	m_Context.video_ram.resize(16384);
	std::fill(m_Context.video_ram.begin(), m_Context.video_ram.end(), 0x0);

	m_Display->Init();
	m_Display->SetLcdMode(LcdMode::OAM);

	m_Timer.Start();
}

void Ppu::Tick()
{
	if (!m_Display->IsLcdEnabled())
	{
		return;
	}

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

uint8_t Ppu::ReadVideoRam(uint16_t address, uint8_t bank)
{
	return m_Context.video_ram[(address - 0x8000) + (bank * 8192)];
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
		if (m_Cartridge->IsColourModeDMG() || m_Display->IsObjectPriorityModeSet())
		{
			std::sort(m_Context.objects_per_line.begin(), m_Context.objects_per_line.end(), [](const OamData& lhs, const OamData& rhs)
			{
				return (lhs.position_x < rhs.position_x);
			});
		}

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
	if (m_Context.pipeline.pushed_x >= m_Display->ScreenResolutionX)
	{
		m_Display->SetLcdMode(LcdMode::HBlank);
		if (m_Display->IsStatInterruptHBlank())
		{
			m_Cpu->RequestInterrupt(InterruptFlag::STAT);
		}

		Emulator::Instance->GetDma()->RunHDMA();
	}
}

void Ppu::VBlank()
{
	if (m_Display->m_Context.ly == 153)
	{
		if (m_Context.dot_ticks == 4)
		{
			m_Display->m_Context.ly = 0;
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
		}
	}

	if (m_Context.dot_ticks >= m_DotTicksPerLine)
	{
		// Keep increasing LY register until we reach the lines per frame
		if (m_Display->m_Context.ly == 0)
		{
			m_Context.dot_ticks = 0;
			IncrementLY();

			LimitFrameRate();

			m_Display->SetLcdMode(LcdMode::OAM);
			if (m_Display->IsStatInterruptOAM())
			{
				m_Cpu->RequestInterrupt(InterruptFlag::STAT);
			}

			m_Display->m_Context.ly = 0;
			m_Context.window_line_counter = 0;
		}
		else
		{
			m_Context.dot_ticks = 0;
			IncrementLY();
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
		if (m_Display->m_Context.ly >= m_Display->ScreenResolutionY)
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

			if (m_Display->IsStatInterruptOAM())
			{
				m_Cpu->RequestInterrupt(InterruptFlag::STAT);
			}
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
	if (m_Display->IsWindowVisible() && (m_Display->m_Context.ly > m_Display->m_Context.wy) && (m_Display->m_Context.ly <= m_Display->m_Context.wy + m_Display->ScreenResolutionY))
	{
		m_Context.window_line_counter++;
	}
}

uint32_t Ppu::FetchSpritePixels(uint32_t color, bool background_pixel_transparent)
{
	for (int i = 0; i < m_Context.pipeline.fetched_entries.size(); i++)
	{
		// Object always have priority if the background is transparent
		if (!background_pixel_transparent)
		{
			if (!m_Display->IsBackgroundEnabled())
			{
				goto draw;
			}

			if (!m_Context.pipeline.fetched_entries[i].oam->priority && !m_Context.pipeline.background_window_attribute.priority)
			{
				goto draw;
			}

			continue;
		}

	draw:

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
		if (Emulator::Instance->GetCartridge()->IsColourModeDMG())
		{
			uint8_t palette = m_Context.pipeline.fetched_entries[i].oam->dmg_palette;
			return m_Display->GetColourFromObjectPalette(palette, palette_index);
		}
		else
		{
			uint8_t palette = m_Context.pipeline.fetched_entries[i].oam->gcb_palette;
			return m_Display->GetColourFromObjectPalette(palette, palette_index);
		}
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
		// Check for flip X
		uint8_t offset = bit;
		if (m_Context.pipeline.background_window_attribute.flip_x)
		{
			offset = 7 - bit;
		}

		// Decode and get pixel colour from palette
		uint8_t data_high = (static_cast<bool>(m_Context.pipeline.background_window_byte_low & (1 << (offset)))) << 0;
		uint8_t data_low = (static_cast<bool>(m_Context.pipeline.background_window_byte_high & (1 << (offset)))) << 1;
		uint8_t palette_index = data_high | data_low;

		// uint32_t colour = m_Display->m_Context.background_palette[palette_index];
		uint8_t palette = m_Context.pipeline.background_window_attribute.colour_palette;
		uint32_t colour = m_Display->GetColourFromBackgroundPalette(palette, palette_index);
		
		if (m_Display->IsObjectEnabled())
		{
			bool background_transparent = (palette_index == 0);
			colour = FetchSpritePixels(colour, background_transparent);
		}

		m_Context.pipeline.pixel_queue.push_back(colour);
		m_Context.pipeline.fifo_x++;
	}

	return true;
}

void Ppu::FetchObjectData(FetchTileByte tile_byte)
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
			m_Context.pipeline.fetched_entries[i].byte_low = this->ReadVideoRam(0x8000 + (tile_index * 16) + tile_y + 0, m_Context.pipeline.fetched_entries[i].oam->bank);
		}
		else if (tile_byte == FetchTileByte::ByteHigh)
		{
			m_Context.pipeline.fetched_entries[i].byte_high = this->ReadVideoRam(0x8000 + (tile_index * 16) + tile_y + 1, m_Context.pipeline.fetched_entries[i].oam->bank);
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

			FetchBackgroundTileId();
			FetchWindowTileId();
			FetchObjectTileId();

			m_Context.pipeline.pipeline_state = FetchState::TileDataLow;
			m_Context.pipeline.fetch_x += 8;
			break;
		}

		case FetchState::TileDataLow:
		{
			FetchTileData(FetchTileByte::ByteLow);
			FetchObjectData(FetchTileByte::ByteLow);

			m_Context.pipeline.pipeline_state = FetchState::TileDataHigh;
			break;
		}

		case FetchState::TileDataHigh:
		{
			FetchTileData(FetchTileByte::ByteHigh);
			FetchObjectData(FetchTileByte::ByteHigh);

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

void Ppu::FetchBackgroundTileId()
{
	if ((m_Cartridge->IsColourModeDMG() && m_Display->IsBackgroundEnabled()) || !m_Cartridge->IsColourModeDMG())
	{
		uint16_t base_address = m_Display->GetBackgroundTileBaseAddress();

		// Get address
		int position_y = (m_Display->m_Context.ly + m_Display->m_Context.scy) & 0xFF;
		int position_x = (m_Context.pipeline.fetch_x + m_Display->m_Context.scx) & 0xFF;

		uint16_t address = base_address + (position_x / 8) + (position_y / 8) * 32;

		// Fetch tile
		m_Context.pipeline.background_window_tile = this->ReadVideoRam(address, 0);

		if (m_Display->GetBackgroundAndWindowTileData() == 0x8800)
		{
			m_Context.pipeline.background_window_tile += 128;
		}

		// Fetch attributes
		uint8_t attribute = this->ReadVideoRam(address, 1);

		m_Context.pipeline.background_window_attribute.colour_palette = static_cast<uint8_t>(attribute & 0b111);
		m_Context.pipeline.background_window_attribute.bank = static_cast<uint8_t>((attribute >> 3) & 0x1);
		m_Context.pipeline.background_window_attribute.flip_x = static_cast<bool>((attribute >> 5) & 0x1);
		m_Context.pipeline.background_window_attribute.flip_y = static_cast<bool>((attribute >> 6) & 0x1);
		m_Context.pipeline.background_window_attribute.priority = static_cast<bool>((attribute >> 7) & 0x1);
	}
}

void Ppu::FetchWindowTileId()
{
	if ((m_Cartridge->IsColourModeDMG() && m_Display->IsBackgroundEnabled()) || !m_Cartridge->IsColourModeDMG())
	{
		if (m_Display->IsWindowVisible())
		{
			if (IsWindowInView(m_Context.pipeline.fetch_x))
			{
				uint16_t base_address = m_Display->GetWindowTileBaseAddress();

				// Divide by 8
				uint8_t position_x = (m_Context.pipeline.fetch_x - m_Display->m_Context.wx + 7) & 0xFF;
				uint8_t position_y = m_Context.window_line_counter & 0xFF; // (m_Display->m_Context.ly - m_Display->m_Context.wy) & 0xFF;

				uint16_t address = base_address + (position_x / 8) + (position_y / 8) * 32;

				// Fetch tile
				m_Context.pipeline.background_window_tile = this->ReadVideoRam(address, 0);

				// Check if we are getting tiles from block 1
				if (m_Display->GetBackgroundAndWindowTileData() == 0x8800)
				{
					m_Context.pipeline.background_window_tile += 128;
				}

				// Fetch attributes
				uint8_t attribute = this->ReadVideoRam(address, 1);

				m_Context.pipeline.background_window_attribute.colour_palette = static_cast<uint8_t>(attribute & 0b111);
				m_Context.pipeline.background_window_attribute.bank = static_cast<uint8_t>((attribute >> 3) & 0x1);
				m_Context.pipeline.background_window_attribute.flip_x = static_cast<bool>((attribute >> 5) & 0x1);
				m_Context.pipeline.background_window_attribute.flip_y = static_cast<bool>((attribute >> 6) & 0x1);
				m_Context.pipeline.background_window_attribute.priority = static_cast<bool>((attribute >> 7) & 0x1);
			}
		}
	}
}

void Ppu::FetchObjectTileId()
{
	if (m_Display->IsObjectEnabled())
	{
		for (auto& oam : m_Context.objects_per_line)
		{
			int obj_x = (oam.position_x - 8) + (m_Display->m_Context.scx % 8);

			int fetch_x = m_Context.pipeline.fetch_x;
			if ((obj_x >= fetch_x && obj_x < fetch_x + 8) || ((obj_x + 8) >= fetch_x && (obj_x + 8) < fetch_x + 8))
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
}

void Ppu::PushPixelToVideoBuffer()
{
	// Check if we're fetching a window tile
	if (m_Display->IsWindowVisible() && IsWindowInView(m_Context.pipeline.pushed_x))
	{
		if (!m_Context.pipeline.fetch_window)
		{
			// Set fetch to window and destroy pipeline
			m_Context.pipeline.fetch_window = true;
			m_Context.dot_ticks = 0;
			m_Context.pipeline.pipeline_state = FetchState::Tile;

			m_Context.pipeline.fifo_x = m_Context.pipeline.line_x;
			m_Context.pipeline.fetch_x = m_Context.pipeline.line_x;

			m_Context.pipeline.pixel_queue.clear();
		}
	}
	else
	{
		m_Context.pipeline.fetch_window = false;
	}

	if (m_Context.pipeline.pixel_queue.size() > 8)
	{
		uint32_t pixel_data = (m_Context.pipeline.pixel_queue.front());

		// Need this for android
#ifndef _WIN32
		pixel_data = 0xFF000000 | pixel_data;
#endif // !_WIN32


		m_Context.pipeline.pixel_queue.pop_front();

		if (m_Context.pipeline.fetch_window)
		{
			m_Display->SetVideoBufferPixel(m_Context.pipeline.pushed_x, m_Display->GetContext()->ly, pixel_data);
			// m_Context.video_buffer[m_Context.pipeline.pushed_x + (m_Display->m_Context.ly * m_Display->ScreenResolutionX)] = pixel_data;
			m_Context.pipeline.pushed_x++;
		}
		else
		{
			if (m_Context.pipeline.line_x >= (m_Display->m_Context.scx % 8))
			{
				m_Display->SetVideoBufferPixel(m_Context.pipeline.pushed_x, m_Display->GetContext()->ly, pixel_data);
				// m_Context.video_buffer[m_Context.pipeline.pushed_x + (m_Display->m_Context.ly * m_Display->ScreenResolutionX)] = pixel_data;
				m_Context.pipeline.pushed_x++;
			}
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

	if (m_Display->IsWindowVisible() && IsWindowInView(m_Context.pipeline.fetch_x))
	{
		offset_y = ((m_Context.window_line_counter & 0x7) << 1);
	}

	// Flip y
	if (m_Context.pipeline.background_window_attribute.flip_y)
	{
		offset_y = 16 - offset_y - 2;
	}

	// Fetch tile data
	uint16_t base_address = m_Display->GetBackgroundAndWindowTileData();
	if (tile_byte == FetchTileByte::ByteLow)
	{
		m_Context.pipeline.background_window_byte_low = this->ReadVideoRam(base_address + offset_x + offset_y, m_Context.pipeline.background_window_attribute.bank);
	}
	else if (tile_byte == FetchTileByte::ByteHigh)
	{
		m_Context.pipeline.background_window_byte_high = this->ReadVideoRam(base_address + offset_x + offset_y + 1, m_Context.pipeline.background_window_attribute.bank);
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

	// VPS lock
	double frame_goal = (m_TargetFrameTime * m_SpeedMultipler);
	double excution_time_seconds = m_Timer.DeltaTime();
	if (excution_time_seconds < frame_goal)
	{
		m_Timer.Stop();

		// Calculate current execution time in seconds
		double elapsed_seconds = (frame_goal - excution_time_seconds);

		// Specify the wait time
		auto wait_time = std::chrono::duration<double, std::milli>(elapsed_seconds * 1000.0f);

		// Record the start time
		auto start_time = std::chrono::high_resolution_clock::now();

		// Busy-wait until the required time has passed
		while (std::chrono::high_resolution_clock::now() - start_time < wait_time)
		{
			// Do nothing
			std::this_thread::yield();
		}

		m_Timer.Start();
	}
}

void Ppu::SaveState(std::fstream* file)
{
	size_t videoram_size = m_Context.video_ram.size();
	file->write(reinterpret_cast<const char*>(&videoram_size), sizeof(size_t));
	file->write(reinterpret_cast<const char*>(m_Context.video_ram.data()), videoram_size * sizeof(uint8_t));

	file->write(reinterpret_cast<const char*>(&m_VramBank), sizeof(m_VramBank));
}

void Ppu::LoadState(std::fstream* file)
{
	size_t videoram_size = 0;
	file->read(reinterpret_cast<char*>(&videoram_size), sizeof(size_t));

	m_Context.video_ram.resize(videoram_size);
	file->read(reinterpret_cast<char*>(m_Context.video_ram.data()), videoram_size * sizeof(uint8_t));

	file->read(reinterpret_cast<char*>(&m_VramBank), sizeof(m_VramBank));
}

void Ppu::SetSpeedMultipler(float speed)
{
	m_SpeedMultipler = speed;
}