#pragma once

#include <memory>
#include <thread>
#include <vector>
#include <chrono>

#include "Windows/MainWindow.h"
#include "Windows/TileWindow.h"
#include "Windows/CartridgeInfoWindow.h"
#include "Windows/CpuRegistersWindow.h"

#include "Render/RenderDevice.h"
#include "Render/RenderShader.h"
#include "Render/RenderTarget.h"
#include "Render/RenderTexture.h"

#include "Audio/XAudio2Output.h"
#include "Render/DisplayOutput.h"

#include "ProfileParser.h"

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
	inline CartridgeInfoWindow* GetCartridgeInfoWindow() const { return m_CartridgeInfoWindow.get(); }

	void SaveState(const std::string& path);
	void LoadState(const std::string& path);
	
	// Tools Windows
	std::unique_ptr<CpuRegisterWindow> CpuRegistersWindow = nullptr;

	// Profile Data
	std::vector<ProfileData> ProfileDataList;
	std::chrono::steady_clock::time_point CurrentTimeStamp;
	std::string CurrentFilename;
	std::string Checksum;

	// Audio
	std::unique_ptr<XAudio2Output> SoundOutput = nullptr;
	std::unique_ptr<DisplayOutput> m_DisplayOutput = nullptr;

	// Rendering
	std::unique_ptr<Render::RenderDevice> m_RenderDevice = nullptr;
	std::unique_ptr<Render::RenderShader> m_RenderShader = nullptr;

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
};