#include "Pch.h"
#include "PixelPipeline.h"
#include "PixelProcessor.h"

#include "Display.h"
#include "Cartridge.h"

PixelPipeline::PixelPipeline(PixelProcessor* ppu, Display* display, Cartridge* cartridge) : m_PixelProcessor(ppu), m_Display(display), m_Cartridge(cartridge)
{
}

void PixelPipeline::PipelineProcess()
{
	if ((m_PixelProcessor->GetContext()->dots & 1))
	{
		PixelFetcher();
	}

	PushPixelToVideoBuffer();
}

void PixelPipeline::PixelFetcher()
{
	switch (m_Context.pipeline_state)
	{
		case FetchState::Tile:
		{
			m_Context.fetched_entries.clear();

			// Load background/window tile
			if ((m_Cartridge->IsColourModeDMG() && m_Display->IsBackgroundEnabled()) || !m_Cartridge->IsColourModeDMG())
			{
				uint16_t base_address = m_Display->GetBackgroundTileBaseAddress();

				// Divide by 8
				int position_y = (m_Display->GetContext()->ly + m_Display->GetContext()->scy) & 0xFF;
				int position_x = (m_Context.fetch_x + m_Display->GetContext()->scx) & 0xFF;

				// Fetch attributes
				uint8_t attribute = m_PixelProcessor->PipelineReadVideoRam(base_address + (position_x >> 3) + (position_y >> 3) * 32, 1);

				m_Context.background_window_attribute.colour_palette = static_cast<uint8_t>(attribute & 0b111);
				m_Context.background_window_attribute.bank = static_cast<uint8_t>((attribute >> 3) & 0x1);
				m_Context.background_window_attribute.flip_x = static_cast<bool>((attribute >> 5) & 0x1);
				m_Context.background_window_attribute.flip_y = static_cast<bool>((attribute >> 6) & 0x1);
				m_Context.background_window_attribute.priority = static_cast<bool>((attribute >> 7) & 0x1);

				// Fetch tile
				m_Context.background_window_tile = m_PixelProcessor->PipelineReadVideoRam(base_address + (position_x >> 3) + (position_y >> 3) * 32, 0);

				if (m_Display->GetBackgroundAndWindowTileData() == 0x8800)
				{
					m_Context.background_window_tile += 128;
				}

				// Load window tile
				LoadWindowTile();
			}

			// Load sprite tile
			if (m_Display->IsObjectEnabled())
			{
				LoadSpriteTile();
			}

			m_Context.pipeline_state = FetchState::TileDataLow;
			m_Context.fetch_x += 8;
			break;
		}

		case FetchState::TileDataLow:
		{
			FetchTileData(FetchTileByte::ByteLow);
			LoadSpriteData(FetchTileByte::ByteLow);

			m_Context.pipeline_state = FetchState::TileDataHigh;
			break;
		}

		case FetchState::TileDataHigh:
		{
			FetchTileData(FetchTileByte::ByteHigh);
			LoadSpriteData(FetchTileByte::ByteHigh);

			m_Context.pipeline_state = FetchState::Idle;
			break;
		}

		case FetchState::Idle:
		{
			m_Context.pipeline_state = FetchState::Push;
			break;
		}

		case FetchState::Push:
		{
			if (PipelineAddPixel())
			{
				m_Context.pipeline_state = FetchState::Tile;
			}

			break;
		}
	}
}

void PixelPipeline::PushPixelToVideoBuffer()
{
	if (m_Context.pixel_queue.size() > 8)
	{
		uint32_t pixel_data = (m_Context.pixel_queue.front());

		// Need this for android
#ifndef _WIN32
		pixel_data = 0xFF000000 | pixel_data;
#endif // !_WIN32


		m_Context.pixel_queue.pop();

		if (m_Context.line_x >= (m_Display->GetContext()->scx % 8))
		{
			m_Display->SetVideoBufferPixel(m_Context.pushed_x, m_Display->GetContext()->ly, pixel_data);
			m_Context.pushed_x++;
		}

		m_Context.line_x++;
	}
}

void PixelPipeline::FetchTileData(FetchTileByte tile_byte)
{
	// Calculate tile offset
	uint16_t offset_x = (m_Context.background_window_tile << 4);
	uint16_t offset_y = ((m_Display->GetContext()->ly + m_Display->GetContext()->scy) % 8) * 2;

	if (m_Display->IsWindowVisible() && this->IsWindowInView(m_Context.fetch_x))
	{
		int window_line = m_PixelProcessor->GetContext()->window_line;
		offset_y = ((window_line & 0x7) << 1);
	}

	// Flip y
	if (m_Context.background_window_attribute.flip_y)
	{
		offset_y = 16 - offset_y - 2;
	}

	// Fetch tile data
	uint16_t base_address = m_Display->GetBackgroundAndWindowTileData();
	if (tile_byte == FetchTileByte::ByteLow)
	{
		m_Context.background_window_byte_low = m_PixelProcessor->PipelineReadVideoRam(base_address + offset_x + offset_y, m_Context.background_window_attribute.bank);
	}
	else if (tile_byte == FetchTileByte::ByteHigh)
	{
		m_Context.background_window_byte_high = m_PixelProcessor->PipelineReadVideoRam(base_address + offset_x + offset_y + 1, m_Context.background_window_attribute.bank);
	}
}

void PixelPipeline::LoadSpriteData(FetchTileByte tile_byte)
{
	int current_ly = m_Display->GetContext()->ly;
	uint8_t sprite_height = m_Display->GetObjectHeight();

	for (int i = 0; i < m_Context.fetched_entries.size(); i++)
	{
		// Check Y orientation for which direction to load the pixels
		uint8_t tile_y = ((current_ly + 16) - m_Context.fetched_entries[i].oam->position_y) * 2;
		if (m_Context.fetched_entries[i].oam->flip_y)
		{
			tile_y = ((sprite_height * 2) - 2) - tile_y;
		}

		uint8_t tile_index = m_Context.fetched_entries[i].oam->tile_id;
		if (sprite_height == 16)
		{
			// Remove last bit
			tile_index &= ~(1);
		}

		if (tile_byte == FetchTileByte::ByteLow)
		{
			m_Context.fetched_entries[i].byte_low = m_PixelProcessor->PipelineReadVideoRam(0x8000 + (tile_index * 16) + tile_y + 0, m_Context.fetched_entries[i].oam->bank);
		}
		else if (tile_byte == FetchTileByte::ByteHigh)
		{
			m_Context.fetched_entries[i].byte_high = m_PixelProcessor->PipelineReadVideoRam(0x8000 + (tile_index * 16) + tile_y + 1, m_Context.fetched_entries[i].oam->bank);
		}
	}
}

void PixelPipeline::LoadSpriteTile()
{
	PixelProcessorContext* ppu_context = const_cast<PixelProcessorContext*>(m_PixelProcessor->GetContext());
	for (auto& oam : ppu_context->objects_per_line)
	{
		int sp_x = (oam.position_x - 8) + (m_Display->GetContext()->scx % 8);

		if ((sp_x >= m_Context.fetch_x && sp_x < m_Context.fetch_x + 8) || ((sp_x + 8) >= m_Context.fetch_x && (sp_x + 8) < m_Context.fetch_x + 8))
		{
			OamPipelineData data;
			data.oam = &oam;

			m_Context.fetched_entries.push_back(data);
		}

		// Max checking 3 sprites on pixels
		if (m_Context.fetched_entries.size() >= 3)
		{
			break;
		}
	}
}

void PixelPipeline::LoadWindowTile()
{
	if (m_Display->IsWindowVisible())
	{
		if (this->IsWindowInView(m_Context.fetch_x))
		{
			// Divide by 8
			uint8_t position_x = (m_Context.fetch_x - m_Display->GetContext()->wx + 7) & 0xFF;
			uint8_t position_y = m_PixelProcessor->GetContext()->window_line; // (display_context->ly - display_context->wy) & 0xFF;

			// Fetch tile
			uint16_t base_address = m_Display->GetWindowTileBaseAddress();
			m_Context.background_window_tile = m_PixelProcessor->PipelineReadVideoRam(base_address + (position_x >> 3) + ((position_y >> 3) * 32), 0);

			// Check if we are getting tiles from block 1
			if (m_Display->GetBackgroundAndWindowTileData() == 0x8800)
			{
				m_Context.background_window_tile += 128;
			}

			// Fetch attributes
			uint8_t attribute = m_PixelProcessor->PipelineReadVideoRam(base_address + (position_x >> 3) + ((position_y >> 3) * 32), 1);

			m_Context.background_window_attribute.colour_palette = static_cast<uint8_t>(attribute & 0b111);
			m_Context.background_window_attribute.bank = static_cast<uint8_t>((attribute >> 3) & 0x1);
			m_Context.background_window_attribute.flip_x = static_cast<bool>((attribute >> 5) & 0x1);
			m_Context.background_window_attribute.flip_y = static_cast<bool>((attribute >> 6) & 0x1);
			m_Context.background_window_attribute.priority = static_cast<bool>((attribute >> 7) & 0x1);
		}
	}
}

bool PixelPipeline::PipelineAddPixel()
{
	// Check if the queue is full - max of 8 pixels
	if (m_Context.pixel_queue.size() > 8)
	{
		return false;
	}

	// Discard pixels that are not on the screen
	int pixel_x = m_Context.fetch_x - (8 - (m_Display->GetContext()->scx % 8));
	if (pixel_x < 0)
	{
		return false;
	}

	for (int bit = 7; bit >= 0; bit--)
	{
		// Check for flip X
		uint8_t offset = bit;
		if (m_Context.background_window_attribute.flip_x)
		{
			offset = 7 - bit;
		}

		// Decode and get pixel colour from palette
		uint8_t data_high = (static_cast<bool>(m_Context.background_window_byte_low & (1 << (offset)))) << 0;
		uint8_t data_low = (static_cast<bool>(m_Context.background_window_byte_high & (1 << (offset)))) << 1;
		uint8_t palette_index = data_high | data_low;

		// uint32_t colour = display_context->background_palette[palette_index];
		uint8_t palette = m_Context.background_window_attribute.colour_palette;
		uint32_t colour = m_Display->GetColourFromBackgroundPalette(palette, palette_index);

		if (m_Display->IsObjectEnabled())
		{
			bool background_transparent = (palette_index == 0);
			colour = FetchSpritePixels(colour, background_transparent);
		}

		m_Context.pixel_queue.push(colour);
		m_Context.fifo_x++;
	}

	return true;
}

uint32_t PixelPipeline::FetchSpritePixels(uint32_t color, bool background_pixel_transparent)
{
	for (int i = 0; i < m_Context.fetched_entries.size(); i++)
	{
		// Object always have priority if the background is transparent
		if (!background_pixel_transparent)
		{
			if (!m_Display->IsBackgroundEnabled())
			{
				goto draw;
			}

			if (!m_Context.fetched_entries[i].oam->priority && !m_Context.background_window_attribute.priority)
			{
				goto draw;
			}

			continue;
		}

	draw:

		// Past pixel point already
		int sprite_x = (m_Context.fetched_entries[i].oam->position_x - 8) + ((m_Display->GetContext()->scx % 8));
		if (sprite_x + 8 < m_Context.fifo_x)
		{
			continue;
		}

		int offset = m_Context.fifo_x - sprite_x;

		uint8_t bit = (7 - offset);
		if (m_Context.fetched_entries[i].oam->flip_x)
		{
			bit = offset;
		}

		uint8_t high = (static_cast<bool>(m_Context.fetched_entries[i].byte_low & (1 << bit))) << 0;
		uint8_t low = (static_cast<bool>(m_Context.fetched_entries[i].byte_high & (1 << bit))) << 1;
		uint8_t palette_index = high | low;

		// Transparent
		if (palette_index == 0)
		{
			continue;
		}

		// Select pixel colour
		if (m_Cartridge->IsColourModeDMG())
		{
			uint8_t palette = m_Context.fetched_entries[i].oam->dmg_palette;
			return m_Display->GetColourFromObjectPalette(palette, palette_index);
		}
		else
		{
			uint8_t palette = m_Context.fetched_entries[i].oam->gcb_palette;
			return m_Display->GetColourFromObjectPalette(palette, palette_index);
		}
	}

	return color;
}

bool PixelPipeline::IsWindowInView(int pixel_x)
{
	const int ScreenResolutionX = 160;
	const int ScreenResolutionY = 144;

	DisplayContext* display_context = m_Display->GetContext();
	if (display_context->ly >= display_context->wy && display_context->ly < display_context->wy + ScreenResolutionY)
	{
		if ((pixel_x >= display_context->wx - 7) && (pixel_x < display_context->wx + ScreenResolutionX - 7))
		{
			return true;
		}
	}

	return false;
}
