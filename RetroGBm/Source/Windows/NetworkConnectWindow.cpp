#include "Windows/NetworkConnectWindow.h"
#include "Utilities/Utilities.h"
#include "Application.h"

#include <sstream>
#include <string>
#include <vector>

#include <uxtheme.h>
#include <dwmapi.h>
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")

// Set theme
// https://docs.microsoft.com/en-gb/windows/win32/controls/cookbook-overview?redirectedfrom=MSDN
#pragma comment(linker,"\"/manifestdependency:type='win32' name = 'Microsoft.Windows.Common-Controls' version = '6.0.0.0' processorArchitecture = '*' publicKeyToken = '6595b64144ccf1df' language = '*'\"")

namespace
{
	static NetworkConnectWindow* GetWindow(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		NetworkConnectWindow* window = nullptr;
		if (uMsg == WM_NCCREATE)
		{
			CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
			window = reinterpret_cast<NetworkConnectWindow*>(pCreate->lpCreateParams);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
		}
		else
		{
			window = reinterpret_cast<NetworkConnectWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		}

		return window;
	}

	static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		NetworkConnectWindow* window = GetWindow(hwnd, msg, wParam, lParam);
		if (window == nullptr)
		{
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}

		return window->HandleMessage(hwnd, msg, wParam, lParam);
	}
}

NetworkConnectWindow::NetworkConnectWindow()
{
}

NetworkConnectWindow::~NetworkConnectWindow()
{
	this->Destroy();
}

void NetworkConnectWindow::Create()
{
	this->WindowCreate("Connect to Network", 300, 115);

	// Initialize common controls
	INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_LISTVIEW_CLASSES };
	InitCommonControlsEx(&icex);

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

	// Create components
	const int container_padding = 10;
	int position_y = container_padding;

	// Label component
	{
		int height = 20;

		m_LabelHwnd = CreateWindowW(L"STATIC", L"IP Address:",
			WS_CHILD | WS_VISIBLE,
			container_padding, position_y, 270, height,
			this->GetHwnd(), nullptr, nullptr, nullptr);

		position_y += height;

		SendMessageW(m_LabelHwnd, WM_SETFONT, (WPARAM)m_Font, TRUE);
	}

	// Input component
	{
		int height = 25;

		m_TextboxHwnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", nullptr,
			WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
			container_padding, position_y, 270, height,
			m_Hwnd, (HMENU)m_ControlTextboxId, nullptr, nullptr);

		position_y += height;

		SendMessageW(m_TextboxHwnd, WM_SETFONT, (WPARAM)m_Font, TRUE);
	}

	// Button component
	{
		int button_width = 80;
		int button_height = 30;

		position_y += 10;

		m_ButtonConnectHwnd = CreateWindowW(L"BUTTON", L"Connect",
			WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
			container_padding, position_y, button_width, button_height, // x, y, width, height
			this->GetHwnd(), (HMENU)m_ControlId, nullptr, nullptr);

		position_y += button_height;

		SendMessageW(m_ButtonConnectHwnd, WM_SETFONT, (WPARAM)m_Font, TRUE);
	}
}

void NetworkConnectWindow::Destroy()
{
	if (m_Hwnd != NULL)
	{
		DestroyWindow(m_Hwnd);
		m_Hwnd = NULL;
	}

	HINSTANCE hInstance = GetModuleHandle(NULL);
	UnregisterClassW(m_RegisterClassName.c_str(), hInstance);
}

LRESULT NetworkConnectWindow::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_CTLCOLORSTATIC:
		{
			HDC hdc = (HDC)wParam;
			SetTextColor(hdc, RGB(0, 0, 0));
			SetBkMode(hdc, TRANSPARENT);
			return (LONG_PTR)GetSysColorBrush(COLOR_WINDOW);
		}

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);

			// Clear screen with fill colour
			FillRect(hdc, &ps.rcPaint, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));

			EndPaint(hwnd, &ps);

			return 0;
		}

		case WM_CLOSE:
		{
			this->Destroy();
			return 0;
		}
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void NetworkConnectWindow::WindowCreate(const std::string& title, int width, int height)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);

	// Convert to wstring
	const std::wstring window_title = Utilities::ConvertToWString(title);
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
	DWORD exStyle = WS_EX_DLGMODALFRAME;
	DWORD dwStyle = (WS_OVERLAPPED | WS_CAPTION | WS_POPUP | WS_SYSMENU);
	HWND parent_window = Application::Instance->GetMainWindow()->GetHwnd();

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
