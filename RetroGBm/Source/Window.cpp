#include "Window.h"
#include <string>
#include <vector>
#include <iostream>
#include "Application.h"
#include <Emulator.h>
#include <Joypad.h>

namespace
{
	static Window* GetWindow(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		Window* window = nullptr;
		if (uMsg == WM_NCCREATE)
		{
			CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
			window = reinterpret_cast<Window*>(pCreate->lpCreateParams);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
		}
		else
		{
			window = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		}

		return window;
	}

	static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		Window* window = GetWindow(hwnd, msg, wParam, lParam);
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

Window::Window(Application* application) : m_Application(application)
{
}

Window::~Window()
{
	DestroyWindow(m_Window);
}

void Window::Create(const std::string& title, int width, int height)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);
	const std::wstring window_title = ConvertToWString(title);

	// Setup window class
	WNDCLASS wc = {};
	wc.style = CS_VREDRAW | CS_HREDRAW;
	wc.lpfnWndProc = MainWndProc;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.lpszClassName = window_title.c_str();

	if (!RegisterClass(&wc))
	{
		throw std::exception("RegisterClass Failed");
	}

	// Create window
	m_Window = CreateWindow(wc.lpszClassName, window_title.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, NULL, NULL, hInstance, this);
	if (m_Window == NULL)
	{
		throw std::exception("CreateWindow Failed");
	}

	// Show window
	ShowWindow(m_Window, SW_SHOWNORMAL);
}

LRESULT Window::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		case WM_KEYDOWN:
		case WM_KEYUP:
			// std::wcout << "Key event from window: " << GetWindowTitle() << '\n';
			HandleKeyboardEvent(msg, wParam, lParam);
			return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void Window::GetSize(int* width, int* height)
{
	RECT rect;
	GetClientRect(m_Window, &rect);

	*width = rect.right - rect.left;
	*height = rect.bottom - rect.top;
}

std::wstring Window::GetWindowTitle()
{
	int window_tile_length = GetWindowTextLength(m_Window) + 1;

	// std::wstring title;
	std::vector<wchar_t> title(window_tile_length);
	GetWindowText(m_Window, title.data(), window_tile_length);

	return std::wstring(title.data(), title.size());
}

void Window::HandleKeyboardEvent(UINT msg, WPARAM wParam, LPARAM lParam)
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
		HandleKey(true, scan_code);
	}
	else if (msg == WM_KEYUP || msg == WM_SYSKEYUP)
	{
		HandleKey(false, scan_code);
	}
}

void Window::HandleKey(bool state, WORD scancode)
{
	if (m_Application == nullptr)
	{
		return;
	}

	const WORD ZKey = 0x5A;
	const WORD XKey = 0x58;

	UINT key = MapVirtualKeyW(scancode, MAPVK_VSC_TO_VK_EX);

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