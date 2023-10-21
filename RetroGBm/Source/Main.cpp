#include <iostream>
#include <memory>
#include "Cartridge.h"
#include "Emulator.h"
#include <Windows.h>
#include <Bus.h>

#include <SDL.h>
#include <thread>

// Useful docs
// https://www.pastraiser.com/cpu/gameboy/gameboy_opcodes.html
// https://rgbds.gbdev.io/docs/v0.5.1/gbz80.7/#HALT
// https://archive.org/details/GameBoyProgManVer1.1/page/n1/mode/2up
// https://gekkio.fi/files/gb-docs/gbctr.pdf

int main(int argc, char** argv)
{
	// Setup SDL
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		SDL_ShowSimpleMessageBox(NULL, "Error", "SDL_Init failed", nullptr);
		return -1;
	}

	SDL_Window* window = SDL_CreateWindow("RetroGBm", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
	if (window == nullptr)
	{
		SDL_ShowSimpleMessageBox(NULL, "Error", "SDL_CreateWindow failed", nullptr);
		return -1;
	}

	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (renderer == nullptr)
	{
		SDL_ShowSimpleMessageBox(NULL, "Error", "SDL_CreateWindow failed", nullptr);
		return -1;
	}

	// Debug texture
	float scale = 4.0f;
	SDL_Texture* texture = SDL_CreateTexture(renderer, 
											 SDL_PIXELFORMAT_ARGB8888, 
											 SDL_TEXTUREACCESS_STREAMING, 
											 static_cast<int>((16 * 8 * scale) + (16 * scale)), 
											 static_cast<int>((32 * 8 * scale) + (64 * scale)));


	SDL_Surface* debugScreen = SDL_CreateRGBSurface(0, (16 * 8 * scale) + (16 * scale),
									   (32 * 8 * scale) + (64 * scale), 32,
									   0x00FF0000,
									   0x0000FF00,
									   0x000000FF,
									   0xFF000000);

	// Setup emulator
	std::unique_ptr<Emulator> emulator = std::make_unique<Emulator>();
	// emulator->LoadRom("Tetris.gb");
	emulator->LoadRom("D:\\Sources\\RetroGBm\\RetroGBm\\Resources\\testroms\\cpu_instrs\\individual\\01-special.gb");
	// emulator->LoadRom("D:\\Sources\\RetroGBm\\RetroGBm\\Resources\\testroms\\instr_timing\\instr_timing.gb");

	bool running = true;

	std::thread t1([&]
	{
		while (running)
		{
			emulator->Tick();
		}
	});

	// Message loop
	try
	{
		while (running)
		{
			SDL_Event e = {};
			if (SDL_PollEvent(&e))
			{
				// Handle inputs
				switch (e.type)
				{
					case SDL_QUIT:
						running = false;
						break;

					default:
						break;
				}
			}
			else
			{
				// Emulator cycle
				/*emulator->Tick();*/

				// Update texture
				int xDraw = 0;
				int yDraw = 0;
				int tileNum = 0;

				SDL_Rect rc;
				rc.x = 0;
				rc.y = 0;
				rc.w = debugScreen->w;
				rc.h = debugScreen->h;
				SDL_FillRect(debugScreen, &rc, 0xFF111111);

				uint16_t addr = 0x8000;

				//384 tiles, 24 x 16
				for (int y = 0; y < 24; y++)
				{
					for (int x = 0; x < 16; x++)
					{
						// display_tile(debugScreen, addr, tileNum, xDraw + (x * scale), yDraw + (y * scale));
						SDL_Rect rc;

						static unsigned long tile_colors[4] = { 0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000 };

						for (int tileY = 0; tileY < 16; tileY += 2)
						{
							uint8_t b1 = ReadFromBus(emulator->GetContext(), (addr + (tileNum * 16) + tileY));
							uint8_t b2 = ReadFromBus(emulator->GetContext(), (addr + (tileNum * 16) + tileY + 1));

							for (int bit = 7; bit >= 0; bit--)
							{
								uint8_t hi = !!(b1 & (1 << bit)) << 1;
								uint8_t lo = !!(b2 & (1 << bit));

								uint8_t color = hi | lo;

								rc.x = xDraw + (x * scale) + ((7 - bit) * scale);
								rc.y = yDraw + (y * scale) + (tileY / 2 * scale);
								rc.w = scale;
								rc.h = scale;

								SDL_FillRect(debugScreen, &rc, tile_colors[color]);
							}
						}



						xDraw += (8 * scale);
						tileNum++;
					}

					yDraw += (8 * scale);
					xDraw = 0;
				}

				SDL_UpdateTexture(texture, NULL, debugScreen->pixels, debugScreen->pitch);

				// Render
				SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
				SDL_RenderClear(renderer);

				SDL_RenderCopy(renderer, texture, NULL, NULL);

				SDL_RenderPresent(renderer);
			}
		}
	}
	catch (const std::exception& ex)
	{
		// TODO: Update this to use a properly logging framework then dealing with Win32 stuff
		HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

		// Get currently console window attributes
		CONSOLE_SCREEN_BUFFER_INFO console_info;
		GetConsoleScreenBufferInfo(hConsole, &console_info);

		// Set to red and print error
		SetConsoleTextAttribute(hConsole, FOREGROUND_INTENSITY | FOREGROUND_RED);
		std::cerr << std::format("Fatal error (0x{:x}): {}", emulator->GetOpCode(), ex.what()) << '\n';

		// Reset attributes
		SetConsoleTextAttribute(hConsole, console_info.wAttributes);
	}

	t1.join();

	// Clean up and exit
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}