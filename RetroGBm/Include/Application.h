#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <memory>
#include "Window.h"
#include "Renderer.h"

class Emulator;

class Application
{
public:
	Application();
	virtual ~Application();

	int Start();

	// Handle messages
	LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	void Init();
	void Run();
	/*void HandleEvents(const SDL_Event& e);
	void HandleWindowEvents(const SDL_Event& e);
	void HandleKey(bool state, SDL_Scancode scancode);*/
	std::atomic_bool m_Running = true;

	// Main window
	Window* m_MainWindow = nullptr;
	DX::Renderer* m_MainRenderer = nullptr;

	/*void CreateMainWindow();
	void UpdateMainWindow();
	SDL_Window* m_MainWindow = nullptr;
	SDL_Renderer* m_MainRenderer = nullptr;
	SDL_Texture* m_MainTexture = nullptr;
	SDL_Surface* m_MainSurface = nullptr;*/

	// Tile window
	Window* m_TileWindow = nullptr;
	DX::Renderer* m_TileRenderer = nullptr;

	/*void CreateTileWindow();
	void UpdateTileWindow();
	SDL_Window* m_TileWindow = nullptr;
	SDL_Renderer* m_TileRenderer = nullptr;
	SDL_Texture* m_TileTexture = nullptr;
	SDL_Surface* m_TileSurface = nullptr;
	float m_TileWindowScale = 4.0f;*/

	float m_TileWindowScale = 4.0f;

	// Emulator
	std::unique_ptr<Emulator> m_Emulator;
};