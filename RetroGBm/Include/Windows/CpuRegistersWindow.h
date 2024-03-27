#pragma once

#include "Window.h"
#include <string>

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
};