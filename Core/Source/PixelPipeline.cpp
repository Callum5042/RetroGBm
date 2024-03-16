#include "Pch.h"
#include "PixelPipeline.h"

#include <Display.h>
#include <PixelProcessor.h>

#include <cstdint>

PixelPipeline::PixelPipeline(PixelProcessor* ppu, Display* display) : m_Ppu(ppu), m_Display(display)
{
}

bool PixelPipeline::Fetch(PixelData* pixel_data)
{
	// Pipeline fetch state
	m_Context.dots++;
	if (m_Context.dots & 1)
	{
		switch (m_Context.fetch_state)
		{
			case FetchState::GetTile:
				ReadTile();
				break;
			case FetchState::ReadData1:
				ReadData1();
				break;
			case FetchState::ReadData2:
				ReadData2();
				break;
			case FetchState::Idle:
				Idle();
				break;
		}
	}

	// Pushes pixels to queue
	AddPixels();

	// Push pixels out of queue
	if (m_Context.background_queue.size() > 8)
	{
		if (pixel_data != nullptr)
		{
			*pixel_data = m_Context.background_queue.front();
		}

		m_Context.background_queue.pop_front();
		return true;
	}

	return false;
}

void PixelPipeline::ReadTile()
{
	// TODO: Check if 'm_Context.fetcher_x' is inside the window. If so use window stuff to fetch tiles. Must also destroy mr pipeline and restart mr fetch

	// TODO: Fetch window tiles


	// Fetch background tiles
	uint16_t base_address = m_Display->GetBackgroundTileBaseAddress();


	// PPU Access to VRAM can be blocked
	// - LCD turning off
	// - At scanline 0 on CGB when not in double speed mode
	// - When switching from mode 3 to mode 0
	// - On CGB when searching OAM and index 37 is reached

	int x = ((m_Display->GetContext()->scx / 8) + m_Context.fetcher_x) & 0x1F;
	int y = (m_Display->GetContext()->ly + m_Display->GetContext()->scy) & 255;
	uint16_t address = base_address + (x + (y * 32));

	// Get tile ID
	const int tilemap_bank = 0;
	m_Context.tile_id = m_Ppu->PipelineReadVideoRam(address, tilemap_bank);

	// Get tile attributes
	const int tileattribute_bank = 1;
	uint8_t attribute = m_Ppu->PipelineReadVideoRam(address, tileattribute_bank);

	m_Context.background_window_attribute.colour_palette = (attribute) & 0b111;
	m_Context.background_window_attribute.bank = (attribute >> 3) & 1;
	m_Context.background_window_attribute.flip_x = static_cast<bool>((attribute >> 5) & 1);
	m_Context.background_window_attribute.flip_y = static_cast<bool>((attribute >> 6) & 1);
	m_Context.background_window_attribute.priority = static_cast<bool>((attribute >> 7) & 1);

	// Increase stage
	m_Context.fetcher_x += 8;
	m_Context.fetch_state = FetchState::ReadData1;
}

void PixelPipeline::ReadData1()
{
	m_Context.data_low = ReadByte(FetchTileByte::ByteLow);
	m_Context.fetch_state = FetchState::ReadData2;
}

void PixelPipeline::ReadData2()
{
	m_Context.data_high = ReadByte(FetchTileByte::ByteHigh);
	m_Context.fetch_state = FetchState::Idle;
}

void PixelPipeline::Idle()
{
	m_Context.fetch_state = FetchState::GetTile;
}

uint8_t PixelPipeline::ReadByte(FetchTileByte tile_byte)
{
	uint16_t base_address = m_Display->GetBackgroundAndWindowTileData();
	uint16_t address = base_address + (m_Context.tile_id * 16);

	if (tile_byte == FetchTileByte::ByteHigh)
	{
		address += 1;
	}

	uint8_t value = m_Ppu->PipelineReadVideoRam(address, m_Context.background_window_attribute.bank);
	return value;
}

void PixelPipeline::AddPixels()
{
	// Check if the queue is full - max of 8 pixels
	if (m_Context.background_queue.size() > 8)
	{
		return;
	}

	// Add pixels to queue
	if (m_Context.background_window_attribute.flip_x)
	{

	}
	//else
	{
		for (int bit = 7; bit >= 0; bit--)
		{
			int offset = bit;

			// Decode and get pixel colour from data
			uint8_t data_high = (static_cast<bool>(m_Context.data_low & (1 << (offset)))) << 0;
			uint8_t data_low = (static_cast<bool>(m_Context.data_high & (1 << (offset)))) << 1;
			uint8_t colour_index = data_high | data_low;

			PixelData pixel;
			pixel.palette = m_Context.background_window_attribute.colour_palette;
			pixel.colour_index = colour_index;
			pixel.sprite_priority = m_Context.background_window_attribute.priority;

			m_Context.background_queue.push_back(pixel);
		}
	}
}