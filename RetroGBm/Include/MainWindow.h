#pragma once

#include "Window.h"

class MainWindow : public Window
{
public:
	MainWindow(Application* application);
	virtual ~MainWindow() = default;

	virtual void Create(const std::string& title, int width, int height) override;

private:

	virtual void HandleMenu(UINT msg, WPARAM wParam, LPARAM lParam) override;
	static const UINT m_MenuFileOpenId = 101;
	static const UINT m_MenuFileCloseId = 102;
	static const UINT m_MenuFileExitId = 103;

	HMENU m_DebugMenuItem;
	static const UINT m_MenuDebugTilemap = 201;
	static const UINT m_MenuDebugTracelog = 202;

	void OpenDialog();
	bool OpenFileDialog(std::string* filepath);
	void ToggleTracelog();
};