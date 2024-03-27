#pragma once

#include <memory>
#include <thread>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "Windows/MainWindow.h"
#include "Windows/TileWindow.h"
#include "Windows/CartridgeInfoWindow.h"

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

	static Application* Instance;

	int Start();

	inline Emulator* GetEmulator() { return m_Emulator.get(); }
	inline Render::RenderDevice* GetRenderDevice() { return m_RenderDevice.get(); }

	void LoadRom(const std::string& file);
	void StopEmulator();

	void CreateTileWindow();
	void CloseTileWindow();

	void CreateCartridgeInfoWindow();
	void ReleaseCartridgeInfoWindow();

	inline MainWindow* GetMainWindow() const { return m_MainWindow.get(); }

	void SaveState();
	void LoadState();

private:
	void Init();
	void Run();
	std::atomic_bool m_Running = true;

	// Main window
	std::unique_ptr<MainWindow> m_MainWindow = nullptr;
	void CreateMainWindow();

	// Tile window
	std::unique_ptr<TileWindow> m_TileWindow = nullptr;

	// Cartridge window
	std::unique_ptr<CartridgeInfoWindow> m_CartridgeInfoWindow = nullptr;

	// Emulator
	std::unique_ptr<Emulator> m_Emulator;
	std::thread m_EmulatorThread;

	// Rendering
	std::unique_ptr<Render::RenderDevice> m_RenderDevice = nullptr;
	std::unique_ptr<Render::RenderShader> m_RenderShader = nullptr;
};