#include "Application.h"
#include <thread>
#include <exception>

#include <format>
#include <string>
#include <algorithm>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <RetroGBm/Emulator.h>
#include <RetroGBm/Joypad.h>
#include <RetroGBm/Logger.h>

Application* Application::Instance = nullptr;

Application::Application()
{
	if (Instance != nullptr)
	{
		throw std::exception("There can only be 1 instance created");
	}

	Instance = this;
}

Application::~Application()
{
}

int Application::Start()
{
	try
	{
		Logger::Initialise(LOG_INFO);
		Logger::Info("RetroGBm starting");

		Init();
		Run();
	}
	catch (const std::exception& e)
	{
		// Log the error
		uint8_t opcode = (m_Emulator != nullptr ? m_Emulator->GetOpCode() : 0);
		Logger::Critical(std::format("Fatal error (0x{:x}): {}", opcode, e.what()));

		// Display error window
		MessageBoxA(NULL, e.what(), "Error", MB_OK);

		return -1;
	}

	Logger::Info("RetroGBm closed");
	return 0;
}

void Application::LoadRom(const std::string& file)
{
	StopEmulator();
	bool tracelog = m_Emulator->IsTraceLogEnabled();

	Logger::Info("Loading ROM file: " + file);

	m_Emulator = std::make_unique<Emulator>();
	m_Emulator->ToggleTraceLog(tracelog);
	m_Emulator->LoadRom(file);

	// Emulator runs on a background thread
	m_EmulatorThread = std::thread([&]
	{
		try
		{
			while (m_Emulator->IsRunning())
			{
				m_Emulator->Tick();
			}
		}
		catch (const std::exception& ex)
		{
			m_Emulator->Stop();

			Logger::Critical(std::format("Fatal error (0x{:x}): {}", m_Emulator->GetOpCode(), ex.what()));
			MessageBoxA(NULL, ex.what(), "Error", MB_OK | MB_ICONERROR);
		}
	});
}

void Application::StopEmulator()
{
	// Stop the emulator
	m_Emulator->Stop();
	if (m_EmulatorThread.joinable())
	{
		m_EmulatorThread.join();
	}

	Logger::Info("Emulator stopped");
}

void Application::Run()
{
	m_Emulator = std::make_unique<Emulator>();

	// UI runs on main thread
	while (m_Running)
	{
		MSG msg = {};
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				m_Running = false;
			}

			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		else
		{
			// Apply shader
			m_RenderShader->Use();

			if (m_Emulator->IsRunning())
			{
				// Update main window
				m_MainWindow->Update();

				// Render debug window
				if (m_TileWindow != nullptr)
				{
					m_TileWindow->Update();
				}

				// CPU register window
				if (CpuRegistersWindow != nullptr)
				{
					CpuRegistersWindow->Update();
				}
			}
			else
			{
				m_MainWindow->Update();
			}
		}
	}

	this->StopEmulator();
}

void Application::Init()
{
	// Render device
	m_RenderDevice = std::make_unique<Render::RenderDevice>();
	m_RenderDevice->Create();

	m_RenderShader = std::make_unique<Render::RenderShader>(m_RenderDevice.get());
	m_RenderShader->Create();

	// Create windows
	CreateMainWindow();
}

void Application::CreateMainWindow()
{
	// Window
	m_MainWindow = std::make_unique<MainWindow>();
	m_MainWindow->Create("RetroGBm", 800, 600);
}

void Application::CreateTileWindow()
{
	m_TileWindow = std::make_unique<TileWindow>(this);

	const int window_width = static_cast<int>(16 * 8 * m_TileWindow->GetTileWindowScale());
	const int window_height = static_cast<int>(24 * 8 * m_TileWindow->GetTileWindowScale());
	m_TileWindow->Create("RetroGBm Tiledata", window_width, window_height);
	m_TileWindow->CreateTilemapWindow();
}

void Application::CloseTileWindow()
{
	m_TileWindow.reset();
	m_TileWindow = nullptr;
}

void Application::CreateCartridgeInfoWindow()
{
	if (m_CartridgeInfoWindow == nullptr)
	{
		m_CartridgeInfoWindow = std::make_unique<CartridgeInfoWindow>(this);
		m_CartridgeInfoWindow->Create("Cartridge Info", 400, 300);
	}
}

void Application::ReleaseCartridgeInfoWindow()
{
	m_CartridgeInfoWindow.release();
}

void Application::SaveState(const std::string& path)
{
	if (m_Emulator->IsRunning())
	{
		m_Emulator->SaveState(path);
	}
}

void Application::LoadState(const std::string& path)
{
	if (m_Emulator->IsRunning())
	{
		try
		{
			m_Emulator->LoadState(path);
		}
		catch (const std::exception& ex)
		{
			MessageBoxA(NULL, ex.what(), "Error", MB_OK | MB_ICONERROR);
		}
	}
}