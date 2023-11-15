#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <string>

class Application;
namespace Render
{
	class RenderTarget;
}

class Window
{
protected:
	Application* m_Application = nullptr;
	Render::RenderTarget* m_RenderTarget = nullptr;

public:
	Window() = default;
	Window(Application* application);
	virtual ~Window();

	virtual void Create(const std::string& title, int width, int height);
	virtual LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	std::wstring GetWindowTitle();

	// Get window size
	void GetSize(int* width, int* height);

	// Get Win32 handle
	inline HWND GetHwnd() { return m_Hwnd; }

	// Attach render target
	void AttachRenderTarget(Render::RenderTarget* renderTarget);

protected:
	HWND m_Hwnd = nullptr;

	virtual void OnClose();

private:
	void HandleKeyboardEvent(UINT msg, WPARAM wParam, LPARAM lParam);
	virtual void HandleMenu(UINT msg, WPARAM wParam, LPARAM lParam);
	void HandleKey(bool state, WORD scancode);
	void OnResized(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	std::wstring m_RegisterClassName;
};