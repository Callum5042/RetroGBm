#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <string>

class Application;

// Tools > Memory: a live hex view of the Game Boy's 64 KiB address space
class MemoryWindow
{
public:
	MemoryWindow(Application* application);
	virtual ~MemoryWindow();

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

	void WindowCreate(int width, int height);
	void CreateControls();
	void Layout();
	void PaintBackground(HDC hdc);

	// Fonts and brushes, released once the window and its controls are gone
	void ReleaseResources();

	// Title shows the loaded ROM, which can change while the window is open
	std::string m_TitleFilename;
	void UpdateTitle();

	void UpdateStatus();

	// Layout
	static const int m_ClientWidth = 1280;
	static const int m_ClientHeight = 780;
	static const int m_CommandBarHeight = 48;
	static const int m_InspectorWidth = 300;

	HFONT m_Font = NULL;
	HBRUSH m_InspectorBrush = NULL;

	// Status bar: what is shown on the left, live or paused on the right
	HWND m_StatusBar = NULL;

	static const int m_ControlStatusBarId = 1000;
};
