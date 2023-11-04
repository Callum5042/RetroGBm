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
	AppendMenuW(filemenu, MF_SEPARATOR, NULL, NULL);
	AppendMenuW(filemenu, MF_STRING, m_MenuFileExitId, L"Exit");
	AppendMenuW(menubar, MF_POPUP, (UINT_PTR)filemenu, L"&File");

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
			MessageBox(NULL, L"Close", L"Menu", MB_OK);
			break;
		case m_MenuFileExitId:
			PostQuitMessage(0);
			break;
	}
}

void MainWindow::OpenDialog()
{
	std::string path;
	if (OpenFileDialog(&path))
	{
		// MessageBoxA(NULL, path.c_str(), "File Path", MB_OK);


		m_Application->GetEmulator()->LoadRom(path);
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
	const COMDLG_FILTERSPEC filters[2] =
	{
		{ L"GameBoy ROM", L"*.gb" },
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