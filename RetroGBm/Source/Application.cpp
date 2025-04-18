#include "Application.h"
#include <thread>
#include <exception>

#include <format>
#include <string>
#include <filesystem>
#include <chrono>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <RetroGBm/Emulator.h>
#include <RetroGBm/Joypad.h>
#include <RetroGBm/Logger.h>

namespace
{
	std::string getISODateTime()
	{
		using namespace std::chrono;

		// Get current time
		auto now = system_clock::now();

		// Extract milliseconds
		auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

		// Convert to time_t for formatting
		auto in_time_t = system_clock::to_time_t(now);

		// Get local time
		std::tm buf;
#ifdef _WIN32
		localtime_s(&buf, &in_time_t); // Windows
#else
		localtime_r(&in_time_t, &buf); // POSIX (Linux/macOS)
#endif

		std::ostringstream ss;
		ss << std::put_time(&buf, "%Y-%m-%dT%H:%M:%S");
		ss << '.' << std::setw(3) << std::setfill('0') << ms.count();

		// Get timezone offset (e.g. +0100)
		char tz[6];
		std::strftime(tz, sizeof(tz), "%z", &buf);
		ss << tz;

		return ss.str();
	}
}

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

	m_Emulator = std::make_unique<Emulator>(m_DisplayOutput.get(), SoundOutput.get());

	std::filesystem::path battery_path = "RomData";
	std::filesystem::create_directories(battery_path);
	std::string filename = std::filesystem::path(file).filename().string();
	battery_path.append(filename + ".save");
	m_Emulator->SetBatteryPath(battery_path.string());

	Logger::Info("Loading ROM file: " + file);
	m_Emulator->ToggleTraceLog(tracelog);
	m_Emulator->LoadRom(file);

	// Set start time
	CurrentTimeStamp = std::chrono::high_resolution_clock::now();
	CurrentFilename = std::filesystem::path(file).filename().string();

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
	if (!m_Emulator->IsRunning())
	{
		return;
	}

	m_Emulator->Stop();
	if (m_EmulatorThread.joinable())
	{
		m_EmulatorThread.join();
	}

	Logger::Info("Emulator stopped");

	// Calculate play time
	auto end_time = std::chrono::high_resolution_clock::now();
	int play_time_minutes = std::chrono::duration_cast<std::chrono::minutes>(end_time - CurrentTimeStamp).count();

	// Set last played date
	std::string today_date = getISODateTime();

	// Find the game profile entry
	bool found_entry = false;
	for (auto& profileData : ProfileDataList)
	{
		if (profileData.filename == CurrentFilename)
		{
			profileData.totalPlayTimeMinutes += play_time_minutes;
			profileData.lastPlayed = today_date;
			found_entry = true;
			break;
		}
	}

	// If we don't find a profile entry then add one
	if (!found_entry)
	{
		ProfileData new_data;
		new_data.filename = CurrentFilename;
		new_data.totalPlayTimeMinutes += play_time_minutes;
		new_data.lastPlayed = today_date;

		ProfileDataList.push_back(new_data);
	}

	// Save profile
	SaveProfile("profile.json", ProfileDataList);
}

void Application::Run()
{
	m_Emulator = std::make_unique<Emulator>(m_DisplayOutput.get(), SoundOutput.get());

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

				// Control audio playback speed
				if (m_Emulator->GetFPS() < 60)
				{
					float playback_ratio = m_Emulator->GetFPS() / 60.0f;
					SoundOutput->SetFrequencyRatio(playback_ratio);
				}
				else
				{
					SoundOutput->SetFrequencyRatio(1.0f);
				}

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
	// Load the profile json
	ProfileDataList = ParseProfile("profile.json");

	// Render device
	m_RenderDevice = std::make_unique<Render::RenderDevice>();
	m_RenderDevice->Create();

	m_RenderShader = std::make_unique<Render::RenderShader>(m_RenderDevice.get());
	m_RenderShader->Create();

	// Create windows
	CreateMainWindow();

	// Initialize audio
	m_DisplayOutput = std::make_unique<DisplayOutput>();
	SoundOutput = std::make_unique<XAudio2Output>();
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