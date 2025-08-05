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
	HWND m_ButtonDelete = NULL;
};