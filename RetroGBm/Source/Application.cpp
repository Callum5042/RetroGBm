#include "Application.h"
#include <thread>
#include <exception>

#include <iostream>
#include <format>
#include <string>
#include <algorithm>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <Emulator.h>
#include <Joypad.h>

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
		MessageBoxA(NULL, e.what(), "Error", MB_OK);

		return -1;
	}

	return 0;
}

void Application::LoadRom(const std::string& file)
{
	StopEmulator();
	bool tracelog = m_Emulator->IsTraceLogEnabled();

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
			MessageBoxA(NULL, ex.what(), "Error", MB_OK | MB_ICONERROR);
		}
	});
}

void Application::StopEmulator()
{
	m_Emulator->Stop();
	if (m_EmulatorThread.joinable())
	{
		m_EmulatorThread.join();
	}
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
			}
			else
			{
				m_MainWindow->Update();
			}
		}
	}

	m_Emulator->Stop();
	if (m_EmulatorThread.joinable())
	{
		m_EmulatorThread.join();
	}
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

void Application::SaveState()
{
	if (m_Emulator->IsRunning())
	{
		m_Emulator->SaveState("");
	}
}

void Application::LoadState()
{
	if (m_Emulator->IsRunning())
	{
		m_Emulator->LoadState("");
	}
}