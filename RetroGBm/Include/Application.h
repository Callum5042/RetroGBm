#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <memory>
#include "Window.h"
#include "Renderer.h"
#include "Shader.h"
#include "Model.h"

class Emulator;

class Application
{
public:
	Application();
	virtual ~Application();

	int Start();

	inline Emulator* GetEmulator() { return m_Emulator.get(); }

private:
	void Init();
	void Run();
	/*void HandleEvents(const SDL_Event& e);
	void HandleWindowEvents(const SDL_Event& e);
	void HandleKey(bool state, SDL_Scancode scancode);*/
	std::atomic_bool m_Running = true;

	// Main window
	std::unique_ptr<Window> m_MainWindow = nullptr;
	std::unique_ptr<DX::Renderer> m_MainRenderer = nullptr;
	std::unique_ptr<DX::Shader> m_MainShader = nullptr;
	std::unique_ptr<DX::Model> m_MainTexture = nullptr;

	// Tile window
	std::unique_ptr<Window> m_TileWindow = nullptr;
	std::unique_ptr<DX::Renderer> m_TileRenderer = nullptr;
	std::unique_ptr<DX::Shader> m_TileShader = nullptr;
	std::unique_ptr<DX::Model> m_TileTexture = nullptr;
	void UpdateTilemapTexture();

	float m_TileWindowScale = 4.0f;

	// Emulator
	std::unique_ptr<Emulator> m_Emulator;
};