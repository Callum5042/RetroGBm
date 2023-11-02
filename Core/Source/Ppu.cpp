#include "Ppu.h"
#include "Cpu.h"
#include "Display.h"
#include "Emulator.h"
#include <cstring>
#include <algorithm>

#pragma warning(push)
#pragma warning(disable : 26819)
#include <SDL.h>
#pragma warning(pop) 

Ppu::Ppu()
{
	m_Bus = Emulator::Instance;
	m_Display = Emulator::Instance->GetDisplay();
}  

void Ppu::Init()
{
	context.current_frame = 0;
	context.dot_ticks = 0;

	context.video_buffer.resize(ScreenResolutionY * ScreenResolutionX);
	std::fill(context.video_buffer.begin(), context.video_buffer.end(), 0x0);

	context.video_ram.resize(0x2000);
	std::fill(context.video_ram.begin(), context.video_ram.end(), 0x0);

	context.pfc.line_x = 0;
	context.pfc.pushed_x = 0;
	context.pfc.fetch_x = 0;
	context.pfc.current_fetch_state = FetchState::Tile;

	context.line_sprites = 0;
	context.fetched_entry_count = 0;
	context.window_line = 0;

	m_Display->Init();
	m_Display->SetLcdMode(LcdMode::OAM);
}

void Ppu::Tick()
{
	context.dot_ticks++;

	switch (m_Display->GetLcdMode())
	{
		case LcdMode::OAM:
			UpdateOam();
			break;
		case LcdMode::PixelTransfer:
			PixelTransfer();
			break;
		case LcdMode::VBlank:
			VBlank();
			break;
		case LcdMode::HBlank:
			HBlank();
			break;
	}
}

void Ppu::WriteOam(uint16_t address, uint8_t value)
{
	if (address >= 0xFE00)
	{
		address -= 0xFE00;
	}

	uint8_t* p = reinterpret_cast<uint8_t*>(context.oam_ram.data());
	p[address] = value;
}

uint8_t Ppu::ReadOam(uint16_t address)
{
	if (address >= 0xFE00)
	{
		address -= 0xFE00;
	}

	uint8_t* p = reinterpret_cast<uint8_t*>(context.oam_ram.data());
	return p[address];
}

void Ppu::WriteVideoRam(uint16_t address, uint8_t value)
{
	context.video_ram[address - 0x8000] = value;
}

uint8_t Ppu::ReadVideoRam(uint16_t address)
{
	return context.video_ram[address - 0x8000];
}

void Ppu::UpdateOam()
{
	// Searching for objects takes 80 ticks
	if (context.dot_ticks >= 80)
	{
		m_Display->SetLcdMode(LcdMode::PixelTransfer);

		// Reset pipeline
		context.pfc.current_fetch_state = FetchState::Tile;
		PipelineReset();
	}

	// Read OAM only on the first tick
	if (context.dot_ticks == 1)
	{
		LoadSpritesForLine();
	}
}

void Ppu::PixelTransfer()
{
	PipelineProcess();

	// Process pixels until we finish the line
	if (context.pfc.pushed_x >= ScreenResolutionX)
	{
		m_Display->SetLcdMode(LcdMode::HBlank);
		if (m_Display->IsStatInterruptHBlank())
		{
			Emulator::Instance->GetCpu()->RequestInterrupt(InterruptFlag::STAT);
		}
	}
}

void Ppu::VBlank()
{
	if (context.dot_ticks >= m_DotTicksPerLine)
	{
		IncementLY();

		// Keep increasing LY register until we reach the lines per frame
		if (m_Display->context.ly >= m_LinesPerFrame)
		{
			m_Display->SetLcdMode(LcdMode::OAM);
			m_Display->context.ly = 0;
			context.window_line = 0;
		}

		context.dot_ticks = 0;
	}
}

void Ppu::HBlank()
{
	if (context.dot_ticks >= m_DotTicksPerLine)
	{
		IncementLY();

		// Enter VBlank if all the scanlines have been drawn
		if (m_Display->context.ly >= ScreenResolutionY)
		{
			m_Display->SetLcdMode(LcdMode::VBlank);
			Emulator::Instance->GetCpu()->RequestInterrupt(InterruptFlag::VBlank);

			if (m_Display->IsStatInterruptVBlank())
			{
				Emulator::Instance->GetCpu()->RequestInterrupt(InterruptFlag::STAT);
			}

			context.current_frame++;
			CalculateFPS();
		}
		else
		{
			m_Display->SetLcdMode(LcdMode::OAM);
		}

		context.dot_ticks = 0;
	}
}

void Ppu::CalculateFPS()
{
	uint32_t end = SDL_GetTicks();
	uint32_t frame_time = end - m_PreviousFrameTime;

	if (frame_time < m_TargetFrameTime)
	{
		SDL_Delay((m_TargetFrameTime - frame_time));
	}

	if (end - m_StartTimer >= 1000)
	{
		uint32_t fps = m_FrameCount;
		m_StartTimer = end;
		m_FrameCount = 0;

		/*if (m_Bus->m_Cartridge.NeedSave())
		{
			m_Bus->m_Cartridge.BatterySave();
		}*/
	}

	m_FrameCount++;
	m_PreviousFrameTime = SDL_GetTicks();
}

void Ppu::PipelineReset()
{
	context.pfc.line_x = 0;
	context.pfc.fetch_x = 0;
	context.pfc.pushed_x = 0;
	context.pfc.fifo_x = 0;

	// Pop all pixels
	while (!context.pfc.pixel_queue.empty())
	{
		context.pfc.pixel_queue.pop();
	}
}

void Ppu::PipelineProcess()
{
	if (!(context.dot_ticks & 1))
	{
		context.pfc.map_y = (m_Display->context.ly + m_Display->context.scy);
		context.pfc.map_x = (context.pfc.fetch_x + m_Display->context.scx);
		context.pfc.tile_y = ((m_Display->context.ly + m_Display->context.scy) % 8) * 2;

		PixelFetcher();
	}

	PushPixelToVideoBuffer();
}

bool Ppu::IsWindowVisible()
{
	return m_Display->IsWindowEnabled()
		&& m_Display->context.wx >= 0
		&& m_Display->context.wx <= 166
		&& m_Display->context.wy >= 0
		&& m_Display->context.wy < ScreenResolutionY;
}

void Ppu::IncementLY()
{
	if (IsWindowVisible() && m_Display->context.ly >= m_Display->context.wy && m_Display->context.ly < m_Display->context.wy + ScreenResolutionY)
	{
		context.window_line++;
	}

	m_Display->context.ly++;
	if (m_Display->context.ly == m_Display->context.lyc)
	{
		// Turn on bit 2 "LYC == LY"
		m_Display->context.stat |= (1 << 2);

		if (m_Display->IsStatInterruptLYC())
		{
			Emulator::Instance->GetCpu()->RequestInterrupt(InterruptFlag::STAT);
		}
	}
	else
	{
		// Turn off bit 2
		m_Display->context.stat &= ~(1 << 2);
	}
}

void Ppu::LoadSpritesForLine()
{
	context.line_sprites = 0;
	context.line_sprite_count = 0;

	int current_ly = m_Display->context.ly;
	uint8_t sprite_height = m_Display->GetObjectHeight();

	// Reset lines
	std::fill(context.line_entry_array.begin(), context.line_entry_array.end(), OamLineNode());

	// OAM supports up to 40 moveable sprites
	for (int i = 0; i < 40; i++)
	{
		OamData oam_data = context.oam_ram[i];

		// If X is 0 then the object is not visible
		if (oam_data.x == 0)
		{
			continue;
		}

		// Only 10 sprites can be shown per scanline
		if (context.line_sprite_count >= 10)
		{
			break;
		}

		// The sprite is on the current line
		if (oam_data.y <= current_ly + 16 && oam_data.y + sprite_height > current_ly + 16)
		{
			OamLineNode* line_entry = &context.line_entry_array[context.line_sprite_count];
			context.line_sprite_count++;

			line_entry->entry = oam_data;
			line_entry->next = nullptr;

			if (context.line_sprites == nullptr || context.line_sprites->entry.x > oam_data.x)
			{
				line_entry->next = context.line_sprites;
				context.line_sprites = line_entry;
				continue;
			}

			// Do sorting
			OamLineNode* current_node = context.line_sprites;
			OamLineNode* previous_node = current_node;

			while (current_node != nullptr)
			{
				if (current_node->entry.x > oam_data.x)
				{
					previous_node->next = line_entry;
					line_entry->next = current_node;
					break;
				}

				if (current_node->next == nullptr)
				{
					current_node->next = line_entry;
					break;
				}

				previous_node = current_node;
				current_node = current_node->next;
			}
		}
	}
}

uint32_t Ppu::FetchSpritePixels(int bit, uint32_t color, uint8_t bg_color)
{
	for (int i = 0; i < context.fetched_entry_count; i++)
	{
		int sp_x = (context.fetched_entries[i].x - 8) + ((m_Display->context.scx % 8));
		if (sp_x + 8 < context.pfc.fifo_x)
		{
			//past pixel point already...
			continue;
		}

		int offset = context.pfc.fifo_x - sp_x;
		if (offset < 0 || offset > 7)
		{
			//out of bounds..
			continue;
		}

		bit = (7 - offset);
		if (context.fetched_entries[i].flag_x_flip)
		{
			bit = offset;
		}

		uint8_t hi = !!(context.pfc.fetch_oam_data[i * 2] & (1 << bit));
		uint8_t lo = !!(context.pfc.fetch_oam_data[(i * 2) + 1] & (1 << bit)) << 1;

		bool bg_priority = context.fetched_entries[i].flag_priority;

		if (!(hi | lo))
		{
			//transparent
			continue;
		}

		if (!bg_priority || bg_color == 0)
		{
			color = (context.fetched_entries[i].flag_dmg_palette) ? m_Display->context.sprite2_palette[hi | lo] : m_Display->context.sprite1_palette[hi | lo];
			if (hi | lo)
			{
				break;
			}
		}
	}

	return color;
}

bool Ppu::PipelineAddPixel()
{
	// Check if the queue is full - max of 8 pixels
	if (context.pfc.pixel_queue.size() > 8)
	{
		return false;
	}

	int x = context.pfc.fetch_x - (8 - (m_Display->context.scx % 8));

	for (int i = 0; i < 8; i++)
	{
		int bit = 7 - i;
		uint8_t data_high = !!(context.pfc.bgw_fetch_data[1] & (1 << bit));
		uint8_t data_low = !!(context.pfc.bgw_fetch_data[2] & (1 << bit)) << 1;
		uint32_t colour = m_Display->context.background_palette[data_high | data_low];

		if (!m_Display->IsBackgroundEnabled())
		{
			colour = m_Display->context.background_palette[0];
		}

		if (m_Display->IsObjectEnabled())
		{
			colour = FetchSpritePixels(bit, colour, data_high | data_low);
		}

		if (x >= 0)
		{
			context.pfc.pixel_queue.push(colour);
			context.pfc.fifo_x++;
		}
	}

	return true;
}

void Ppu::LoadSpriteTile()
{
	OamLineNode* le = context.line_sprites;

	while (le)
	{
		int sp_x = (le->entry.x - 8) + (m_Display->context.scx % 8);

		if ((sp_x >= context.pfc.fetch_x && sp_x < context.pfc.fetch_x + 8) || ((sp_x + 8) >= context.pfc.fetch_x && (sp_x + 8) < context.pfc.fetch_x + 8))
		{
			//need to add entry
			context.fetched_entries[context.fetched_entry_count++] = le->entry;
		}

		le = le->next;

		if (!le || context.fetched_entry_count >= 3)
		{
			//max checking 3 sprites on pixels
			break;
		}
	}
}

void Ppu::LoadSpriteData(uint8_t offset)
{
	int current_ly = m_Display->context.ly;
	uint8_t sprite_height = m_Display->GetObjectHeight();

	for (int i = 0; i < context.fetched_entry_count; i++)
	{
		// Check Y orientation for which direction to load the pixels
		uint8_t ty = ((current_ly + 16) - context.fetched_entries[i].y) * 2;
		if (context.fetched_entries[i].flag_y_flip)
		{
			ty = ((sprite_height * 2) - 2) - ty;
		}

		uint8_t tile_index = context.fetched_entries[i].tile_index;
		if (sprite_height == 16)
		{
			// Remove last bit
			tile_index &= ~(1);
		}

		context.pfc.fetch_oam_data[(i * 2) + offset] = m_Bus->ReadBus(0x8000 + (tile_index * 16) + ty + offset);
	}
}

void Ppu::LoadWindowTile()
{
	if (!IsWindowVisible())
	{
		return;
	}

	uint8_t window_y = m_Display->context.wy;
	if (context.pfc.fetch_x + 7 >= m_Display->context.wx && context.pfc.fetch_x + 7 < m_Display->context.wx + ScreenResolutionY + 14)
	{
		if (m_Display->context.ly >= window_y && m_Display->context.ly < window_y + ScreenResolutionX)
		{
			uint8_t w_tile_y = context.window_line / 8;

			context.pfc.bgw_fetch_data[0] = m_Bus->ReadBus(m_Display->GetWindowTileBaseAddress() + ((context.pfc.fetch_x + 7 - m_Display->context.wx) / 8) + (w_tile_y * 32));
			if (m_Display->GetBackgroundTileData() == 0x8800)
			{
				context.pfc.bgw_fetch_data[0] += 128;
			}
		}
	}
}

void Ppu::PixelFetcher()
{
	switch (context.pfc.current_fetch_state)
	{
		case FetchState::Tile:
		{
			context.fetched_entry_count = 0;

			// Load background/window tile
			if (m_Display->IsBackgroundEnabled())
			{
				// Load background tile
				int16_t address = m_Display->GetBackgroundTileBaseAddress();
				context.pfc.bgw_fetch_data[0] = m_Bus->ReadBus(address + (context.pfc.map_x / 8) + (((context.pfc.map_y / 8)) * 32));

				if (m_Display->GetBackgroundTileData() == 0x8800)
				{
					context.pfc.bgw_fetch_data[0] += 128;
				}

				LoadWindowTile();
			}

			// Load sprite tile
			if (m_Display->IsObjectEnabled() && context.line_sprites != nullptr)
			{
				LoadSpriteTile();
			}

			context.pfc.current_fetch_state = FetchState::TileDataLow;
			context.pfc.fetch_x += 8;
			break;
		}

		case FetchState::TileDataLow:
		{
			context.pfc.bgw_fetch_data[1] = m_Bus->ReadBus(m_Display->GetBackgroundTileData() + (context.pfc.bgw_fetch_data[0] * 16) + context.pfc.tile_y);
			LoadSpriteData(0);
			context.pfc.current_fetch_state = FetchState::TileDataHigh;
			break;
		}

		case FetchState::TileDataHigh:
		{
			context.pfc.bgw_fetch_data[2] = m_Bus->ReadBus(m_Display->GetBackgroundTileData() + (context.pfc.bgw_fetch_data[0] * 16) + context.pfc.tile_y + 1);
			LoadSpriteData(1);
			context.pfc.current_fetch_state = FetchState::Idle;
			break;
		}

		case FetchState::Idle:
		{
			context.pfc.current_fetch_state = FetchState::Push;
			break;
		}

		case FetchState::Push:
		{
			if (PipelineAddPixel())
			{
				context.pfc.current_fetch_state = FetchState::Tile;
			}

			break;
		}
	}
}

void Ppu::PushPixelToVideoBuffer()
{
	if (context.pfc.pixel_queue.size() > 8)
	{
		uint32_t pixel_data = context.pfc.pixel_queue.front();
		context.pfc.pixel_queue.pop();

		if (context.pfc.line_x >= (m_Display->context.scx % 8))
		{
			context.video_buffer[context.pfc.pushed_x + (m_Display->context.ly * ScreenResolutionX)] = pixel_data;
			context.pfc.pushed_x++;
		}

		context.pfc.line_x++;
	}
}