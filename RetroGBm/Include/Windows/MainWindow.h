#pragma once

#include <string>
#include <memory>
#include <filesystem>
#include <RetroGBm/HighTimer.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "Render/RenderTarget.h"
#include "Render/RenderTexture.h"

class Application;

class MainWindow
{
	Application* m_Application = nullptr;

public:
	MainWindow();
	virtual ~MainWindow();

	void Create(const std::string& title, int width, int height);
	void Update();

	LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	inline HWND GetHwnd() { return m_RenderHwnd; }

	void ToggleTileWindowMenuItem(bool checked);
	void ToggleCpuRegistersWindowMenuItem(bool checked);

	void SetStatusBarTitle(const std::string& text);
	void SetStatusBarStats(const std::string& text);
	void SetStatusBarState(const std::string& text);

protected:
	void OnClose();
	void OnKeyPressed(UINT virtual_key_code);

private:
	void CreateMainWindow(const std::string& title, int width, int height);
	HWND m_Hwnd = NULL;
	std::wstring m_RegisterClassName;

	HighTimer m_Timer;
	float m_TimeElapsed = 0;
	int m_FrameCount = 0;
	int m_FramesPerSecond = 0;

	static const UINT m_RenderTimer = 555;

	void LoadRom(const std::string& path);

	// ROM list window
	void CreateRomListWindow();
	HWND m_ListHwnd = NULL;
	static const UINT m_ListMenuId = 4001;
	bool m_ListIsEmpty = false;

	// Render window
	void CreateRenderWindow();
	void ComputeRenderWindowSize(int* width, int* height);
	HWND m_RenderHwnd = NULL;
	std::unique_ptr<Render::RenderTarget> m_MainRenderTarget = nullptr;
	std::unique_ptr<Render::RenderTexture> m_MainRenderTexture = nullptr;

	// Menu bar
	void CreateMenuBar();
	void HandleMenu(UINT msg, WPARAM wParam, LPARAM lParam);
	HMENU m_MenuBar = NULL;

	HMENU m_FileMenuItem = NULL;
	static const UINT m_MenuFileOpenId = 101;
	static const UINT m_MenuFileCloseId = 102;
	static const UINT m_MenuFileRestartId = 104;
	static const UINT m_MenuFileExitId = 103;
	static const UINT m_MenuFileOpenRomDirectoryId = 105;
	static const UINT m_MenuFileRefreshId = 106;

	HMENU m_ToolsMenuItem = NULL;
	static const UINT m_MenuToolsTilemap = 201;
	static const UINT m_MenuToolsTracelog = 202;
	static const UINT m_MenuToolsCartridgeInfo = 203;
	static const UINT m_MenuToolsCpuRegisters = 204;
	static const UINT m_MenuToolsCheats = 205;

	HMENU m_EmulationMenuItem = NULL;
	static const UINT m_MenuEmulationPausePlay = 301;
	static const UINT m_MenuEmulationSaveState = 302;
	static const UINT m_MenuEmulationLoadState = 303;
	static const UINT m_MenuEmulationDoubleSpeed = 304;
	static const UINT m_MenuEmulationScreenshot = 305;

	HMENU m_OptionsMenuItem = NULL;
	static const UINT m_MenuOptionsEnableAudio = 401;
	static const UINT m_MenuOptionsStretchDisplay = 402;
	static const UINT m_MenuOptionsLinearFilter = 403;
	static const UINT m_MenuOptionsNetworkHost = 404;
	static const UINT m_MenuOptionsNetworkConnect = 405;
	static const UINT m_MenuOptionsNetworkDisconnect = 406;
	static const UINT m_MenuOptionsSkipBootRom = 407;

	HMENU m_SaveSlotMenuItem = NULL;
	static const UINT m_MenuSaveSlot1 = 2100;

	HMENU m_LoadSlotMenuItem = NULL;
	static const UINT m_MenuLoadSlot1 = 3100;

	// Status bar
	void CreateStatusBar();
	void ComputeStatusBarSections();
	HWND m_HwndStatusbar = NULL;
	HMENU m_StatusBar = NULL;

	// Other unknown atm
	bool m_StretchDisplay = true;

	void OpenDialog();
	void OpenDialogRomDirectory();
	bool OpenFileDialog(std::string* filepath);
	bool OpenFileDialogRomDirectory(std::string* filepath);
	void ToggleTracelog();
	void Refresh();

	void ToggleEmulationPaused();
	void ToggleEmulationDoubleSpeed();
	void TakeScreenshot();

	std::string m_FilePath;
	void RestartEmulation();

	void HandleKeyboardEvent(UINT msg, WPARAM wParam, LPARAM lParam);
	void HandleKey(bool state, WORD scancode);
	void OnResized(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void UpdateSaveStateDetails();

	// ROM Directories
	std::filesystem::path m_RomPath = "ROMS";
	void RefreshRomList();
};