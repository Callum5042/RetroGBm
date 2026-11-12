#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <string>

class Application;

// Tools > Tile Map: shows the whole of a 32 x 32 tile map in VRAM ($9800 or $9C00), with the part on screen marked
class TileMapWindow
{
public:
	TileMapWindow(Application* application);
	virtual ~TileMapWindow();

	void Create();
	void Destroy();

	inline bool IsOpen() const { return m_Hwnd != NULL; }
	void Focus();

	// Called every frame while the emulator is running
	void Update();

	// Handles Tab and Esc for this window. Returns true if the message was consumed.
	bool HandleDialogMessage(MSG* msg);

	LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	Application* m_Application = nullptr;

	HWND m_Hwnd = NULL;
	HWND m_LastFocus = NULL;

	void WindowCreate(int width, int height);
	void CreateControls();
	void Layout();
	void PaintBackground(HDC hdc);

	// Fonts and brushes, released once the window and its controls are gone
	void ReleaseResources();

	bool m_IsColour = false;
	void UpdateStatus();

	// Title shows the loaded ROM, which can change while the window is open
	std::string m_TitleFilename;
	void UpdateTitle();

	// Layout
	static const int m_ClientWidth = 1100;
	static const int m_ClientHeight = 720;
	static const int m_CommandBarHeight = 48;
	static const int m_InspectorWidth = 312;

	HFONT m_Font = NULL;
	HBRUSH m_InspectorBrush = NULL;

	// Command bar
	HWND m_ButtonPause = NULL;

	// Status bar: mode and live/paused on the left, the hovered entry on the right
	HWND m_StatusBar = NULL;

	static const int m_ControlPauseId = 1000;
	static const int m_ControlStatusBarId = 1001;
};
