#pragma once

#include <string>
#include <Windows.h>
#include <memory>
#include <thread>
#include <RetroGBm/HighTimer.h>

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

	HMENU m_ToolsMenuItem = NULL;
	static const UINT m_MenuToolsCpuRegisters = 204;
	static const UINT m_MenuToolsTilemap = 201;
	static const UINT m_MenuToolsTracelog = 202;
	static const UINT m_MenuToolsCartridgeInfo = 203;

	HMENU m_EmulationMenuItem = NULL;
	static const UINT m_MenuEmulationPausePlay = 301;
	static const UINT m_MenuEmulationSaveState = 302;
	static const UINT m_MenuEmulationLoadState = 303;

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

	void OpenDialog();
	bool OpenFileDialog(std::string* filepath);
	void ToggleTracelog();

	void ToggleEmulationPaused();

	std::string m_FilePath;
	void RestartEmulation();

	void HandleKeyboardEvent(UINT msg, WPARAM wParam, LPARAM lParam);
	void HandleKey(bool state, WORD scancode);
	void OnResized(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void UpdateSaveStateDetails();
};