#include "Application.h"
#include <thread>
#include <exception>

#include <iostream>
#include <format>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <Bus.h>

Application::Application()
{
}

Application::~Application()
{
	// Cleanup main window
	SDL_DestroyRenderer(m_MainRenderer);
	SDL_DestroyWindow(m_MainWindow);

	// Cleanup tile window
	SDL_DestroyRenderer(m_TileRenderer);
	SDL_DestroyWindow(m_TileWindow);

	// Cleanup SDL
	SDL_Quit();
}

int Application::Start()
{
	try
	{
		Init();
		Run();
	}
	catch (const std::exception& e)
	{
		HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);

		// Get currently console window attributes
		CONSOLE_SCREEN_BUFFER_INFO console_info;
		GetConsoleScreenBufferInfo(console_handle, &console_info);

		// Set to red and print error
		SetConsoleTextAttribute(console_handle, FOREGROUND_INTENSITY | FOREGROUND_RED);
		std::cerr << std::format("Fatal error (0x{:x}): {}", m_Emulator->GetOpCode(), e.what()) << '\n';

		// Reset attributes
		SetConsoleTextAttribute(console_handle, console_info.wAttributes);

		// Display error window
		SDL_ShowSimpleMessageBox(NULL, "Error", e.what(), nullptr);
		return -1;
	}

	return 0;
}

void Application::Run()
{
	// Emulator runs on a background thread
	std::thread emulator_thread([&]
	{
		m_Emulator = std::make_unique<Emulator>();
		m_Emulator->LoadRom("D:\\Sources\\RetroGBm\\RetroGBm\\Resources\\testroms\\cpu_instrs\\individual\\01-special.gb");

		while (m_Running)
		{
			m_Emulator->Tick();
		}
	});

	// UI runs on main thread
	while (m_Running)
	{
		SDL_Event e = {};
		if (SDL_PollEvent(&e))
		{
			HandleEvents(e);
		}
		else
		{
			// Update windows
			UpdateMainWindow();
			UpdateTileWindow();

			// Display tile window
			SDL_SetRenderDrawColor(m_TileRenderer, 0, 0, 0, 255);
			SDL_RenderClear(m_TileRenderer);
			SDL_RenderCopy(m_TileRenderer, m_TileTexture, NULL, NULL);
			SDL_RenderPresent(m_TileRenderer);

			// Display main window
			SDL_SetRenderDrawColor(m_MainRenderer, 0, 0, 0, 255);
			SDL_RenderClear(m_MainRenderer);
			SDL_RenderCopy(m_MainRenderer, m_MainTexture, NULL, NULL);
			SDL_RenderPresent(m_MainRenderer);
		}
	}

	emulator_thread.join();
}

void Application::HandleEvents(const SDL_Event& e)
{
	switch (e.type)
	{
		case SDL_QUIT:
			m_Running = false;
			break;

		case SDL_WINDOWEVENT:
			HandleWindowEvents(e);
			break;

		default:
			break;
	}
}

void Application::HandleWindowEvents(const SDL_Event& e)
{
	switch (e.window.event)
	{
		case SDL_WINDOWEVENT_CLOSE:
			m_Running = false;
			break;
	}
}

void Application::Init()
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		throw std::exception("SDL_Init failed");
	}

	CreateMainWindow();
	CreateTileWindow();
}

void Application::CreateMainWindow()
{
	m_MainWindow = SDL_CreateWindow("RetroGBm", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
	if (m_MainWindow == nullptr)
	{
		throw std::exception("SDL_CreateWindow failed");
	}

	m_MainRenderer = SDL_CreateRenderer(m_MainWindow, -1, SDL_RENDERER_ACCELERATED);
	if (m_MainRenderer == nullptr)
	{
		throw std::exception("SDL_CreateRenderer failed");
	}

	m_MainTexture = SDL_CreateTexture(m_MainRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 800, 600);
	m_MainSurface = SDL_CreateRGBSurface(0, 800, 600, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
}

void Application::CreateTileWindow()
{
	const int window_width = static_cast<int>(16 * 8 * m_TileWindowScale);
	const int window_height = static_cast<int>(32 * 8 * m_TileWindowScale);

	m_TileWindow = SDL_CreateWindow("RetroGBm - Tile Data", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_width, window_height, SDL_WINDOW_SHOWN);
	if (m_TileWindow == nullptr)
	{
		throw std::exception("SDL_CreateWindow failed");
	}

	m_TileRenderer = SDL_CreateRenderer(m_TileWindow, -1, SDL_RENDERER_ACCELERATED);
	if (m_TileRenderer == nullptr)
	{
		throw std::exception("SDL_CreateRenderer failed");
	}

	const int texture_width = static_cast<int>((16 * 8 * m_TileWindowScale) + (16 * m_TileWindowScale));
	const int texture_height = static_cast<int>((32 * 8 * m_TileWindowScale) + (64 * m_TileWindowScale));

	m_TileTexture = SDL_CreateTexture(m_TileRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, texture_width, texture_height);
	m_TileSurface = SDL_CreateRGBSurface(0, texture_width, texture_height, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
}

void Application::UpdateTileWindow()
{
	int draw_x = 0;
	int draw_y = 0;
	int tile_number = 0;

	SDL_Rect rc = {};
	rc.x = 0;
	rc.y = 0;
	rc.w = m_TileSurface->w;
	rc.h = m_TileSurface->h;
	SDL_FillRect(m_TileSurface, &rc, 0xFF111111);

	uint16_t address = 0x8000;
	const unsigned long tile_colors[4] = { 0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000 };

	// 384 tiles, 24 x 16
	for (int y = 0; y < 24; y++)
	{
		for (int x = 0; x < 16; x++)
		{
			SDL_Rect rc = {};

			for (int tileY = 0; tileY < 16; tileY += 2)
			{
				uint8_t b1 = ReadFromBus(m_Emulator->GetContext(), (address + (tile_number * 16) + tileY));
				uint8_t b2 = ReadFromBus(m_Emulator->GetContext(), (address + (tile_number * 16) + tileY + 1));

				for (int bit = 7; bit >= 0; bit--)
				{
					uint8_t hi = !!(b1 & (1 << bit)) << 1;
					uint8_t lo = !!(b2 & (1 << bit));

					uint8_t color = hi | lo;

					rc.x = static_cast<int>(draw_x + (x * m_TileWindowScale) + ((7 - bit) * m_TileWindowScale));
					rc.y = static_cast<int>(draw_y + (y * m_TileWindowScale) + (tileY / 2.0f * m_TileWindowScale));
					rc.w = static_cast<int>(m_TileWindowScale);
					rc.h = static_cast<int>(m_TileWindowScale);

					SDL_FillRect(m_TileSurface, &rc, tile_colors[color]);
				}
			}

			draw_x += static_cast<int>(8 * m_TileWindowScale);
			tile_number++;
		}

		draw_y += static_cast<int>(8 * m_TileWindowScale);
		draw_x = 0;
	}

	SDL_UpdateTexture(m_TileTexture, NULL, m_TileSurface->pixels, m_TileSurface->pitch);
}

void Application::UpdateMainWindow()
{
	SDL_Rect rc = {};
	rc.x = rc.y = 0;
	rc.w = rc.h = 2048;

	auto& video_buffer = m_Emulator->GetContext()->ppu_context.video_buffer;

	const int YRES = 144;
	const int XRES = 160;

	int scale = 4;
	for (int line_num = 0; line_num < YRES; line_num++)
	{
		for (int x = 0; x < XRES; x++)
		{
			rc.x = x * scale;
			rc.y = line_num * scale;
			rc.w = scale;
			rc.h = scale;

			SDL_FillRect(m_MainSurface, &rc, video_buffer[x + (line_num * XRES)]);
		}
	}

	SDL_UpdateTexture(m_MainTexture, NULL, m_MainSurface->pixels, m_MainSurface->pitch);
}