#pragma once

#include "Window.h"
#include <string>
#include <vector>
#include <map>

class Application;

struct InfoModel
{
	std::wstring tag;
	std::wstring text;
};

class CartridgeInfoWindow
{
	Application* m_Application = nullptr;

public:
	CartridgeInfoWindow(Application* application);
	virtual ~CartridgeInfoWindow();

	void Create(const std::string& title, int width, int height);
	void Destroy();
	LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void UpdateCartridgeInfo();

private:
	HFONT m_Font = NULL;
	void FontCreate();

	HBRUSH m_BrushBackground = NULL;

	// Create window
	void WindowCreate(const std::string& title, int width, int height);
	HWND m_Hwnd = NULL;
	std::wstring m_RegisterClassName;

	// Groupbox
	HWND m_GroupBox = NULL;
	void CreateGroupBox(int width, int height);

	// Draw content
	void CreateContentModel(const std::vector<InfoModel>& content, int window_width);
	std::map<std::wstring, HWND> m_ContentText;
};