#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <memory>
#include "Window.h"
#include "MainWindow.h"

#include "Render/RenderDevice.h"
#include "Render/RenderShader.h"
#include "Render/RenderTarget.h"
#include "Render/RenderTexture.h"

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
	std::atomic_bool m_Running = true;

	// Main window
	std::unique_ptr<MainWindow> m_MainWindow = nullptr;
	std::unique_ptr<Render::RenderTarget> m_MainRenderTarget = nullptr;
	std::unique_ptr<Render::RenderTexture> m_MainRenderTexture = nullptr;
	void CreateMainWindow();

	// Tile window
	std::unique_ptr<Window> m_TileWindow = nullptr;
	std::unique_ptr<Render::RenderTarget> m_TileRenderTarget = nullptr;
	std::unique_ptr<Render::RenderTexture> m_TileRenderTexture = nullptr;
	void CreateTilemapWindow();
	void UpdateTilemapTexture();

	float m_TileWindowScale = 4.0f;

	// Emulator
	std::unique_ptr<Emulator> m_Emulator;

	// Rendering
	std::unique_ptr<Render::RenderDevice> m_RenderDevice = nullptr;
	std::unique_ptr<Render::RenderShader> m_RenderShader = nullptr;
};