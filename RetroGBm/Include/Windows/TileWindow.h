#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "Windows/TileExportDialog.h"
#include "Windows/TileSheetView.h"

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

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

	// Pins a tile (0-383) in a bank, showing that bank if it's hidden
	void SelectTile(int bank, int tile);

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

	// Fonts and brushes, released once the window and its controls are gone
	void ReleaseResources();

	// CGB has two VRAM banks and 8 BG + 8 OBJ palettes, DMG has one bank and BGP, OBP0 and OBP1
	bool m_IsColour = false;
	void ApplyMode();
	void UpdateStatus();

	// Title shows the loaded ROM, which can change while the window is open
	std::string m_TitleFilename;
	void UpdateTitle();

	// Copy of VRAM and the palettes, taken every frame unless updates are paused
	bool m_HasSnapshot = false;
	std::array<std::vector<uint8_t>, 2> m_Vram;
	std::array<std::array<uint32_t, 4>, 8> m_BackgroundColours = {};
	std::array<std::array<uint32_t, 4>, 8> m_ObjectColours = {};
	void CaptureSnapshot();

	// Draws the snapshot with the selected banks and palette
	std::vector<int> VisibleBanks() const;
	void RenderSheets();
	TileSheetView m_SheetView;

	// The selected palette's 4 colours (0x00RRGGBB). Colour 0 of an object palette is transparent.
	struct PaletteColours
	{
		std::array<uint32_t, 4> colours = {};
		bool transparent = false;
	};
	PaletteColours SelectedPalette() const;
	PaletteColours PaletteFromItemData(LRESULT data) const;

	// Inspector: details of the selected tile
	bool m_InspectorHasTile = false;
	std::array<uint32_t, 64> m_PreviewPixels = {};
	PaletteColours m_PreviewPalette;
	void UpdateInspector();
	void DrawPreview(const DRAWITEMSTRUCT* item);
	void CopySelectedBytes();

	// Status bar: the tile under the mouse
	void UpdateHoverStatus();

	// Export as PNG, from the current snapshot
	std::unique_ptr<TileExportDialog> m_ExportDialog;
	void OpenExportDialog();

	// Layout
	static const int m_ClientWidth = 1280;
	static const int m_ClientHeight = 780;
	static const int m_CommandBarHeight = 48;
	static const int m_InspectorWidth = 312;

	HFONT m_Font = NULL;
	HFONT m_HeadingFont = NULL;
	HFONT m_BoldFont = NULL;
	HFONT m_MonoFont = NULL;
	HFONT m_MonoSmallFont = NULL;
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
	HWND m_LabelTileSubtitle = NULL;
	HWND m_TilePreview = NULL;
	std::array<HWND, 5> m_PropertyLabels = {};
	std::array<HWND, 5> m_PropertyValues = {};
	HWND m_LabelBytes = NULL;
	HWND m_ButtonCopy = NULL;
	HWND m_EditBytes = NULL;
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
	static const int m_ControlSheetViewId = 1009;
	static const int m_ControlCopyId = 1010;
	static const int m_ControlPreviewId = 1011;
	static const int m_ControlBytesId = 1012;
};
