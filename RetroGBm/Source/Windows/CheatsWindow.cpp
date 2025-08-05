#include "Windows/CheatsWindow.h"
#include "Application.h"

#include <uxtheme.h>
#include <dwmapi.h>
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")

// Set theme
// https://docs.microsoft.com/en-gb/windows/win32/controls/cookbook-overview?redirectedfrom=MSDN
#pragma comment(linker,"\"/manifestdependency:type='win32' name = 'Microsoft.Windows.Common-Controls' version = '6.0.0.0' processorArchitecture = '*' publicKeyToken = '6595b64144ccf1df' language = '*'\"")

namespace
{
	static CheatsWindow* GetWindow(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		CheatsWindow* window = nullptr;
		if (uMsg == WM_NCCREATE)
		{
			CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
			window = reinterpret_cast<CheatsWindow*>(pCreate->lpCreateParams);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
		}
		else
		{
			window = reinterpret_cast<CheatsWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		}

		return window;
	}

	static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		CheatsWindow* window = GetWindow(hwnd, msg, wParam, lParam);
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

CheatsWindow::CheatsWindow(Application* application) : m_Application(application)
{
}

CheatsWindow::~CheatsWindow()
{
	this->Destroy();
}

void CheatsWindow::Create()
{
	this->WindowCreate("Cheats", 400, 300);

	static HFONT m_Font;

	// Create font
	{
		HDC hdc = GetDC(NULL);
		long lfHeight = -MulDiv(12, GetDeviceCaps(hdc, LOGPIXELSY), 72);
		ReleaseDC(NULL, hdc);

		m_Font = CreateFont(lfHeight, 0, 0, 0, 0, FALSE, 0, 0, 0, 0, 0, 0, 0, L"Arial");

		/*m_Font = CreateFont(
			-MulDiv(9, GetDeviceCaps(hdc, LOGPIXELSY), 72),
			0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Segoe UI"));*/
	}

	// Label: "Cheat name"
	m_LabelName = CreateWindowW(L"STATIC", L"Cheat name:",
		WS_CHILD | WS_VISIBLE,
		20, 20, 100, 20,
		m_Hwnd, nullptr, nullptr, nullptr);

	SendMessageW(m_LabelName, WM_SETFONT, (WPARAM)m_Font, TRUE);

	// Single-line Edit: Cheat name
	m_EditName = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", nullptr,
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
		20, 45, 340, 25,
		m_Hwnd, nullptr, nullptr, nullptr);

	SendMessageW(m_EditName, WM_SETFONT, (WPARAM)m_Font, TRUE);

	// Label: "Cheat code"
	m_LabelCode = CreateWindowW(L"STATIC", L"Cheat code:",
		WS_CHILD | WS_VISIBLE,
		20, 80, 100, 20,
		m_Hwnd, nullptr, nullptr, nullptr);

	SendMessageW(m_LabelCode, WM_SETFONT, (WPARAM)m_Font, TRUE);

	// Multi-line Edit: Cheat code
	m_EditCode = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", nullptr,
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
		20, 105, 340, 120,
		m_Hwnd, nullptr, nullptr, nullptr);

	SendMessageW(m_EditCode, WM_SETFONT, (WPARAM)m_Font, TRUE);

	// Button: Add
	m_ButtonAdd = CreateWindowW(L"BUTTON", L"Add",
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
		20, 235, 160, 30, // x, y, width, height
		m_Hwnd, (HMENU)1001, nullptr, nullptr);

	SendMessageW(m_ButtonAdd, WM_SETFONT, (WPARAM)m_Font, TRUE);

	// Button: Delete
	m_ButtonDelete = CreateWindowW(L"BUTTON", L"Delete",
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
		200, 235, 160, 30,
		m_Hwnd, (HMENU)1002, nullptr, nullptr);

	SendMessageW(m_ButtonDelete, WM_SETFONT, (WPARAM)m_Font, TRUE);
}

void CheatsWindow::Destroy()
{
	HINSTANCE hInstance = GetModuleHandle(NULL);
	UnregisterClass(m_RegisterClassName.c_str(), hInstance);

	DestroyWindow(m_Hwnd);
}

void CheatsWindow::WindowCreate(const std::string& title, int width, int height)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);


	m_BrushBackground = CreateSolidBrush(RGB(255, 255, 255));

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

LRESULT CheatsWindow::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_CLOSE:
			Destroy();
			return 0;

		case WM_CTLCOLORSTATIC:
		{
			HDC hdc = (HDC)wParam;
			SetTextColor(hdc, RGB(0, 0, 0));
			SetBkMode(hdc, TRANSPARENT);
			return (LONG_PTR)GetSysColorBrush(COLOR_WINDOW);
		}
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}