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
	m_Emulator = std::make_unique<Emulator>();
	//m_Emulator->LoadRom("D:\\Sources\\RetroGBm\\RetroGBm\\Resources\\Tetris.gb");
	m_Emulator->LoadRom("D:\\Sources\\RetroGBm\\RetroGBm\\Resources\\testroms\\cpu_instrs\\individual\\02-interrupts.gb");
	// m_Emulator->LoadRom("D:\\Sources\\RetroGBm\\RetroGBm\\Resources\\testroms\\cpu_instrs\\cpu_instrs.gb");

	std::thread emulator_thread([&]
	{
		while (m_Running && m_Emulator->IsRunning())
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

			// Display main window
			SDL_SetRenderDrawColor(m_MainRenderer, 0, 0, 0, 255);
			SDL_RenderClear(m_MainRenderer);
			SDL_RenderCopy(m_MainRenderer, m_MainTexture, NULL, NULL);
			SDL_RenderPresent(m_MainRenderer);

			// Display tile window
			SDL_SetRenderDrawColor(m_TileRenderer, 0, 0, 0, 255);
			SDL_RenderClear(m_TileRenderer);
			SDL_RenderCopy(m_TileRenderer, m_TileTexture, NULL, NULL);
			SDL_RenderPresent(m_TileRenderer);
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

		case SDL_KEYDOWN:
			HandleKey(true, e.key.keysym.scancode);
			break;

		case SDL_KEYUP:
			HandleKey(false, e.key.keysym.scancode);
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
	const int debug_width = (16 * 8 * m_TileWindowScale) + (16 * m_TileWindowScale);
	const int debug_height = (24 * 8 * m_TileWindowScale) + (64 * m_TileWindowScale);

	m_TileWindow = SDL_CreateWindow("RetroGBm Tilemap", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 16 * 8 * m_TileWindowScale, 24 * 8 * m_TileWindowScale, SDL_WINDOW_SHOWN);
	if (m_TileWindow == nullptr)
	{
		throw std::exception("SDL_CreateWindow failed");
	}

	m_TileRenderer = SDL_CreateRenderer(m_TileWindow, -1, SDL_RENDERER_ACCELERATED);
	if (m_TileRenderer == nullptr)
	{
		throw std::exception("SDL_CreateRenderer failed");
	}

	m_TileTexture = SDL_CreateTexture(m_TileRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, debug_width, debug_height);
	m_TileSurface = SDL_CreateRGBSurface(0, debug_width, debug_height, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
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

	// 384 tiles, 24 x 16
	for (int y = 0; y < 24; y++)
	{
		for (int x = 0; x < 16; x++)
		{
			// Display tile
			const unsigned long tile_colours[4] = { 0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000 };

			SDL_Rect rc = {};
			for (int tileY = 0; tileY < 16; tileY += 2)
			{
				uint8_t b1 = m_Emulator->ReadBus(addr + (tileNum * 16) + tileY);
				uint8_t b2 = m_Emulator->ReadBus(addr + (tileNum * 16) + tileY + 1);

				for (int bit = 7; bit >= 0; bit--)
				{
					uint8_t hi = !!(b1 & (1 << bit)) << 1;
					uint8_t lo = !!(b2 & (1 << bit));

					uint8_t color = hi | lo;

					rc.x = xDraw + (x * m_TileWindowScale) + ((7 - bit) * m_TileWindowScale);
					rc.y = yDraw + (y * m_TileWindowScale) + (tileY / 2 * m_TileWindowScale);
					rc.w = m_TileWindowScale;
					rc.h = m_TileWindowScale;

					SDL_FillRect(m_TileSurface, &rc, tile_colours[color]);
				}
			}

			xDraw += (8 * m_TileWindowScale);
			tileNum++;
		}

		yDraw += (8 * m_TileWindowScale);
		xDraw = 0;
	}

	SDL_UpdateTexture(m_TileTexture, NULL, m_TileSurface->pixels, m_TileSurface->pitch);
}

void Application::UpdateMainWindow()
{
	SDL_Rect rc = {};
	rc.x = rc.y = 0;
	rc.w = rc.h = 2048;

	auto& video_buffer = Emulator::Instance->GetPpu()->context.video_buffer;
	int screen_resolution_x = Emulator::Instance->GetPpu()->ScreenResolutionX;
	int screen_resolution_y = Emulator::Instance->GetPpu()->ScreenResolutionY;

	for (int line_num = 0; line_num < screen_resolution_y; line_num++)
	{
		for (int x = 0; x < screen_resolution_x; x++)
		{
			rc.x = x * m_TileWindowScale;
			rc.y = line_num * m_TileWindowScale;
			rc.w = m_TileWindowScale;
			rc.h = m_TileWindowScale;

			SDL_FillRect(m_MainSurface, &rc, video_buffer[x + (line_num * screen_resolution_x)]);
		}
	}

	SDL_UpdateTexture(m_MainTexture, NULL, m_MainSurface->pixels, m_MainSurface->pitch);
}

void Application::HandleKey(bool state, SDL_Scancode scancode)
{
	switch (scancode)
	{
		case SDL_SCANCODE_Z:
			m_Emulator->GetJoypad()->SetJoypad(JoypadButton::B, state);
			break;
		case SDL_SCANCODE_X:
			m_Emulator->GetJoypad()->SetJoypad(JoypadButton::A, state);
			break;
		case SDL_SCANCODE_RETURN:
			m_Emulator->GetJoypad()->SetJoypad(JoypadButton::Start, state);
			break;
		case SDL_SCANCODE_TAB:
			m_Emulator->GetJoypad()->SetJoypad(JoypadButton::Select, state);
			break;
		case SDL_SCANCODE_UP:
			m_Emulator->GetJoypad()->SetJoypad(JoypadButton::Up, state);
			break;
		case SDL_SCANCODE_DOWN:
			m_Emulator->GetJoypad()->SetJoypad(JoypadButton::Down, state);
			break;
		case SDL_SCANCODE_LEFT:
			m_Emulator->GetJoypad()->SetJoypad(JoypadButton::Left, state);
			break;
		case SDL_SCANCODE_RIGHT:
			m_Emulator->GetJoypad()->SetJoypad(JoypadButton::Right, state);
			break;
	}
}