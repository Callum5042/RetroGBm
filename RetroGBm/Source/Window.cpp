#include "Window.h"
#include <string>
#include <vector>
#include <iostream>

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
			std::wcout << "Key event from window: " << GetWindowTitle() << '\n';
			// HandleKeyboardEvent(msg, wParam, lParam);
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