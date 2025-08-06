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

	LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:

	// Create window
	void WindowCreate(const std::string& title, int width, int height);
	HWND m_Hwnd = NULL;
	std::wstring m_RegisterClassName;

	HBRUSH m_BrushBackground = NULL;

	// UI
	HWND m_LabelName = NULL;
	HWND m_EditName = NULL;

	HWND m_LabelCode = NULL;
	HWND m_EditCode = NULL;

	HWND m_ButtonAdd = NULL;
	HWND m_ButtonUpdate = NULL;
	HWND m_ButtonDelete = NULL;

	const int m_ControlListViewId = 1;
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