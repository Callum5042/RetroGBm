#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <RetroGBm/TileData.h>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

// Export as PNG: saves or copies the tile data, from a copy of VRAM taken when the dialog opens.
// Disables its owner while open.
class TileExportDialog
{
public:
	struct Palette
	{
		std::wstring name;

		// 0x00RRGGBB
		std::array<uint32_t, 4> colours = {};

		// Colour 0 of an object palette is transparent
		bool object = false;
	};

	enum class Layout : int { Bank0 = 0, Bank1 = 1, SideBySide = 2, Stacked = 3 };

	struct Source
	{
		bool is_colour = false;
		std::array<std::vector<uint8_t>, 2> vram;
		std::vector<Palette> palettes;
		int palette = 0;
		Layout layout = Layout::Bank0;

		// Used for the default file name
		std::wstring rom_name;
	};

	TileExportDialog() = default;
	virtual ~TileExportDialog();

	void Show(HWND owner, const Source& source);
	void Close();

	inline bool IsOpen() const { return m_Hwnd != NULL; }

	// Handles Tab, Enter and Esc. Returns true if the message was consumed.
	bool HandleDialogMessage(MSG* msg);

	LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	HWND m_Hwnd = NULL;
	HWND m_Owner = NULL;
	Source m_Source;

	static const int m_ClientWidth = 560;
	static const int m_ClientHeight = 548;
	static const int m_FooterHeight = 60;

	HFONT m_Font = NULL;
	HFONT m_BoldFont = NULL;
	HFONT m_MonoFont = NULL;
	void ReleaseFonts();

	void CreateControls();
	void PaintBackground(HDC hdc);

	// Options
	Layout SelectedLayout() const;
	int SelectedScale() const;

	// The image with the current options
	TileData::Image m_Image;
	void Rebuild();
	void DrawPreview(const DRAWITEMSTRUCT* item);

	// RGBA image as PNG
	std::vector<uint8_t> EncodePng() const;
	void Save();
	void CopyToClipboard();

	// Controls
	HWND m_LabelSource = NULL;
	std::array<HWND, 4> m_RadioLayout = {};
	HWND m_LabelPalette = NULL;
	HWND m_ComboPalette = NULL;
	HWND m_LabelScale = NULL;
	std::array<HWND, 4> m_RadioScale = {};
	HWND m_CheckGrid = NULL;
	HWND m_CheckTransparent = NULL;
	HWND m_LabelPreview = NULL;
	HWND m_LabelSize = NULL;
	HWND m_Preview = NULL;
	HWND m_ButtonCopy = NULL;
	HWND m_ButtonCancel = NULL;
	HWND m_ButtonSave = NULL;

	static const int m_ControlLayoutId = 100;
	static const int m_ControlPaletteId = 110;
	static const int m_ControlScaleId = 120;
	static const int m_ControlGridId = 130;
	static const int m_ControlTransparentId = 131;
	static const int m_ControlPreviewId = 140;
	static const int m_ControlCopyId = 150;
	static const int m_CopiedTimerId = 1;
};
