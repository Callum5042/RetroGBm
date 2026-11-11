#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <string>

class Application;

// Tools > Tile Data: shows the tile data in VRAM ($8000-$97FF) for each bank
class TileWindow
{
public:
	TileWindow(Application* application);
	virtual ~TileWindow();

	void Create();
	void Destroy();

	inline bool IsOpen() const { return m_Hwnd != NULL; }
	void Focus();

	// Called every frame while the emulator is running
	void Update();

	// Handles Tab for this window. Returns true if the message was consumed.
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

	// CGB has two VRAM banks and 8 BG + 8 OBJ palettes, DMG has one bank and BGP, OBP0 and OBP1
	bool m_IsColour = false;
	void ApplyMode();
	void UpdateStatus();

	// Layout
	static const int m_ClientWidth = 1280;
	static const int m_ClientHeight = 780;
	static const int m_CommandBarHeight = 48;
	static const int m_InspectorWidth = 312;

	HFONT m_Font = NULL;
	HFONT m_HeadingFont = NULL;
	HBRUSH m_InspectorBrush = NULL;

	// Command bar
	HWND m_LabelBank = NULL;
	HWND m_ButtonBank0 = NULL;
	HWND m_ButtonBank1 = NULL;
	HWND m_ButtonBankBoth = NULL;
	HWND m_LabelPalette = NULL;
	HWND m_ComboPalette = NULL;
	HWND m_LabelZoom = NULL;
	HWND m_ComboZoom = NULL;
	HWND m_CheckGrid = NULL;
	HWND m_ButtonPause = NULL;
	HWND m_ButtonExport = NULL;

	// Inspector
	HWND m_LabelTileHeading = NULL;
	HWND m_LabelTileHint = NULL;

	// Status bar: mode and live/paused on the left, the hovered tile on the right
	HWND m_StatusBar = NULL;

	static const int m_ControlBank0Id = 1000;
	static const int m_ControlBank1Id = 1001;
	static const int m_ControlBankBothId = 1002;
	static const int m_ControlPaletteId = 1003;
	static const int m_ControlZoomId = 1004;
	static const int m_ControlGridId = 1005;
	static const int m_ControlPauseId = 1006;
	static const int m_ControlExportId = 1007;
	static const int m_ControlStatusBarId = 1008;
};
