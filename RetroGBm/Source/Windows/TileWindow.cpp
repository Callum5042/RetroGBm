#include "Windows/TileWindow.h"
#include "Windows/MainWindow.h"
#include "Render/RenderTarget.h"
#include "Render/RenderTexture.h"
#include "Application.h"

#include <RetroGBm/Emulator.h>
#include <RetroGBm/Display.h>
#include <RetroGBm/Ppu.h>
#include <RetroGBm/TileData.h>

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
	// Tiles are drawn with a 1 pixel gap between them
	const int debug_width = static_cast<int>((16 * 8) + (16));
	const int debug_height = static_cast<int>((24 * 8) + (24));

	std::vector<uint32_t> buffer;
	buffer.resize(debug_width * debug_height);
	std::fill(buffer.begin(), buffer.end(), 0x0);

	// Copy VRAM bank 0 tile data ($8000-$97FF)
	std::vector<uint8_t> tile_data(TileData::BytesPerBank);
	for (int i = 0; i < TileData::BytesPerBank; i++)
	{
		tile_data[i] = m_Application->GetEmulator()->GetPpu()->ReadVideoRam(static_cast<uint16_t>(0x8000 + i), 0);
	}

	const std::vector<uint8_t> sheet = TileData::DecodeSheet(tile_data.data());

	for (int y = 0; y < TileData::SheetHeight; y++)
	{
		for (int x = 0; x < TileData::SheetWidth; x++)
		{
			const uint8_t colour_index = sheet[y * TileData::SheetWidth + x];
			const uint32_t colour = m_Application->GetEmulator()->GetDisplay()->GetColourFromBackgroundPalette(0, colour_index);

			// Offset each pixel by its tile column and row to leave the gap
			const int x1 = x + (x / 8);
			const int y1 = y + (y / 8);
			buffer[x1 + (y1 * debug_width)] = colour;
		}
	}

	m_TileRenderTexture->Update(buffer.data(), debug_width * sizeof(uint32_t));
}

void TileWindow::OnClose()
{
	Window::OnClose();

	m_Application->GetMainWindow()->ToggleTileWindowMenuItem(false);
}
