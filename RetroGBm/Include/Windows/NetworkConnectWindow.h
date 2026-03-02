#pragma once

#include "Window.h"
#include <string>
#include <vector>

class NetworkConnectWindow
{
public:
	NetworkConnectWindow();
	virtual ~NetworkConnectWindow();

	void Create();
	void Destroy();

	LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	inline HWND GetHwnd() const
	{
		return m_Hwnd;
	}

private:


	void WindowCreate(const std::string& title, int width, int height);
	HWND m_Hwnd = NULL;
	std::wstring m_RegisterClassName;

	HWND m_LabelName;

	HWND m_ButtonAdd;
	const int m_ControlAddButtonId = 2001;

	// Font
	HFONT m_Font;

	// Label
	HWND m_LabelHwnd = NULL;

	// Textbox
	HWND m_TextboxHwnd = NULL;
	int m_ControlTextboxId = 1001;

	// Buttons
	HWND m_ButtonConnectHwnd = NULL;
	int m_ControlId = 2000;
};