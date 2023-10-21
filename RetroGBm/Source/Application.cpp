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

	// TODO: Refactor
	/*SDL_Texture* m_TileTexture = SDL_CreateTexture(m_TileRenderer,
											 SDL_PIXELFORMAT_ARGB8888,
											 SDL_TEXTUREACCESS_STREAMING,
											 static_cast<int>((16 * 8 * m_TileWindowScale) + (16 * m_TileWindowScale)),
											 static_cast<int>((32 * 8 * m_TileWindowScale) + (64 * m_TileWindowScale)));


	SDL_Surface* m_TileSurface = SDL_CreateRGBSurface(0, (16 * 8 * m_TileWindowScale) + (16 * m_TileWindowScale),
													(32 * 8 * m_TileWindowScale) + (64 * m_TileWindowScale), 32,
													0x00FF0000,
													0x0000FF00,
													0x000000FF,
													0xFF000000);*/

	// END TODO: Refactor

	// UI runs on main thread
	while (m_Running)
	{
		SDL_Event e = {};
		if (SDL_PollEvent(&e))
		{
			// Handle inputs
			switch (e.type)
			{
				case SDL_QUIT:
					m_Running = false;
					break;

				default:
					break;
			}
		}
		else
		{
			// Update tile window
			UpdateTileWindow();

			// Display tile window
			SDL_SetRenderDrawColor(m_TileRenderer, 0, 0, 0, 255);
			SDL_RenderClear(m_TileRenderer);
			SDL_RenderCopy(m_TileRenderer, m_TileTexture, NULL, NULL);
			SDL_RenderPresent(m_TileRenderer);

			// Display main window
			SDL_SetRenderDrawColor(m_MainRenderer, 0, 0, 0, 255);
			SDL_RenderClear(m_MainRenderer);
			// SDL_RenderCopy(m_MainRenderer, texture, NULL, NULL);
			SDL_RenderPresent(m_MainRenderer);
		}
	}

	emulator_thread.join();
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
	int xDraw = 0;
	int yDraw = 0;
	int tileNum = 0;

	SDL_Rect rc = {};
	rc.x = 0;
	rc.y = 0;
	rc.w = m_TileSurface->w;
	rc.h = m_TileSurface->h;
	SDL_FillRect(m_TileSurface, &rc, 0xFF111111);

	uint16_t addr = 0x8000;

	//384 tiles, 24 x 16
	for (int y = 0; y < 24; y++)
	{
		for (int x = 0; x < 16; x++)
		{
			SDL_Rect rc = {};

			static unsigned long tile_colors[4] = { 0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000 };

			for (int tileY = 0; tileY < 16; tileY += 2)
			{
				uint8_t b1 = ReadFromBus(m_Emulator->GetContext(), (addr + (tileNum * 16) + tileY));
				uint8_t b2 = ReadFromBus(m_Emulator->GetContext(), (addr + (tileNum * 16) + tileY + 1));

				for (int bit = 7; bit >= 0; bit--)
				{
					uint8_t hi = !!(b1 & (1 << bit)) << 1;
					uint8_t lo = !!(b2 & (1 << bit));

					uint8_t color = hi | lo;

					rc.x = static_cast<int>(xDraw + (x * m_TileWindowScale) + ((7 - bit) * m_TileWindowScale));
					rc.y = static_cast<int>(yDraw + (y * m_TileWindowScale) + (tileY / 2.0f * m_TileWindowScale));
					rc.w = static_cast<int>(m_TileWindowScale);
					rc.h = static_cast<int>(m_TileWindowScale);

					SDL_FillRect(m_TileSurface, &rc, tile_colors[color]);
				}
			}

			xDraw += static_cast<int>(8 * m_TileWindowScale);
			tileNum++;
		}

		yDraw += static_cast<int>(8 * m_TileWindowScale);
		xDraw = 0;
	}

	SDL_UpdateTexture(m_TileTexture, NULL, m_TileSurface->pixels, m_TileSurface->pitch);
}