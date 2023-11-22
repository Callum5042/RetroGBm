#include "MainWindow.h"
#include "Application.h"
#include "Utilities/Utilities.h"
#include "../resource.h"

#include <Emulator.h>
#include <Joypad.h>

#include <string>
#include <vector>
#include <shobjidl.h>

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

	// Statusbar on draw on top of render
	SetWindowPos(m_HwndStatusbar, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

void MainWindow::Update()
{
	m_MainRenderTexture->Update(m_Application->GetEmulator()->GetVideoBuffer(), m_Application->GetEmulator()->GetVideoPitch());

	m_MainRenderTarget->Clear();
	m_MainRenderTexture->Render();
	m_MainRenderTarget->Present();
}

LRESULT MainWindow::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_CLOSE:
			OnClose();
			return 0;

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
			EndPaint(hwnd, &ps);

			return 0;
		}
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void MainWindow::HandleMenu(UINT msg, WPARAM wParam, LPARAM lParam)
{
	const UINT menu_id = LOWORD(wParam);
	switch (menu_id)
	{
		case m_MenuFileOpenId:
			OpenDialog();
			break;
		case m_MenuFileCloseId:
			m_Application->StopEmulator();
			EnableMenuItem(m_DebugMenuItem, m_MenuDebugCartridgeInfo, MF_DISABLED);
			break;
		case m_MenuFileRestartId:
			RestartEmulation();
			break;
		case m_MenuFileExitId:
			PostQuitMessage(0);
			break;
		case m_MenuDebugTilemap:
		{
			UINT menu_state = GetMenuState(m_DebugMenuItem, m_MenuDebugTilemap, MF_BYCOMMAND);
			if (menu_state & MF_CHECKED)
			{
				CheckMenuItem(m_DebugMenuItem, m_MenuDebugTilemap, MF_BYCOMMAND | MF_UNCHECKED);
				m_Application->CloseTileWindow();
			}
			else
			{
				CheckMenuItem(m_DebugMenuItem, m_MenuDebugTilemap, MF_BYCOMMAND | MF_CHECKED);
				m_Application->CreateTileWindow();
			}

			// CheckMenuItem(m_DebugMenuItem, m_MenuDebugTilemap, MF_BYPOSITION | MF_CHECKED);
			break;
		}
		case m_MenuDebugTracelog:
			ToggleTracelog();
			break;
		case m_MenuDebugCartridgeInfo:
			if (m_Application->GetEmulator()->IsRunning())
			{
				UINT menu_state = GetMenuState(m_DebugMenuItem, m_MenuDebugCartridgeInfo, MF_BYCOMMAND);
				if (!(menu_state & MF_DISABLED))
				{
					m_Application->CreateCartridgeInfoWindow();
				}
			}
			break;
		case m_MenuEmulationPausePlay:
			ToggleEmulationPaused();
			break;
		case m_MenuEmulationSaveState:
			m_Application->SaveState();
			break;
		case m_MenuEmulationLoadState:
			m_Application->LoadState();
			break;
	}
}

void MainWindow::ToggleEmulationPaused()
{
	Emulator* emulator = m_Application->GetEmulator();
	if (!emulator->IsRunning())
	{
		return;
	}

	UINT menu_state = GetMenuState(m_EmulationMenuItem, m_MenuEmulationPausePlay, MF_BYCOMMAND);
	if (menu_state & MF_CHECKED)
	{
		CheckMenuItem(m_EmulationMenuItem, m_MenuEmulationPausePlay, MF_BYCOMMAND | MF_UNCHECKED);
		emulator->Pause(false);
	}
	else
	{
		CheckMenuItem(m_EmulationMenuItem, m_MenuEmulationPausePlay, MF_BYCOMMAND | MF_CHECKED);
		emulator->Pause(true);
	}
}

void MainWindow::ToggleTileWindowMenuItem(bool checked)
{
	if (checked)
	{
		CheckMenuItem(m_DebugMenuItem, m_MenuDebugTilemap, MF_BYCOMMAND | MF_CHECKED);
	}
	else
	{
		CheckMenuItem(m_DebugMenuItem, m_MenuDebugTilemap, MF_BYCOMMAND | MF_UNCHECKED);
	}
}

void MainWindow::ToggleTracelog()
{
	UINT menu_state = GetMenuState(m_DebugMenuItem, m_MenuDebugTracelog, MF_BYCOMMAND);
	if (menu_state & MF_CHECKED)
	{
		CheckMenuItem(m_DebugMenuItem, m_MenuDebugTracelog, MF_BYCOMMAND | MF_UNCHECKED);
		m_Application->GetEmulator()->ToggleTraceLog(false);
	}
	else
	{
		CheckMenuItem(m_DebugMenuItem, m_MenuDebugTracelog, MF_BYCOMMAND | MF_CHECKED);
		m_Application->GetEmulator()->ToggleTraceLog(true);
	}
}

void MainWindow::OpenDialog()
{
	std::string path;
	if (OpenFileDialog(&path))
	{
		m_FilePath = path;
		m_Application->LoadRom(path);
		EnableMenuItem(m_DebugMenuItem, m_MenuDebugCartridgeInfo, MF_ENABLED);
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
		Emulator* emulator = m_Application->GetEmulator();
		if (emulator->IsRunning())
		{
			bool paused = emulator->IsPaused();

			if (paused)
			{
				emulator->Pause(false);
				CheckMenuItem(m_EmulationMenuItem, m_MenuEmulationPausePlay, MF_BYCOMMAND | MF_UNCHECKED);
			}
			else
			{
				emulator->Pause(true);
				CheckMenuItem(m_EmulationMenuItem, m_MenuEmulationPausePlay, MF_BYCOMMAND | MF_CHECKED);
			}
		}
	}

	// Save state
	if (virtual_key_code == VK_F5)
	{
		m_Application->SaveState();
	}
	else if (virtual_key_code == VK_F8)
	{
		m_Application->LoadState();
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

	// Resize render window
	int render_width, render_height;
	ComputeRenderWindowSize(&render_width, &render_height);
	SetWindowPos(m_RenderHwnd, NULL, 0, 0, render_width, render_height, SWP_FRAMECHANGED | SWP_NOMOVE);

	// Don't resize on minimized
	//if (wParam == SIZE_MINIMIZED)
	//	return;

	//// Get window size
	//int width, height;
	//this->GetSize(&width, &height);

	//// Resize target
	//if (m_RenderTarget != nullptr)
	//{
	//	m_RenderTarget->Resize(width, height);
	//}
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

	// Emulation menu
	m_EmulationMenuItem = CreateMenu();
	AppendMenuW(m_EmulationMenuItem, MF_UNCHECKED, m_MenuEmulationPausePlay, L"Pause");
	AppendMenuW(m_EmulationMenuItem, MF_SEPARATOR, NULL, NULL);
	AppendMenuW(m_EmulationMenuItem, MF_STRING, m_MenuEmulationSaveState, L"Save State");
	AppendMenuW(m_EmulationMenuItem, MF_STRING, m_MenuEmulationLoadState, L"Load State");
	AppendMenuW(m_MenuBar, MF_POPUP, reinterpret_cast<UINT_PTR>(m_EmulationMenuItem), L"Emulation");

	// Debug menu
	m_DebugMenuItem = CreateMenu();
	AppendMenuW(m_DebugMenuItem, MF_UNCHECKED, m_MenuDebugTilemap, L"Tilemap");
	AppendMenuW(m_DebugMenuItem, MF_UNCHECKED, m_MenuDebugTracelog, L"Tracelog");
	AppendMenuW(m_DebugMenuItem, MF_STRING | MF_DISABLED, m_MenuDebugCartridgeInfo, L"Cartridge Info");
	AppendMenuW(m_MenuBar, MF_POPUP, reinterpret_cast<UINT_PTR>(m_DebugMenuItem), L"Debug");

	// Assign menubar to window
	SetMenu(m_Hwnd, m_MenuBar);
}

void MainWindow::CreateStatusBar()
{
	RECT rcClient;
	HLOCAL hloc;
	PINT paParts;
	int i, nWidth;

	int cParts = 4;

	// Ensure that the common control DLL is loaded.
	InitCommonControls();

	HINSTANCE hInstance = GetModuleHandle(NULL);

	// Create the status bar
	m_HwndStatusbar = CreateWindow(STATUSCLASSNAME, NULL, SBARS_SIZEGRIP | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, m_Hwnd, m_StatusBar, hInstance, NULL);

	// Get the coordinates of the parent window's client area
	GetClientRect(m_Hwnd, &rcClient);

	// Allocate an array for holding the right edge coordinates
	hloc = LocalAlloc(LHND, sizeof(int) * cParts);
	paParts = (PINT)LocalLock(hloc);

	// Calculate the right edge coordinate for each part, and copy the coordinates to the array
	nWidth = rcClient.right / cParts;
	int rightEdge = nWidth;
	for (i = 0; i < cParts; i++)
	{
		paParts[i] = rightEdge;
		rightEdge += nWidth;
	}

	// Tell the status bar to create the window parts.
	SendMessage(m_HwndStatusbar, SB_SETPARTS, (WPARAM)cParts, (LPARAM)paParts);

	// Free the array, and return.
	LocalUnlock(hloc);
	LocalFree(hloc);
}

void MainWindow::CreateRenderWindow()
{
	HINSTANCE hInstance = GetModuleHandle(NULL);

	int width, height;
	ComputeRenderWindowSize(&width, &height);

	// Create render window as a child of the main window
	m_RenderHwnd = CreateWindow(m_RegisterClassName.c_str(), L"EmulatorWindow", WS_POPUP | WS_VISIBLE | WS_SYSMENU, 0, 0, width, height, NULL, NULL, hInstance, this);
	if (m_RenderHwnd == NULL)
	{
		throw std::exception("CreateWindow Failed");
	}

	SetParent(m_RenderHwnd, m_Hwnd);
	ShowWindow(m_RenderHwnd, SW_SHOW);

	// Target
	m_MainRenderTarget = m_Application->GetRenderDevice()->CreateRenderTarget();
	m_MainRenderTarget->Create(m_RenderHwnd);
	m_MainRenderTarget->DisableFullscreenAltEnter();

	// Texture
	m_MainRenderTexture = m_Application->GetRenderDevice()->CreateTexture();
	m_MainRenderTexture->Create(160, 144);
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