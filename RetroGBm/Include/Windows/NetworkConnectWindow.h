#pragma once

#include "Window.h"
#include <string>

class NetworkConnectWindow
{
public:
	NetworkConnectWindow();
	virtual ~NetworkConnectWindow();

	void Create();
	void Destroy();

	LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:


	void WindowCreate(const std::string& title, int width, int height);
	HWND m_Hwnd = NULL;
	std::wstring m_RegisterClassName;

	HWND m_LabelName;

	HWND m_ButtonAdd;
	const int m_ControlAddButtonId = 2001;
};