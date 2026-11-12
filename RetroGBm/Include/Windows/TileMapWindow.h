#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "Windows/TileMapView.h"

#include <RetroGBm/TileMap.h>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

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

	// CGB has a second VRAM bank holding the attributes, and 8 background palettes
	bool m_IsColour = false;
	void UpdateStatus();

	// Title shows the loaded ROM, which can change while the window is open
	std::string m_TitleFilename;
	void UpdateTitle();

	// Copy of VRAM ($8000-$9FFF), the LCD registers and the background palettes,
	// taken every frame unless updates are paused
	struct Snapshot
	{
		std::array<std::vector<uint8_t>, 2> vram;
		uint8_t lcdc = 0;
		uint8_t scy = 0;
		uint8_t scx = 0;
		uint8_t wy = 0;
		uint8_t wx = 0;
		std::array<std::array<uint32_t, 4>, 8> colours = {};
	};

	bool m_HasSnapshot = false;
	Snapshot m_Snapshot;
	void CaptureSnapshot();

	// What the Map and Tile data lists pick, resolved against LCDC
	enum class MapChoice : int { Background = 0, Window = 1, Low = 2, High = 3 };
	enum class AddressingChoice : int { Auto = 0, Unsigned = 1, Signed = 2 };
	MapChoice SelectedMap() const;
	uint16_t MapAddress() const;
	TileMap::Addressing Addressing() const;

	// Draws the snapshot with the selected map and addressing
	void RenderMap();
	TileMapView m_MapView;
	std::vector<uint32_t> m_MapPixels;

	// Saves the map as shown, at the current zoom and with the grid if it's on
	void ExportPng();

	// Inspector: details of the selected map entry
	bool m_InspectorHasEntry = false;
	std::array<uint32_t, 64> m_PreviewPixels = {};
	std::array<uint32_t, 4> m_PreviewColours = {};
	std::wstring m_PreviewPaletteName;
	void UpdateInspector();
	void DrawPreview(const DRAWITEMSTRUCT* item);

	// Where a map entry is drawn on screen, if anywhere, as the background or the window
	std::wstring ScreenPositionText(int entry) const;

	// Status bar: the entry under the mouse
	void UpdateHoverStatus();

	// Opens the selected entry's tile in the Tile Data window
	void OpenSelectedTile();

	// Layout
	static const int m_ClientWidth = 1000;
	static const int m_ClientHeight = 720;
	static const int m_CommandBarHeight = 48;
	static const int m_InspectorWidth = 312;

	HFONT m_Font = NULL;
	HFONT m_HeadingFont = NULL;
	HFONT m_MonoFont = NULL;
	HFONT m_MonoSmallFont = NULL;
	HBRUSH m_InspectorBrush = NULL;

	// Command bar
	HWND m_LabelMap = NULL;
	HWND m_ComboMap = NULL;
	HWND m_LabelAddressing = NULL;
	HWND m_ComboAddressing = NULL;
	HWND m_LabelZoom = NULL;
	HWND m_ComboZoom = NULL;
	HWND m_CheckGrid = NULL;
	HWND m_CheckOverlay = NULL;
	HWND m_ButtonPause = NULL;
	HWND m_ButtonExport = NULL;

	// Inspector
	HWND m_LabelHeading = NULL;
	HWND m_LabelSubtitle = NULL;
	HWND m_Preview = NULL;
	std::array<HWND, 7> m_PropertyLabels = {};
	std::array<HWND, 7> m_PropertyValues = {};
	HWND m_LabelHint = NULL;

	// Status bar: mode and live/paused on the left, the hovered entry on the right
	HWND m_StatusBar = NULL;

	static const int m_ControlPauseId = 1000;
	static const int m_ControlStatusBarId = 1001;
	static const int m_ControlMapId = 1002;
	static const int m_ControlAddressingId = 1003;
	static const int m_ControlZoomId = 1004;
	static const int m_ControlGridId = 1005;
	static const int m_ControlMapViewId = 1006;
	static const int m_ControlOverlayId = 1007;
	static const int m_ControlPreviewId = 1008;
	static const int m_ControlExportId = 1009;
};
