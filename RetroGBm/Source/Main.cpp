#include <iostream>
#include <memory>
#include "Cartridge.h"
#include "Emulator.h"
#include <Windows.h>

#include <SDL.h>

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
	SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, (16 * 8 * scale) + (16 * scale), (32 * 8 * scale) + (64 * scale));

	// Setup emulator
	std::unique_ptr<Emulator> emulator = std::make_unique<Emulator>();
	// emulator->LoadRom("Tetris.gb");
	//emulator->LoadRom("D:\\Sources\\RetroGBm\\RetroGBm\\Resources\\testroms\\cpu_instrs\\individual\\01-special.gb");
	emulator->LoadRom("D:\\Sources\\RetroGBm\\RetroGBm\\Resources\\testroms\\cpu_instrs\\individual\\07-jr,jp,call,ret,rst.gb");

	// Message loop
	try
	{
		bool running = true;
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
				emulator->Tick();

				// Render
				SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
				SDL_RenderClear(renderer);
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

	// Clean up and exit
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}