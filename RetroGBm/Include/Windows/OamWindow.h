#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "Windows/OamGridView.h"
#include "Windows/OamScreenView.h"

#include <RetroGBm/Oam.h>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

class Application;

// Tools > Objects (OAM): the 40 objects in OAM, with which ones the 10-objects-per-line limit drops
class OamWindow
{
public:
	OamWindow(Application* application);
	virtual ~OamWindow();

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

	// CGB has a second VRAM bank and 8 object palettes, picked by different attribute bits
	bool m_IsColour = false;
	void UpdateStatus();

	// Title shows the loaded ROM, which can change while the window is open
	std::string m_TitleFilename;
	void UpdateTitle();

	// Copy of OAM, the tile data in VRAM ($8000-$97FF), LCDC and the object palettes,
	// taken every frame unless updates are paused
	struct Snapshot
	{
		std::array<uint8_t, Oam::Size> oam = {};
		std::array<std::vector<uint8_t>, 2> tiles;
		uint8_t lcdc = 0;
		std::array<std::array<uint32_t, 4>, 8> colours = {};

		// Which object wins where they overlap: the one further left (DMG, or CGB set to it by OPRI),
		// else the one earlier in OAM
		bool coordinate_priority = true;

		// Last frame as 0x00RRGGBB, only taken while the Screen view shows it as the background
		std::vector<uint32_t> frame;
	};

	bool m_HasSnapshot = false;
	Snapshot m_Snapshot;
	void CaptureSnapshot();

	// What the Size list picks, resolved against LCDC.2
	enum class SizeChoice : int { Auto = 0, Small = 1, Tall = 2 };
	int ObjectHeight() const;

	// Which lines each object is dropped from, for the snapshot and size
	Oam::LineScan m_LineScan;

	// Builds the grid's cells, or the screen's scene, from the snapshot
	void RenderObjects();
	void RenderScreen();
	OamGridView m_GridView;
	OamScreenView m_ScreenView;

	// Objects shows the grid, Screen shows where they are on screen
	bool IsScreenView() const;
	void ShowView();

	// Whether the Screen view draws the game's last frame under the objects
	bool ShowBackground() const;

	// The selected object, shared by both views and the inspector
	int m_Selection = -1;
	void SelectionChanged(int index);

	// Palette the object uses: its CGB palette (0-7), or OBP0 or OBP1 on DMG
	int PaletteOf(const Oam::Object& object) const;
	std::wstring PaletteName(const Oam::Object& object) const;

	// Object pixels as the views want them: 0xFFRRGGBB where drawn, 0 where transparent
	std::vector<uint32_t> ObjectPixels(const Oam::Object& object, int height) const;

	// Inspector: details of the selected object
	bool m_InspectorHasObject = false;
	bool m_InspectorHasWarning = false;
	std::vector<uint32_t> m_PreviewPixels;
	int m_PreviewHeight = 8;
	std::array<uint32_t, 4> m_PreviewColours = {};
	std::wstring m_PreviewPaletteName;
	std::wstring m_WarningTitle;
	std::wstring m_WarningText;
	int m_WarningHeight = 0;
	void UpdateInspector();
	void DrawPreview(const DRAWITEMSTRUCT* item);
	void DrawWarning(const DRAWITEMSTRUCT* item);

	// Where the object is, or why it can't be seen
	std::wstring PositionText(const Oam::Object& object, int index) const;

	// Status bar: the object under the mouse in either view
	void UpdateHoverStatus(int index);

	// Opens the selected object's first tile in the Tile Data window
	void OpenSelectedTile();

	// Whether hidden objects are in the grid, and so in the export
	bool ShowHidden() const;

	// Saves the objects in the grid as a PNG, 8 to a row at the current zoom, with transparent backgrounds
	void ExportPng();

	// Layout
	static const int m_ClientWidth = 1000;
	static const int m_ClientHeight = 720;
	static const int m_CommandBarHeight = 48;
	static const int m_InspectorWidth = 312;

	HFONT m_Font = NULL;
	HFONT m_BoldFont = NULL;
	HFONT m_HeadingFont = NULL;
	HFONT m_MonoFont = NULL;
	HFONT m_MonoSmallFont = NULL;
	HBRUSH m_InspectorBrush = NULL;

	// Command bar
	HWND m_ButtonObjects = NULL;
	HWND m_ButtonScreen = NULL;
	HWND m_LabelSize = NULL;
	HWND m_ComboSize = NULL;
	HWND m_LabelZoom = NULL;
	HWND m_ComboZoom = NULL;
	HWND m_CheckShowHidden = NULL;
	HWND m_CheckBackground = NULL;
	HWND m_CheckOutlines = NULL;
	HWND m_CheckLineLimit = NULL;
	HWND m_ButtonPause = NULL;
	HWND m_ButtonExport = NULL;

	// Inspector
	HWND m_LabelHeading = NULL;
	HWND m_LabelSubtitle = NULL;
	HWND m_Preview = NULL;
	std::array<HWND, 8> m_PropertyLabels = {};
	std::array<HWND, 8> m_PropertyValues = {};
	HWND m_Warning = NULL;
	HWND m_ButtonOpenTile = NULL;
	HWND m_LabelHint = NULL;

	// Status bar: mode, object count and line limit on the left, the hovered object on the right
	HWND m_StatusBar = NULL;

	static const int m_ControlPauseId = 1000;
	static const int m_ControlStatusBarId = 1001;
	static const int m_ControlSizeId = 1002;
	static const int m_ControlGridId = 1003;
	static const int m_ControlPreviewId = 1004;
	static const int m_ControlWarningId = 1005;
	static const int m_ControlOpenTileId = 1006;
	static const int m_ControlZoomId = 1007;
	static const int m_ControlShowHiddenId = 1008;
	static const int m_ControlExportId = 1009;
	static const int m_ControlObjectsId = 1010;
	static const int m_ControlScreenId = 1011;
	static const int m_ControlScreenViewId = 1012;
	static const int m_ControlBackgroundId = 1013;
	static const int m_ControlOutlinesId = 1014;
	static const int m_ControlLineLimitId = 1015;
};
