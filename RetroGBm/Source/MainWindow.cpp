#include "MainWindow.h"
#include <shobjidl.h>
#include <string>
#include <vector>
#include "Application.h"
#include <Emulator.h>

namespace
{
	static std::string ConvertToString(const std::wstring& str)
	{
		size_t size = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);

		std::vector<char> buffer(size);
		int chars_converted = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), buffer.data(), static_cast<int>(buffer.size()), NULL, NULL);

		return std::string(buffer.data(), chars_converted);
	}
}

MainWindow::MainWindow(Application* application) : Window(application)
{
}

void MainWindow::Create(const std::string& title, int width, int height)
{
	Window::Create(title, width, height);

	// Menu
	HMENU menubar = CreateMenu();

	HMENU filemenu = CreateMenu();
	AppendMenuW(filemenu, MF_STRING, m_MenuFileOpenId, L"Open");
	AppendMenuW(filemenu, MF_STRING, m_MenuFileCloseId, L"Close");
	AppendMenuW(filemenu, MF_STRING, m_MenuFileRestartId, L"Restart");
	AppendMenuW(filemenu, MF_SEPARATOR, NULL, NULL);
	AppendMenuW(filemenu, MF_STRING, m_MenuFileExitId, L"Exit");
	AppendMenuW(menubar, MF_POPUP, reinterpret_cast<UINT_PTR>(filemenu), L"&File");

	m_EmulationMenuItem = CreateMenu();
	AppendMenuW(m_EmulationMenuItem, MF_UNCHECKED, m_MenuEmulationPausePlay, L"Pause");
	AppendMenuW(m_EmulationMenuItem, MF_SEPARATOR, NULL, NULL);
	AppendMenuW(m_EmulationMenuItem, MF_STRING, m_MenuEmulationSaveState, L"Save State");
	AppendMenuW(m_EmulationMenuItem, MF_STRING, m_MenuEmulationLoadState, L"Load State");
	AppendMenuW(menubar, MF_POPUP, reinterpret_cast<UINT_PTR>(m_EmulationMenuItem), L"Emulation");

	m_DebugMenuItem = CreateMenu();
	AppendMenuW(m_DebugMenuItem, MF_UNCHECKED, m_MenuDebugTilemap, L"Tilemap");
	AppendMenuW(m_DebugMenuItem, MF_UNCHECKED, m_MenuDebugTracelog, L"Tracelog");
	AppendMenuW(m_DebugMenuItem, MF_STRING | MF_DISABLED, m_MenuDebugCartridgeInfo, L"Cartridge Info");
	AppendMenuW(menubar, MF_POPUP, reinterpret_cast<UINT_PTR>(m_DebugMenuItem), L"Debug");

	SetMenu(this->GetHwnd(), menubar);
}

LRESULT MainWindow::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO info = reinterpret_cast<LPMINMAXINFO>(lParam);
			info->ptMinTrackSize.x = 160 * 2;
			info->ptMinTrackSize.y = 144 * 2;
			break;
		}
	}

	return Window::HandleMessage(hwnd, msg, wParam, lParam);
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
	const COMDLG_FILTERSPEC filters[3] =
	{
		{ L"Gameboy ROM", L"*.gb;*.gbc" },
		{ L"All Files",L"*.*" }
	};

	file_open->SetFileTypes(2, filters);
	file_open->SetTitle(L"Open ROM");

	hr = file_open->Show(this->GetHwnd());
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
			*filepath = ConvertToString(path);
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
	Window::OnClose();
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