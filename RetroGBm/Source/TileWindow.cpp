#include "TileWindow.h"
#include "MainWindow.h"
#include "Render/RenderTarget.h"
#include "Render/RenderTexture.h"
#include "Application.h"

#include <RetroGBm/Emulator.h>
#include <RetroGBm/Display.h>

#include <vector>

TileWindow::TileWindow(Application* application) : Window(application)
{
}

void TileWindow::CreateTilemapWindow()
{
	// Target
	m_TileRenderTarget = m_Application->GetRenderDevice()->CreateRenderTarget();
	m_TileRenderTarget->Create(m_Hwnd);
	m_TileRenderTarget->DisableFullscreenAltEnter();

	// Texture
	const int debug_width = static_cast<int>((16 * 8) + (16));
	const int debug_height = static_cast<int>((24 * 8) + (24));
	m_TileRenderTexture = m_Application->GetRenderDevice()->CreateTexture();
	m_TileRenderTexture->Create(debug_width, debug_height);
}

void TileWindow::Update()
{
	if (!m_Application->GetEmulator()->IsRunning())
	{
		return;
	}

	UpdateTilemapTexture();

	m_TileRenderTarget->Clear();
	m_TileRenderTexture->Render();
	m_TileRenderTarget->Present();
}

void TileWindow::UpdateTilemapTexture()
{
	const int debug_width = static_cast<int>((16 * 8) + (16));
	const int debug_height = static_cast<int>((24 * 8) + (64));

	std::vector<uint32_t> buffer;
	buffer.resize(debug_width * debug_height);
	std::fill(buffer.begin(), buffer.end(), 0x0);

	int xDraw = 0;
	int yDraw = 0;
	int tile_number = 0;

	const uint16_t address = 0x8000;

	// 384 tiles, 24 x 16
	for (int y = 0; y < 24; y++)
	{
		for (int x = 0; x < 16; x++)
		{
			// Display tile (16 bytes big - 2bits per pixel)
			for (int tileY = 0; tileY < 16; tileY += 2)
			{
				uint8_t byte1 = m_Application->GetEmulator()->ReadBus(address + (tile_number * 16) + tileY);
				uint8_t byte2 = m_Application->GetEmulator()->ReadBus(address + (tile_number * 16) + tileY + 1);

				for (int bit = 7; bit >= 0; bit--)
				{
					// Get pixel colour from palette
					uint8_t high = (static_cast<bool>(byte1 & (1 << bit))) << 1;
					uint8_t low = (static_cast<bool>(byte2 & (1 << bit))) << 0;
					uint8_t colour_index = high | low;

					uint32_t colour = m_Application->GetEmulator()->GetDisplay()->GetColourFromBackgroundPalette(0, colour_index);

					// Calculate pixel position in buffer
					int x1 = xDraw + (x)+((7 - bit));
					int y1 = yDraw + (y)+(tileY / 2);
					buffer[x1 + (y1 * debug_width)] = colour;
				}
			}

			// Drawing 8 pixels at a time
			xDraw += 8;
			tile_number++;
		}

		// Drawing 8 pixels at a time
		yDraw += 8;
		xDraw = 0;
	}

	m_TileRenderTexture->Update(buffer.data(), debug_width * sizeof(uint32_t));
}

void TileWindow::OnClose()
{
	Window::OnClose();

	m_Application->GetMainWindow()->ToggleTileWindowMenuItem(false);
}