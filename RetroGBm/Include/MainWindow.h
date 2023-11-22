#pragma once

#include "Window.h"
#include <string>

class MainWindow : public Window
{
public:
	MainWindow(Application* application);
	virtual ~MainWindow() = default;

	virtual void Create(const std::string& title, int width, int height) override;

	void ToggleTileWindowMenuItem(bool checked);

protected:
	virtual void OnClose() override;

	virtual void OnKeyPressed(UINT virtual_key_code) override;

	virtual LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

private:

	virtual void HandleMenu(UINT msg, WPARAM wParam, LPARAM lParam) override;
	static const UINT m_MenuFileOpenId = 101;
	static const UINT m_MenuFileCloseId = 102;
	static const UINT m_MenuFileRestartId = 104;
	static const UINT m_MenuFileExitId = 103;

	HMENU m_DebugMenuItem = NULL;
	static const UINT m_MenuDebugTilemap = 201;
	static const UINT m_MenuDebugTracelog = 202;
	static const UINT m_MenuDebugCartridgeInfo = 203;

	HMENU m_EmulationMenuItem = NULL;
	static const UINT m_MenuEmulationPausePlay = 301;
	static const UINT m_MenuEmulationSaveState = 302;
	static const UINT m_MenuEmulationLoadState = 303;

	void OpenDialog();
	bool OpenFileDialog(std::string* filepath);
	void ToggleTracelog();

	void ToggleEmulationPaused();

	std::string m_FilePath;
	void RestartEmulation();
};