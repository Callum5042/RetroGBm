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
	bool result = false;

	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr))
	{
		IFileOpenDialog* pFileOpen = NULL;

		// Create the FileOpenDialog object
		hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

		if (SUCCEEDED(hr))
		{
			// Show the Open dialog box
			hr = pFileOpen->Show(NULL);

			// Get the file name from the dialog box
			if (SUCCEEDED(hr))
			{
				IShellItem* pItem;
				hr = pFileOpen->GetResult(&pItem);
				if (SUCCEEDED(hr))
				{
					PWSTR path;
					hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &path);
					if (SUCCEEDED(hr))
					{
						*filepath = ConvertToString(path);
						CoTaskMemFree(path);
						result = true;
					}

					pItem->Release();
				}
			}

			pFileOpen->Release();
		}

		CoUninitialize();
	}

	return result;
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
		Emulator* emulator = m_Application->GetEmulator();
		if (emulator->IsRunning())
		{
			emulator->SaveState();
		}
	}
	else if (virtual_key_code == VK_F8)
	{
		Emulator* emulator = m_Application->GetEmulator();
		if (emulator->IsRunning())
		{
			emulator->LoadState();
		}
	}
}

void MainWindow::RestartEmulation()
{
	if (m_Application->GetEmulator()->IsRunning())
	{
		m_Application->LoadRom(m_FilePath);
	}
}