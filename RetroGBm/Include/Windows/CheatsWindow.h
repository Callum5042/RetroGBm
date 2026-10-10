#pragma once

#include "Window.h"
#include <string>

class Application;

class CheatsWindow
{
	Application* m_Application = nullptr;

public:
	CheatsWindow(Application* application);
	virtual ~CheatsWindow();

	void Create();
	void Destroy();

	inline bool IsOpen() const { return m_Hwnd != NULL; }
	void Focus();

	// Handles Tab, Enter and Esc for this window. Returns true if the message was consumed.
	bool HandleDialogMessage(MSG* msg);

	LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:

	// Create window
	void WindowCreate(const std::string& title, int width, int height);
	void PaintBackground(HDC hdc);
	HWND m_Hwnd = NULL;
	std::wstring m_RegisterClassName;

	// Layout
	static const int m_ClientWidth = 640;
	static const int m_ClientHeight = 388;
	static const int m_FooterHeight = 44;

	HFONT m_Font = NULL;
	HWND m_LastFocus = NULL;

	// Footer
	HWND m_LabelFooter = NULL;
	HWND m_ButtonClose = NULL;

	// UI
	HWND m_LabelName = NULL;
	HWND m_EditName = NULL;

	HWND m_LabelCode = NULL;
	HWND m_EditCode = NULL;

	HWND m_ButtonAdd = NULL;
	HWND m_ButtonUpdate = NULL;
	HWND m_ButtonDelete = NULL;

	const int m_ControlListViewId = 1000;
	const int m_ControlEditNameId = 1001;
	const int m_ControlEditCodeId = 1002;
	const int m_ControlAddButtonId = 2001;
	const int m_ControlUpdateButtonId = 2002;
	const int m_ControlDeleteButtonId = 2003;

	// ListView
	HWND m_ListHwnd = NULL;
	static const UINT m_ListMenuId = 5001;

	// Cheat codes
	int m_SelectedCheatCodeIndex = -1;
};