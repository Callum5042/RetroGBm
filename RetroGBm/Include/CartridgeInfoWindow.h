#pragma once

#include "Window.h"

class CartridgeInfoWindow : public Window
{
public:
	CartridgeInfoWindow() = default;
	virtual ~CartridgeInfoWindow() = default;

	void Create(const std::string& title, int width, int height) override;
	LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

private:
	HFONT m_Font = NULL;
	HBRUSH m_BrushBackground = NULL;
};