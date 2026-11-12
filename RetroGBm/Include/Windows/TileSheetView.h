#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// Draws one or more VRAM banks of tile data side by side, each as a 16 x 24 tile sheet with
// an address column, block separators and an optional grid. Scrolls when larger than the view.
class TileSheetView
{
public:
	struct Sheet
	{
		int bank = 0;

		// TileData::SheetWidth x TileData::SheetHeight pixels, 0x00RRGGBB, row by row
		std::vector<uint32_t> pixels;

		bool operator==(const Sheet& other) const = default;
	};

	// A tile in a bank, or none when bank is -1
	struct TileRef
	{
		int bank = -1;
		int tile = -1;

		bool IsValid() const { return bank >= 0 && tile >= 0; }
		bool operator==(const TileRef& other) const = default;
	};

	TileSheetView() = default;
	virtual ~TileSheetView();

	void Create(HWND parent, int id);
	void Destroy();

	inline HWND GetHwnd() const { return m_Hwnd; }

	// Repaints only when something changed, as the window calls this every frame
	void SetSheets(const std::vector<Sheet>& sheets);
	void SetZoom(int zoom);
	void SetGrid(bool grid);

	// Shown in the middle of the view when there are no sheets
	void SetEmptyText(const std::wstring& text);

	// The tile under the mouse, and the tile picked by clicking or the arrow keys
	inline TileRef GetHover() const { return m_Hover; }
	inline TileRef GetSelection() const { return m_Selection; }

	// Sets the selection without raising SelectionChanged
	void SetSelection(const TileRef& tile);

	// Scrolls so a tile is in view
	void EnsureVisible(const TileRef& tile);

	std::function<void()> HoverChanged;
	std::function<void()> SelectionChanged;

	// Ctrl+C while the view has focus
	std::function<void()> CopyRequested;

	LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	HWND m_Hwnd = NULL;

	HFONT m_Font = NULL;
	HFONT m_BoldFont = NULL;
	HFONT m_MonoFont = NULL;
	HFONT m_MonoBoldFont = NULL;

	std::vector<Sheet> m_Sheets;
	std::wstring m_EmptyText;
	int m_Zoom = 3;
	bool m_Grid = true;

	// Layout, in content coordinates before scrolling
	static const int m_PaddingX = 20;
	static const int m_PaddingY = 16;
	static const int m_HeaderHeight = 20;
	static const int m_HeaderGap = 8;
	static const int m_GutterWidth = 48;
	static const int m_GutterGap = 8;
	static const int m_BankGap = 32;

	int CellSize() const { return 8 * m_Zoom; }
	SIZE ContentSize() const;

	// Top-left of a sheet's pixels
	POINT SheetOrigin(size_t index) const;

	// Scrolling
	int m_ScrollX = 0;
	int m_ScrollY = 0;
	void UpdateScrollBars();
	void ScrollTo(int x, int y);
	void OnScroll(int bar, WORD request);

	// Back buffer, a DIB section the size of the client area so the grid can be blended into it
	HDC m_BackDC = NULL;
	HBITMAP m_BackBitmap = NULL;
	HGDIOBJ m_BackPrevious = NULL;
	uint32_t* m_BackPixels = nullptr;
	int m_BackWidth = 0;
	int m_BackHeight = 0;
	void EnsureBackBuffer(int width, int height);
	void ReleaseBackBuffer();

	// Hover and selection
	TileRef m_Hover;
	TileRef m_Selection;
	bool m_TrackingMouse = false;

	int SheetIndexOf(int bank) const;
	TileRef HitTest(int x, int y) const;
	RECT TileRect(size_t sheet_index, int tile) const;
	void UpdateHoverFromCursor();
	void SetHover(const TileRef& tile);
	void Select(const TileRef& tile);
	void MoveSelection(UINT key);

	void Paint(HDC hdc);
	void PaintSheet(size_t index);
	void BlendRect(int x, int y, int width, int height);
	void PaintHighlights(size_t index);
};
