#include "Windows/MainWindow.h"
#include "Windows/CartridgeInfoWindow.h"
#include "Application.h"
#include "Utilities/Utilities.h"
#include "../resource.h"
#include "Audio/XAudio2Output.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <RetroGBm/Emulator.h>
#include <RetroGBm/Joypad.h>
#include <RetroGBm/SaveStateHeader.h>

#include <format>
#include <string>
#include <vector>
#include <shobjidl.h>
#include <filesystem>
#include <chrono>
#include <sstream>

namespace
{
	MainWindow* GetWindow(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		MainWindow* window = nullptr;
		if (uMsg == WM_NCCREATE)
		{
			CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
			window = reinterpret_cast<MainWindow*>(pCreate->lpCreateParams);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
		}
		else
		{
			window = reinterpret_cast<MainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		}

		return window;
	}

	LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		MainWindow* window = GetWindow(hwnd, msg, wParam, lParam);
		if (window == nullptr)
		{
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}

		return window->HandleMessage(hwnd, msg, wParam, lParam);
	}
}

MainWindow::MainWindow()
{
	m_Application = Application::Instance;

	m_Timer.Start();
}

MainWindow::~MainWindow()
{
	if (m_Hwnd != NULL)
	{
		DestroyWindow(m_Hwnd);
	}

	HINSTANCE hInstance = GetModuleHandle(NULL);
	UnregisterClassW(m_RegisterClassName.c_str(), hInstance);
}

void MainWindow::Create(const std::string& title, int width, int height)
{
	CreateMainWindow(title, width, height);
	CreateMenuBar();
	CreateStatusBar();
	CreateRenderWindow();
	CreateRomListWindow();

	// Statusbar on draw on top of render
	SetWindowPos(m_HwndStatusbar, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

void MainWindow::Update()
{
	// Calculate FPS
	m_Timer.Tick();
	m_FrameCount++;

	if ((m_Timer.TotalTime() - m_TimeElapsed) >= 1.0f)
	{
		m_FramesPerSecond = m_FrameCount;

		// Reset for next average
		m_FrameCount = 0;
		m_TimeElapsed += 1.0f;

		// Update stats FPS
		Emulator* emulator = m_Application->GetEmulator();
		if (emulator->IsRunning())
		{
			this->SetStatusBarStats(std::format("FPS: {} - VPS: {}", m_FramesPerSecond, emulator->GetFPS()));
		}
	}

	// Update texture
	m_MainRenderTarget->Clear();
	if (m_Application->GetEmulator()->IsRunning())
	{
		void* pixels = m_Application->m_DisplayOutput->PixelBuffer;

		if (pixels != nullptr)
		{
			m_MainRenderTexture->Update(pixels, m_Application->GetEmulator()->GetVideoPitch());
			m_MainRenderTexture->Render();
		}
	}

	m_MainRenderTarget->Present();
}

LRESULT MainWindow::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_NOTIFY:
		{
			LPNMHDR nmhdr = (LPNMHDR)lParam;

			// Check if the message is from your ListView (ID = 1)
			if (nmhdr->idFrom == m_ListMenuId && nmhdr->code == NM_DBLCLK)
			{
				LPNMITEMACTIVATE item = (LPNMITEMACTIVATE)lParam;

				if (item->iItem != -1)
				{
					int row = item->iItem;

					// Optional: Get text from that row
					WCHAR buf[256];
					ListView_GetItemText(m_ListHwnd, row, 0, buf, 256);

					std::wstring filename(buf);
					std::filesystem::path rom_path("ROMS");
					rom_path.append(filename);

					this->LoadRom(rom_path.string());
				}
			}
			return 0;
		}
		break;

		case WM_CLOSE:
		{
			if (m_Application->GetEmulator()->IsRunning())
			{
				m_Application->GetEmulator()->Pause(true);

				int result = MessageBox(m_Hwnd, L"Do you want to exit the emulator?", L"Exit Emulator", MB_OKCANCEL | MB_ICONQUESTION);
				if (result == IDOK)
				{
					OnClose();
				}
				else
				{
					m_Application->GetEmulator()->Pause(false);
				}
			}
			else
			{
				PostQuitMessage(0);
			}

			return 0;
		}

		case WM_SETCURSOR:
		{
			if ((HWND)wParam == m_RenderHwnd)
			{
				SetCursor(NULL);
				return TRUE;
			}

			break;
		}

		case WM_SYSCHAR:
			// Disable beeping when we ALT key combo is pressed
			return 1;

		case WM_SIZE:
			OnResized(hwnd, msg, wParam, lParam);
			return 0;

		case WM_KEYDOWN:
		case WM_KEYUP:
			HandleKeyboardEvent(msg, wParam, lParam);
			return 0;

		case WM_COMMAND:
			HandleMenu(msg, wParam, lParam);
			break;

		case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO info = reinterpret_cast<LPMINMAXINFO>(lParam);
			info->ptMinTrackSize.x = 160 * 2;
			info->ptMinTrackSize.y = 144 * 2;
			break;
		}

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);

			// Fill screen with colour to avoid horrible effect
			FillRect(hdc, &ps.rcPaint, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));

			// Text to display
			if (m_ListIsEmpty)
			{
				// Create a modern font (Segoe UI, 20pt)
				HFONT hFont = CreateFontA(
					0, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
					ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
					DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
					"Segoe UI"
				);

				// Select it into the DC
				HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

				LPCSTR text = "No 'ROMS' directory found";

				// Get client rectangle of parent window
				RECT rect;
				GetClientRect(hwnd, &rect);

				// Set text format options
				DrawTextA(hdc, text, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

				// Clean up: restore old font and delete new font
				SelectObject(hdc, hOldFont);
				DeleteObject(hFont);
			}

			EndPaint(hwnd, &ps);

			return 0;
		}

		//case WM_NCHITTEST:
		//{
		//	// This allows for clicking the child render window to pass the events through to the main window
		//	if (hwnd == m_RenderHwnd)
		//	{
		//		return HTTRANSPARENT;
		//	}

		//	break;
		//}

		case WM_ENTERSIZEMOVE:
			SetTimer(m_Hwnd, m_RenderTimer, 1, NULL);
			break;

		case WM_EXITSIZEMOVE:
			KillTimer(m_Hwnd, m_RenderTimer);
			break;

		case WM_TIMER:
		{
			switch (wParam)
			{
				case m_RenderTimer:
					this->Update();
					break;
			}
		}
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void MainWindow::HandleMenu(UINT msg, WPARAM wParam, LPARAM lParam)
{
	const UINT menu_id = LOWORD(wParam);
	switch (menu_id)
	{
		// File Menu
		case m_MenuFileOpenId:
			OpenDialog();
			break;
		case m_MenuFileCloseId:
		{
			m_Application->StopEmulator();
			EnableMenuItem(m_ToolsMenuItem, m_MenuToolsCartridgeInfo, MF_DISABLED);
			this->SetStatusBarTitle("");
			this->SetStatusBarStats("");
			this->SetStatusBarState("");

			ShowWindow(m_RenderHwnd, SW_HIDE);
			ShowWindow(m_ListHwnd, SW_SHOW);

			if (m_Application->CpuRegistersWindow != nullptr)
			{
				m_Application->CpuRegistersWindow->Clear();
			}

			break;
		}
		case m_MenuFileRestartId:
			RestartEmulation();
			break;
		case m_MenuFileExitId:
			PostQuitMessage(0);
			break;

			// Emulation Menu
		case m_MenuEmulationPausePlay:
			ToggleEmulationPaused();
			break;
		case m_MenuEmulationSaveState:
			// m_Application->SaveState();
			break;
		case m_MenuEmulationLoadState:
			// m_Application->LoadState();
			break;

		case m_MenuEmulationDoubleSpeed:
			this->ToggleEmulationDoubleSpeed();
			break;

		case m_MenuEmulationScreenshot:
			this->TakeScreenshot();
			break;

			// Options Menu
		case m_MenuOptionsEnableAudio:
		{
			m_Application->SoundOutput->EnableAudio = !m_Application->SoundOutput->EnableAudio;

			if (m_Application->SoundOutput->EnableAudio)
			{
				CheckMenuItem(m_OptionsMenuItem, m_MenuOptionsEnableAudio, MF_BYCOMMAND | MF_CHECKED);
			}
			else
			{
				CheckMenuItem(m_OptionsMenuItem, m_MenuOptionsEnableAudio, MF_BYCOMMAND | MF_UNCHECKED);
			}

			break;
		}

		// Tools Menu
		case m_MenuToolsCpuRegisters:
		{
			UINT menu_state = GetMenuState(m_ToolsMenuItem, m_MenuToolsCpuRegisters, MF_BYCOMMAND);
			if (menu_state & MF_CHECKED)
			{
				CheckMenuItem(m_ToolsMenuItem, m_MenuToolsCpuRegisters, MF_BYCOMMAND | MF_UNCHECKED);

				m_Application->CpuRegistersWindow->Destroy();
				m_Application->CpuRegistersWindow.release();
			}
			else
			{
				CheckMenuItem(m_ToolsMenuItem, m_MenuToolsCpuRegisters, MF_BYCOMMAND | MF_CHECKED);

				m_Application->CpuRegistersWindow = std::make_unique<CpuRegisterWindow>();
				m_Application->CpuRegistersWindow->Create();
			}

			break;
		}
		case m_MenuToolsTilemap:
		{
			UINT menu_state = GetMenuState(m_ToolsMenuItem, m_MenuToolsTilemap, MF_BYCOMMAND);
			if (menu_state & MF_CHECKED)
			{
				CheckMenuItem(m_ToolsMenuItem, m_MenuToolsTilemap, MF_BYCOMMAND | MF_UNCHECKED);
				m_Application->CloseTileWindow();
			}
			else
			{
				CheckMenuItem(m_ToolsMenuItem, m_MenuToolsTilemap, MF_BYCOMMAND | MF_CHECKED);
				m_Application->CreateTileWindow();
			}
			break;
		}
		case m_MenuToolsTracelog:
			ToggleTracelog();
			break;
		case m_MenuToolsCartridgeInfo:
			if (m_Application->GetEmulator()->IsRunning())
			{
				UINT menu_state = GetMenuState(m_ToolsMenuItem, m_MenuToolsCartridgeInfo, MF_BYCOMMAND);
				if (!(menu_state & MF_DISABLED))
				{
					m_Application->CreateCartridgeInfoWindow();
				}
			}
			break;
	}

	// Handle save state
	if (menu_id > m_MenuSaveSlot1 && menu_id <= m_MenuSaveSlot1 + 9)
	{
		int slot = menu_id - m_MenuSaveSlot1;

		std::string filename = std::filesystem::path(m_FilePath).filename().string();

		std::filesystem::path savestate_path = "SaveStates";
		savestate_path.append(filename);

		std::filesystem::create_directories(savestate_path);
		savestate_path.append("Save-" + std::to_string(slot) + ".state");
		m_Application->SaveState(savestate_path.string());

		UpdateSaveStateDetails();
	}

	// Handle load state
	if (menu_id > m_MenuLoadSlot1 && menu_id <= m_MenuLoadSlot1 + 9)
	{
		int slot = menu_id - m_MenuLoadSlot1;

		std::string filename = std::filesystem::path(m_FilePath).filename().string();

		std::filesystem::path savestate_path = "SaveStates";
		savestate_path.append(filename);

		if (std::filesystem::exists(savestate_path))
		{
			savestate_path.append("Save-" + std::to_string(slot) + ".state");
			m_Application->LoadState(savestate_path.string());
		}
	}
}

void MainWindow::ToggleEmulationPaused()
{
	Emulator* emulator = m_Application->GetEmulator();
	if (!emulator->IsRunning())
	{
		return;
	}

	bool paused = emulator->IsPaused();
	if (paused)
	{
		CheckMenuItem(m_EmulationMenuItem, m_MenuEmulationPausePlay, MF_BYCOMMAND | MF_UNCHECKED);
		emulator->Pause(false);
		this->SetStatusBarState("Playing");
	}
	else
	{
		CheckMenuItem(m_EmulationMenuItem, m_MenuEmulationPausePlay, MF_BYCOMMAND | MF_CHECKED);
		emulator->Pause(true);
		this->SetStatusBarState("Paused");
	}
}

void MainWindow::ToggleEmulationDoubleSpeed()
{
	Emulator* emulator = m_Application->GetEmulator();
	if (!emulator->IsRunning())
	{
		return;
	}

	static bool checked = false;
	if (checked)
	{
		CheckMenuItem(m_EmulationMenuItem, m_MenuEmulationDoubleSpeed, MF_BYCOMMAND | MF_UNCHECKED);
		emulator->SetEmulationSpeedMultipler(1.0f);
		checked = false;
	}
	else
	{
		CheckMenuItem(m_EmulationMenuItem, m_MenuEmulationDoubleSpeed, MF_BYCOMMAND | MF_CHECKED);
		emulator->SetEmulationSpeedMultipler(0.5f);
		checked = true;
	}
}

void MainWindow::ToggleTileWindowMenuItem(bool checked)
{
	if (checked)
	{
		CheckMenuItem(m_ToolsMenuItem, m_MenuToolsTilemap, MF_BYCOMMAND | MF_CHECKED);
	}
	else
	{
		CheckMenuItem(m_ToolsMenuItem, m_MenuToolsTilemap, MF_BYCOMMAND | MF_UNCHECKED);
	}
}

void MainWindow::ToggleCpuRegistersWindowMenuItem(bool checked)
{
	if (checked)
	{
		CheckMenuItem(m_ToolsMenuItem, m_MenuToolsCpuRegisters, MF_BYCOMMAND | MF_CHECKED);
	}
	else
	{
		CheckMenuItem(m_ToolsMenuItem, m_MenuToolsCpuRegisters, MF_BYCOMMAND | MF_UNCHECKED);
	}
}

void MainWindow::ToggleTracelog()
{
	UINT menu_state = GetMenuState(m_ToolsMenuItem, m_MenuToolsTracelog, MF_BYCOMMAND);
	if (menu_state & MF_CHECKED)
	{
		CheckMenuItem(m_ToolsMenuItem, m_MenuToolsTracelog, MF_BYCOMMAND | MF_UNCHECKED);
		m_Application->GetEmulator()->ToggleTraceLog(false);
	}
	else
	{
		CheckMenuItem(m_ToolsMenuItem, m_MenuToolsTracelog, MF_BYCOMMAND | MF_CHECKED);
		m_Application->GetEmulator()->ToggleTraceLog(true);
	}
}

void MainWindow::OpenDialog()
{
	std::string path;
	if (OpenFileDialog(&path))
	{
		this->LoadRom(path);
	}
}

void MainWindow::LoadRom(const std::string& path)
{
	m_FilePath = path;
	m_Application->LoadRom(path);
	EnableMenuItem(m_ToolsMenuItem, m_MenuToolsCartridgeInfo, MF_ENABLED);

	std::string filename = std::filesystem::path(m_FilePath).filename().string();

	this->SetStatusBarTitle(filename);
	this->SetStatusBarState("Playing");

	// Reload cartridge info
	CartridgeInfoWindow* cartridge_info_window = m_Application->GetCartridgeInfoWindow();
	if (cartridge_info_window != nullptr)
	{
		cartridge_info_window->UpdateCartridgeInfo();
	}

	// Show savestate details
	UpdateSaveStateDetails();

	// Show render window
	ShowWindow(m_ListHwnd, SW_HIDE);
	ShowWindow(m_RenderHwnd, SW_SHOW);
}

void MainWindow::UpdateSaveStateDetails()
{
	for (int i = 1; i <= 9; ++i)
	{
		std::string filename = std::filesystem::path(m_FilePath).filename().string();

		std::filesystem::path savestate_path = "SaveStates";
		savestate_path.append(filename);
		savestate_path.append("Save-" + std::to_string(i) + ".state");

		std::ifstream file(savestate_path, std::ios::binary | std::ios::in);
		if (file.is_open())
		{
			SaveStateHeader header;
			file.read(reinterpret_cast<char*>(&header), sizeof(SaveStateHeader));

			if (header.version == 1)
			{
				char identifier[8] = { 'R', 'E', 'T', 'R', 'O', 'G', 'B', 'M' };
				if (!std::equal(std::begin(header.identifier), std::end(header.identifier), std::begin(identifier)))
				{
					continue;
				}

				// Date modified string
				char date_modified_str[11]; // Enough space for "yyyy/mm/dd\0"
				std::tm* date_modified = std::localtime(&header.date_modified);
				std::strftime(date_modified_str, sizeof(date_modified_str), "%Y/%m/%d", date_modified);

				// Time played
				std::chrono::duration<double> time_played(header.time_played);
				auto duration_in_hours = std::chrono::duration_cast<std::chrono::hours>(time_played);
				auto duration_in_minutes = std::chrono::duration_cast<std::chrono::minutes>(time_played) - duration_in_hours;

				std::wstringstream ss;
				ss << L"Slot " << std::to_wstring(i) << L" - ";

				// Date modified
				ss << date_modified_str << " - ";

				// Hours
				if (duration_in_hours.count() != 0)
				{
					ss << duration_in_hours.count() << " hours";
					if (duration_in_minutes.count() != 0)
					{
						ss << " " << duration_in_minutes.count() << " minutes";
					}
				}
				else
				{
					ss << duration_in_minutes.count() << " minutes";
				}

				ModifyMenu(m_SaveSlotMenuItem, m_MenuSaveSlot1 + i, MF_STRING, m_MenuSaveSlot1 + i, ss.str().c_str());
				ModifyMenu(m_LoadSlotMenuItem, m_MenuLoadSlot1 + i, MF_STRING, m_MenuLoadSlot1 + i, ss.str().c_str());
			}
		}
	}
}

bool MainWindow::OpenFileDialog(std::string* filepath)
{
	HRESULT hr = S_OK;

	// Initialize the COM library
	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(hr))
	{
		throw std::exception("CoInitializeEx failed");
	}

	// Create the FileOpenDialog object
	IFileOpenDialog* file_open = NULL;
	hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&file_open));
	if (FAILED(hr))
	{
		CoUninitialize();
		throw std::exception("CoCreateInstance failed");
	}

	// Show the Open dialog box
	const COMDLG_FILTERSPEC filters[] =
	{
		{ L"Gameboy ROM", L"*.gb;*.gbc" },
		{ L"All Files",L"*.*" }
	};

	file_open->SetFileTypes(2, filters);
	file_open->SetTitle(L"Open ROM");

	hr = file_open->Show(m_Hwnd);
	if (FAILED(hr))
	{
		CoUninitialize();
		return false;
	}

	// Get the file name from the dialog box
	IShellItem* item = NULL;
	hr = file_open->GetResult(&item);
	if (SUCCEEDED(hr))
	{
		PWSTR path;
		hr = item->GetDisplayName(SIGDN_FILESYSPATH, &path);

		// Display the file name to the user
		if (SUCCEEDED(hr))
		{
			*filepath = Utilities::ConvertToString(path);
			CoTaskMemFree(path);
		}

		item->Release();
	}

	// Cleanup
	file_open->Release();
	CoUninitialize();

	return true;
}

void MainWindow::OnClose()
{
	m_Application->StopEmulator();
	PostQuitMessage(0);
}

void MainWindow::OnKeyPressed(UINT virtual_key_code)
{
	const WORD RKey = 0x52;
	const WORD PKey = 0x50;
	const WORD CKey = 0x43;

	// Restart
	if (virtual_key_code == RKey)
	{
		RestartEmulation();
	}

	// Pause
	if (virtual_key_code == PKey || virtual_key_code == CKey)
	{
		ToggleEmulationPaused();
	}

	// Save state
	if (virtual_key_code == VK_F5)
	{
		// m_Application->SaveState();
	}
	else if (virtual_key_code == VK_F8)
	{
		// m_Application->LoadState();
	}
}

void MainWindow::RestartEmulation()
{
	if (m_Application->GetEmulator()->IsRunning())
	{
		m_Application->LoadRom(m_FilePath);
	}
}

void MainWindow::HandleKeyboardEvent(UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Decode win32 message
	WORD key_flags = HIWORD(lParam);
	bool repeat = (key_flags & KF_REPEAT) == KF_REPEAT;
	bool alt_down = (key_flags & KF_ALTDOWN) == KF_ALTDOWN;
	WORD scan_code = LOBYTE(key_flags);
	BOOL extended_key = (key_flags & KF_EXTENDED) == KF_EXTENDED;

	if (extended_key)
	{
		scan_code = MAKEWORD(scan_code, 0xE0);
	}

	if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN)
	{
		if (!repeat)
		{
			HandleKey(true, scan_code);
		}
	}
	else if (msg == WM_KEYUP || msg == WM_SYSKEYUP)
	{
		HandleKey(false, scan_code);
	}
}

void MainWindow::HandleKey(bool state, WORD scancode)
{
	if (m_Application == nullptr)
	{
		return;
	}

	const WORD ZKey = 0x5A;
	const WORD XKey = 0x58;

	UINT key = MapVirtualKeyW(scancode, MAPVK_VSC_TO_VK_EX);
	if (state)
	{
		this->OnKeyPressed(key);
	}

	switch (key)
	{
		case ZKey:
			m_Application->GetEmulator()->GetJoypad()->SetJoypad(JoypadButton::B, state);
			break;
		case XKey:
			m_Application->GetEmulator()->GetJoypad()->SetJoypad(JoypadButton::A, state);
			break;
		case VK_RETURN:
			m_Application->GetEmulator()->GetJoypad()->SetJoypad(JoypadButton::Start, state);
			break;
		case VK_TAB:
			m_Application->GetEmulator()->GetJoypad()->SetJoypad(JoypadButton::Select, state);
			break;
		case VK_UP:
			m_Application->GetEmulator()->GetJoypad()->SetJoypad(JoypadButton::Up, state);
			break;
		case VK_DOWN:
			m_Application->GetEmulator()->GetJoypad()->SetJoypad(JoypadButton::Down, state);
			break;
		case VK_LEFT:
			m_Application->GetEmulator()->GetJoypad()->SetJoypad(JoypadButton::Left, state);
			break;
		case VK_RIGHT:
			m_Application->GetEmulator()->GetJoypad()->SetJoypad(JoypadButton::Right, state);
			break;
	}
}

void MainWindow::OnResized(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	UINT width = LOWORD(lParam);
	UINT height = HIWORD(lParam);

	// Resize statusbar
	SendMessage(m_HwndStatusbar, WM_SIZE, wParam, lParam);
	ComputeStatusBarSections();

	// Resize title window
	if (m_ListHwnd != NULL)
	{
		MoveWindow(m_ListHwnd, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
	}

	// Resize render window
	int render_width, render_height;
	ComputeRenderWindowSize(&render_width, &render_height);
	SetWindowPos(m_RenderHwnd, NULL, 0, 0, render_width, render_height, SWP_FRAMECHANGED | SWP_NOMOVE);

	// Don't resize on minimized
	if (wParam == SIZE_MINIMIZED)
		return;

	// Resize target
	if (m_MainRenderTarget != nullptr)
		m_MainRenderTarget->Resize(render_width, render_height);
}

void MainWindow::CreateMainWindow(const std::string& title, int width, int height)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);
	const std::wstring window_title = Utilities::ConvertToWString(title);
	m_RegisterClassName = window_title;

	// Setup window class
	WNDCLASS wc = {};
	wc.style = CS_VREDRAW | CS_HREDRAW;
	wc.lpfnWndProc = MainWndProc;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.lpszClassName = m_RegisterClassName.c_str();
	wc.lpszMenuName = NULL;

	if (!RegisterClass(&wc))
	{
		throw std::exception("RegisterClass Failed");
	}

	// Compute window rectangle dimensions based on requested client area dimensions.
	RECT window_rect = { 0, 0, width, height };
	AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW, false);
	int window_width = window_rect.right - window_rect.left;
	int window_height = window_rect.bottom - window_rect.top;

	// Create window
	m_Hwnd = CreateWindow(wc.lpszClassName, window_title.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, window_width, window_height, NULL, NULL, hInstance, this);
	if (m_Hwnd == NULL)
	{
		throw std::exception("CreateWindow Failed");
	}

	// Show window
	ShowWindow(m_Hwnd, SW_SHOWNORMAL);
}

void MainWindow::CreateMenuBar()
{
	m_MenuBar = CreateMenu();

	// File menu
	m_FileMenuItem = CreateMenu();
	AppendMenuW(m_FileMenuItem, MF_STRING, m_MenuFileOpenId, L"Open");
	AppendMenuW(m_FileMenuItem, MF_STRING, m_MenuFileCloseId, L"Close");
	AppendMenuW(m_FileMenuItem, MF_STRING, m_MenuFileRestartId, L"Restart");
	AppendMenuW(m_FileMenuItem, MF_SEPARATOR, NULL, NULL);
	AppendMenuW(m_FileMenuItem, MF_STRING, m_MenuFileExitId, L"Exit");
	AppendMenuW(m_MenuBar, MF_POPUP, reinterpret_cast<UINT_PTR>(m_FileMenuItem), L"&File");

	// Saveslot menu
	m_SaveSlotMenuItem = CreateMenu();
	for (int i = 1; i <= 9; ++i)
	{
		std::wstring title = L"Slot " + std::to_wstring(i) + L" - Empty";
		AppendMenuW(m_SaveSlotMenuItem, MF_STRING, m_MenuSaveSlot1 + i, title.c_str());
	}

	// m_LoadSlotMenuItem menu
	m_LoadSlotMenuItem = CreateMenu();
	for (int i = 1; i <= 9; ++i)
	{
		std::wstring title = L"Slot " + std::to_wstring(i) + L" - Empty";
		AppendMenuW(m_LoadSlotMenuItem, MF_STRING, m_MenuLoadSlot1 + i, title.c_str());
	}

	// Emulation menu
	m_EmulationMenuItem = CreateMenu();
	AppendMenuW(m_EmulationMenuItem, MF_UNCHECKED, m_MenuEmulationPausePlay, L"Pause");
	AppendMenuW(m_EmulationMenuItem, MF_SEPARATOR, NULL, NULL);
	AppendMenuW(m_EmulationMenuItem, MF_POPUP, reinterpret_cast<UINT_PTR>(m_SaveSlotMenuItem), L"Save State");
	AppendMenuW(m_EmulationMenuItem, MF_POPUP, reinterpret_cast<UINT_PTR>(m_LoadSlotMenuItem), L"Load State");
	AppendMenuW(m_EmulationMenuItem, MF_SEPARATOR, NULL, NULL);
	AppendMenuW(m_EmulationMenuItem, MF_UNCHECKED, m_MenuEmulationDoubleSpeed, L"Double Speed");
	AppendMenuW(m_EmulationMenuItem, MF_SEPARATOR, NULL, NULL);
	AppendMenuW(m_EmulationMenuItem, MF_UNCHECKED, m_MenuEmulationScreenshot, L"Screenshot");
	AppendMenuW(m_MenuBar, MF_POPUP, reinterpret_cast<UINT_PTR>(m_EmulationMenuItem), L"Emulation");

	// Options menu
	m_OptionsMenuItem = CreateMenu();
	AppendMenuW(m_OptionsMenuItem, MF_CHECKED, m_MenuOptionsEnableAudio, L"Enable Audio");
	AppendMenuW(m_MenuBar, MF_POPUP, reinterpret_cast<UINT_PTR>(m_OptionsMenuItem), L"Options");

	// Tools menu
	m_ToolsMenuItem = CreateMenu();
	AppendMenuW(m_ToolsMenuItem, MF_UNCHECKED, m_MenuToolsCpuRegisters, L"CPU Registers");
	AppendMenuW(m_ToolsMenuItem, MF_SEPARATOR, NULL, NULL);
	AppendMenuW(m_ToolsMenuItem, MF_UNCHECKED, m_MenuToolsTilemap, L"Tiledata");
	AppendMenuW(m_ToolsMenuItem, MF_UNCHECKED, m_MenuToolsTracelog, L"Tracelog");
	AppendMenuW(m_ToolsMenuItem, MF_STRING | MF_DISABLED, m_MenuToolsCartridgeInfo, L"Cartridge Info");
	AppendMenuW(m_MenuBar, MF_POPUP, reinterpret_cast<UINT_PTR>(m_ToolsMenuItem), L"Tools");

	// Assign menubar to window
	SetMenu(m_Hwnd, m_MenuBar);
}

void MainWindow::CreateStatusBar()
{
	// Ensure that the common control DLL is loaded
	InitCommonControls();

	// Create the status bar
	HINSTANCE hInstance = GetModuleHandle(NULL);
	m_HwndStatusbar = CreateWindow(STATUSCLASSNAME, NULL, SBARS_SIZEGRIP | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, m_Hwnd, m_StatusBar, hInstance, NULL);
	ComputeStatusBarSections();
}

void MainWindow::ComputeStatusBarSections()
{
	// Calculate status bar
	RECT status_rect;
	GetClientRect(m_HwndStatusbar, &status_rect);

	int section_width = status_rect.right / 3;

	// Create sections
	std::vector<int> edges = { section_width * 1, section_width * 2, section_width * 3 };
	SendMessage(m_HwndStatusbar, SB_SETPARTS, (WPARAM)edges.size(), (LPARAM)edges.data());
}

void MainWindow::CreateRomListWindow()
{
	HINSTANCE hInstance = GetModuleHandle(NULL);

	// Init common controls
	INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_LISTVIEW_CLASSES };
	InitCommonControlsEx(&icex);

	// Populate ROM paths
	if (std::filesystem::exists("ROMS"))
	{

		int width, height;
		ComputeRenderWindowSize(&width, &height);

		// Create ListView
		m_ListHwnd = CreateWindow(WC_LISTVIEW, L"",
			WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_NOSORTHEADER,
			0, 0, width, height, m_Hwnd, reinterpret_cast<HMENU>((UINT_PTR)m_ListMenuId), hInstance, this);

		// Add columns
		LVCOLUMN lvCol = { 0 };
		lvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

		const wchar_t* title = L"Title";
		lvCol.pszText = const_cast<wchar_t*>(title);
		lvCol.cx = 400;
		ListView_InsertColumn(m_ListHwnd, 0, &lvCol);

		const wchar_t* time_played = L"Time Played";
		lvCol.pszText = const_cast<wchar_t*>(time_played);
		lvCol.cx = 200;
		ListView_InsertColumn(m_ListHwnd, 1, &lvCol);

		const wchar_t* last_played = L"Last Played";
		lvCol.pszText = const_cast<wchar_t*>(last_played);
		lvCol.cx = 200;
		ListView_InsertColumn(m_ListHwnd, 2, &lvCol);

		// Add sample items
		std::vector<std::wstring> titles;
		std::vector<std::wstring> times;
		std::vector<std::wstring> history;
		std::filesystem::path rom_path("ROMS");

		for (const auto& entry : std::filesystem::directory_iterator(rom_path))
		{
			std::filesystem::path extensions = entry.path().extension();
			if (extensions == ".gbc" || extensions == ".gb")
			{
				titles.push_back(entry.path().filename().wstring());

				std::wstring total_time_played = L"No time played";
				std::wstring last_played = L"Never played";

				for (auto& gameData : m_Application->ProfileDataList)
				{
					if (entry.path().filename() == gameData.filename)
					{
						// Total Play Time
						int time_played = gameData.totalPlayTimeMinutes;
						if (time_played >= 120)
						{
							uint64_t minutes = time_played % 60;
							uint64_t hours = (time_played - minutes) / 60;

							total_time_played = std::to_wstring(hours) + L" hours " + std::to_wstring(minutes) + L" minutes";
						}
						else if (time_played > 0 && time_played < 120)
						{
							total_time_played = std::to_wstring(time_played) + L" minutes";
						}
						else
						{
							total_time_played = L"Less than a minute";
						}

						// Last Played
						std::string last_played_json = gameData.lastPlayed;

						// Remove extra info to match format
						std::string trimmed = last_played_json.substr(0, 19);

						// Declare a sys_time object for parsing the datetime
						std::chrono::sys_time<std::chrono::seconds> tp;

						// Parse the datetime without milliseconds and timezone
						std::stringstream ss(trimmed);
						ss >> std::chrono::parse("%FT%T", tp);

						if (!ss.fail())
						{
							// Convert sys_time to year_month_day for formatting
							auto ymd = std::chrono::year_month_day{ std::chrono::floor<std::chrono::days>(tp) };
							last_played = Utilities::ConvertToWString(std::format("{:%d/%m/%Y}", ymd));
						}
					}
				}

				times.push_back(total_time_played);
				history.push_back(last_played);
			}
		}

		for (int i = 0; i < titles.size(); ++i)
		{
			LVITEM lvItem = { 0 };
			lvItem.mask = LVIF_TEXT;
			lvItem.iItem = i;
			lvItem.pszText = (LPWSTR)titles[i].c_str();
			ListView_InsertItem(m_ListHwnd, &lvItem);

			ListView_SetItemText(m_ListHwnd, i, 1, (LPWSTR)times[i].c_str());
			ListView_SetItemText(m_ListHwnd, i, 2, (LPWSTR)history[i].c_str());
		}

		ListView_SetExtendedListViewStyle(m_ListHwnd, LVS_EX_FULLROWSELECT);

		ShowWindow(m_ListHwnd, SW_SHOW);
		UpdateWindow(m_ListHwnd);
	}
	else
	{
		m_ListIsEmpty = true;
	}
}

void MainWindow::CreateRenderWindow()
{
	HINSTANCE hInstance = GetModuleHandle(NULL);

	int width, height;
	ComputeRenderWindowSize(&width, &height);

	// Create render window as a child of the main window
	m_RenderHwnd = CreateWindow(m_RegisterClassName.c_str(), L"EmulatorWindow", WS_POPUP | WS_VISIBLE | WS_SYSMENU | WS_CHILD, 0, 0, width, height, NULL, NULL, hInstance, this);
	if (m_RenderHwnd == NULL)
	{
		throw std::exception("CreateWindow Failed");
	}

	SetParent(m_RenderHwnd, m_Hwnd);
	ShowWindow(m_RenderHwnd, SW_HIDE);

	// Target
	m_MainRenderTarget = m_Application->GetRenderDevice()->CreateRenderTarget();
	m_MainRenderTarget->Create(m_RenderHwnd);
	m_MainRenderTarget->DisableFullscreenAltEnter();

	// Texture
	m_MainRenderTexture = m_Application->GetRenderDevice()->CreateTexture();
	m_MainRenderTexture->Create(160, 144);

	m_Application->m_DisplayOutput = std::make_unique<DisplayOutput>(m_MainRenderTexture.get());
}

void MainWindow::ComputeRenderWindowSize(int* width, int* height)
{
	// Calculate main window rectangle
	RECT window_rect;
	GetClientRect(m_Hwnd, &window_rect);

	// Calculate status bar
	RECT status_rect;
	GetClientRect(m_HwndStatusbar, &status_rect);

	// Compute size
	*width = (window_rect.right);
	*height = (window_rect.bottom - status_rect.bottom);
}

void MainWindow::SetStatusBarTitle(const std::string& text)
{
	int section = 0;
	std::wstring str = Utilities::ConvertToWString(text);
	SendMessage(m_HwndStatusbar, SB_SETTEXT, section | SBT_POPOUT, reinterpret_cast<LPARAM>(str.data()));
}

void MainWindow::SetStatusBarStats(const std::string& text)
{
	int section = 1;
	std::wstring str = Utilities::ConvertToWString(text);
	SendMessage(m_HwndStatusbar, SB_SETTEXT, section | SBT_POPOUT, reinterpret_cast<LPARAM>(str.data()));
}

void MainWindow::SetStatusBarState(const std::string& text)
{
	int section = 2;
	std::wstring str = Utilities::ConvertToWString(text);
	SendMessage(m_HwndStatusbar, SB_SETTEXT, section | SBT_POPOUT, reinterpret_cast<LPARAM>(str.data()));
}

void MainWindow::TakeScreenshot()
{
	m_Application->GetEmulator()->Pause(true);

	// Generate a GUID for name
	GUID gidReference;
	HRESULT hCreateGuid = CoCreateGuid(&gidReference);

	WCHAR str[256];
	int len = StringFromGUID2(gidReference, str, 256);

	std::string guid(Utilities::ConvertToString(str));

	// Create a buffer for the image data in RGBA format
	int width = 160;
	int height = 144;
	std::string filename = guid + ".png";

	// Save screenshots in /Screenshots/{filename} folder
	std::filesystem::path savepath("Screenshots");
	savepath.append(std::filesystem::path(m_FilePath).filename().string());

	if (!std::filesystem::exists(savepath))
	{
		std::filesystem::create_directories(savepath);
	}

	savepath.append(filename);

	const uint8_t* videoBuffer = (const uint8_t*)m_Application->GetEmulator()->GetVideoBuffer();
	int videoPitch = m_Application->GetEmulator()->GetVideoPitch();

	std::vector<uint8_t> imageData(width * height * 4);

	// Copy the video buffer into the imageData buffer, row by row
	for (int row = 0; row < height; ++row)
	{
		std::memcpy(imageData.data() + row * width * 4, videoBuffer + row * videoPitch, width * 4);
	}

	// Save the image as PNG
	stbi_write_png(savepath.string().c_str(), width, height, 4, imageData.data(), width * 4);

	m_Application->GetEmulator()->Pause(false);
}
