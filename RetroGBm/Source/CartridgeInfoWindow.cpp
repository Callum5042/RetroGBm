#include "CartridgeInfoWindow.h"
#include "Application.h"
#include <Emulator.h>
#include <Cartridge.h>

// Set theme
// https://docs.microsoft.com/en-gb/windows/win32/controls/cookbook-overview?redirectedfrom=MSDN
#pragma comment(linker,"\"/manifestdependency:type='win32' name = 'Microsoft.Windows.Common-Controls' version = '6.0.0.0' processorArchitecture = '*' publicKeyToken = '6595b64144ccf1df' language = '*'\"")

namespace
{
	static CartridgeInfoWindow* GetWindow(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		CartridgeInfoWindow* window = nullptr;
		if (uMsg == WM_NCCREATE)
		{
			CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
			window = reinterpret_cast<CartridgeInfoWindow*>(pCreate->lpCreateParams);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
		}
		else
		{
			window = reinterpret_cast<CartridgeInfoWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		}

		return window;
	}

	static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		CartridgeInfoWindow* window = GetWindow(hwnd, msg, wParam, lParam);
		if (window == nullptr)
		{
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}

		return window->HandleMessage(hwnd, msg, wParam, lParam);
	}

	static std::wstring ConvertToWString(const std::string& str)
	{
		size_t size = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0);

		std::vector<wchar_t> buffer(size);
		int chars_converted = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), buffer.data(), static_cast<int>(buffer.size()));

		return std::wstring(buffer.data(), chars_converted);
	}
}

CartridgeInfoWindow::CartridgeInfoWindow(Application* application) : m_Application(application)
{
}

CartridgeInfoWindow::~CartridgeInfoWindow()
{
	Destroy();
}

void CartridgeInfoWindow::Create(const std::string& title, int width, int height)
{
	WindowCreate(title, width, height);
	FontCreate();
	CreateGroupBox(width, height);

	Cartridge* cartridge = m_Application->GetEmulator()->GetCartridge();
	const CartridgeInfo* info = cartridge->GetCartridgeInfo();

	std::vector<InfoModel> content;
	content.push_back({ L"Title", ConvertToWString(info->title) });
	content.push_back({ L"Cartridge", ConvertToWString(info->header.cartridge_type) });
	content.push_back({ L"Manufacturer", ConvertToWString(info->header.manufacturer_code) });
	content.push_back({ L"Old Licensee", ConvertToWString(info->header.old_licensee) });
	content.push_back({ L"ROM size", std::to_wstring(info->header.rom_size) });
	content.push_back({ L"RAM size", std::to_wstring(info->header.ram_size) });
	content.push_back({ L"Colour mode", ConvertToWString(cartridge->GetColourMode()) });
	CreateContentModel(content, width);
}

void CartridgeInfoWindow::Destroy()
{
	HINSTANCE hInstance = GetModuleHandle(NULL);
	UnregisterClass(m_RegisterClassName.c_str(), hInstance);

	DestroyWindow(m_Hwnd);
	m_Application->ReleaseCartridgeInfoWindow();
}

LRESULT CartridgeInfoWindow::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_CLOSE:
			Destroy();
			return 0;

		case WM_CTLCOLORSTATIC:
		{
			HDC hdc = (HDC)wParam;
			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, RGB(255, 255, 255));
			return (LONG_PTR)(m_BrushBackground);
		}
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void CartridgeInfoWindow::WindowCreate(const std::string& title, int width, int height)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);

	// Convert to wstring
	const std::wstring window_title = ConvertToWString(title);
	m_RegisterClassName = window_title;

	// Setup window class
	WNDCLASS wc = {};
	wc.style = CS_VREDRAW | CS_HREDRAW;
	wc.lpfnWndProc = MainWndProc;
	wc.hInstance = hInstance;
	wc.hIcon = NULL;
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.lpszClassName = m_RegisterClassName.c_str();
	wc.lpszMenuName = NULL;
	RegisterClass(&wc);

	// Compute window rectangle dimensions based on requested client area dimensions.
	RECT window_rect = { 0, 0, width, height };
	AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW, false);
	int window_width = window_rect.right - window_rect.left;
	int window_height = window_rect.bottom - window_rect.top;

	// Create window
	DWORD exStyle = WS_EX_DLGMODALFRAME;
	DWORD dwStyle = (WS_OVERLAPPED | WS_CAPTION | WS_POPUP | WS_SYSMENU);
	HWND parent_window = m_Application->GetMainWindow()->GetHwnd();

	// Get the dimensions of the parent window
	RECT parentRect;
	GetClientRect(parent_window, &parentRect);

	// Calculate the position for the child window
	int x = (parentRect.right - window_width) / 2;
	int y = (parentRect.bottom - window_height) / 2;

	m_Hwnd = CreateWindowEx(exStyle, wc.lpszClassName, window_title.c_str(), dwStyle, x, y, window_width, window_height, parent_window, NULL, hInstance, this);
	if (m_Hwnd == NULL)
	{
		throw std::exception("CreateWindow Failed");
	}

	// Show window
	ShowWindow(m_Hwnd, SW_SHOWNORMAL);
}

void CartridgeInfoWindow::FontCreate()
{
	HDC hdc = GetDC(NULL);
	long lfHeight = -MulDiv(12, GetDeviceCaps(hdc, LOGPIXELSY), 72);
	ReleaseDC(NULL, hdc);

	m_Font = CreateFont(lfHeight, 0, 0, 0, 0, FALSE, 0, 0, 0, 0, 0, 0, 0, L"Arial");
}

void CartridgeInfoWindow::CreateGroupBox(int width, int height)
{
	m_BrushBackground = CreateSolidBrush(RGB(255, 255, 255));

	HWND groupbox = CreateWindowEx(WS_EX_WINDOWEDGE,
								   L"BUTTON",  // Predefined class; Unicode assumed 
								   L"Info",      // Button text 
								   BS_GROUPBOX | WS_GROUP | WS_CHILD | BS_DEFPUSHBUTTON | WS_VISIBLE,  // Styles 
								   10,         // x position 
								   0,          // y position 
								   width - 20,        // Button width
								   height - 10,        // Button height
								   m_Hwnd,     // Parent window
								   NULL,
								   (HINSTANCE)GetWindowLongPtr(m_Hwnd, GWLP_HINSTANCE),
								   NULL);      // Pointer not needed.

	SendMessage(groupbox, WM_SETFONT, (WPARAM)m_Font, TRUE);
}

void CartridgeInfoWindow::CreateContentModel(const std::vector<InfoModel>& content, int window_width)
{
	// Draw text
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(m_Hwnd, &ps);
	FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
	SelectObject(hdc, m_Font);

	// Calculate position
	RECT rect;
	GetClientRect(m_GroupBox, &rect);
	rect.left = 20;
	rect.top = 26;

	int y = 0;
	for (auto& data : content)
	{
		rect.top = 26 + (y * 32);

		DrawText(hdc, data.tag.c_str(), -1, &rect, DT_SINGLELINE | DT_NOCLIP);

		// Textbox control
		int edit_x = window_width / 3;
		int edit_width = (window_width - edit_x) - rect.left;

		HWND textbox = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", data.text.c_str(), WS_CHILD | WS_VISIBLE | WS_DISABLED, edit_x, rect.top, edit_width, 24, m_Hwnd, NULL, (HINSTANCE)GetWindowLongPtr(m_Hwnd, GWLP_HINSTANCE), NULL);
		SendMessage(textbox, WM_SETFONT, (WPARAM)m_Font, TRUE);

		++y;
	}
}