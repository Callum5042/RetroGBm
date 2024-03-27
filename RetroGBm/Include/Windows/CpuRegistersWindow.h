#pragma once

#include "Window.h"
#include <string>
#include <map>
#include <thread>

class CpuRegisterWindow
{
public:
	CpuRegisterWindow();
	virtual ~CpuRegisterWindow();

	void Create();
	void Destroy();

	LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:

	void WindowCreate(const std::string& title, int width, int height);
	HWND m_Hwnd = NULL;
	std::wstring m_RegisterClassName;

	// Group box
	HFONT m_Font = NULL;
	HBRUSH m_BrushBackground = NULL;
	HWND m_GroupBox = NULL;

	// Edit textboxes
	std::map<std::string, HWND> m_TextBoxes;

	std::thread m_CpuPollThread;
	std::atomic_bool m_ThreadPolling = false;
};