#pragma once

#include <SDL.h>
#include <Emulator.h>
#include <memory>

class Application
{
public:
	Application();
	virtual ~Application();

	int Start();

private:
	void Init();
	void Run();
	void HandleEvents(const SDL_Event& e);
	void HandleWindowEvents(const SDL_Event& e);
	void HandleKey(bool state, SDL_Scancode scancode);
	std::atomic_bool m_Running = true;

	// Main window
	void CreateMainWindow();
	void UpdateMainWindow();
	SDL_Window* m_MainWindow = nullptr;
	SDL_Renderer* m_MainRenderer = nullptr;
	SDL_Texture* m_MainTexture = nullptr;
	SDL_Surface* m_MainSurface = nullptr;

	// Tile window
	void CreateTileWindow();
	void UpdateTileWindow();
	SDL_Window* m_TileWindow = nullptr;
	SDL_Renderer* m_TileRenderer = nullptr;
	SDL_Texture* m_TileTexture = nullptr;
	SDL_Surface* m_TileSurface = nullptr;
	float m_TileWindowScale = 4.0f;

	// Emulator
	std::unique_ptr<Emulator> m_Emulator;
};