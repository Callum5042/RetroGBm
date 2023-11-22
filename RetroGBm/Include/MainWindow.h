#pragma once

#include <string>
#include <Windows.h>

class Application;

class MainWindow
{
	Application* m_Application = nullptr;

public:
	MainWindow(Application* application);
	virtual ~MainWindow() = default;

	void Create(const std::string& title, int width, int height);

	void ToggleTileWindowMenuItem(bool checked);

	inline HWND GetHwnd() { return m_Hwnd; }

	LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
	void OnClose();
	void OnKeyPressed(UINT virtual_key_code);

private:
	HWND m_Hwnd = NULL;
	HWND m_RenderHwnd = NULL;
	HWND m_HwndStatusbar = NULL;

	int m_StatusBarHeight = 0;

	void HandleMenu(UINT msg, WPARAM wParam, LPARAM lParam);
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

	void SomethingInit();
	HMENU m_StatusBar = NULL;

	std::wstring m_RegisterClassName;

	void HandleKeyboardEvent(UINT msg, WPARAM wParam, LPARAM lParam);
	void HandleKey(bool state, WORD scancode);
	void OnResized(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};