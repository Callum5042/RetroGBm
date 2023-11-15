#pragma once

#include "Window.h"

class Application;

class CartridgeInfoWindow
{
	Application* m_Application = nullptr;

public:
	CartridgeInfoWindow(Application* application);
	virtual ~CartridgeInfoWindow();

	void Create(const std::string& title, int width, int height);
	void Destroy();
	LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	HFONT m_Font = NULL;
	HBRUSH m_BrushBackground = NULL;

	// Create window
	void CreateHwnd(const std::string& title, int width, int height);
	HWND m_Hwnd = NULL;
	std::wstring m_RegisterClassName;

	// Create controls
};