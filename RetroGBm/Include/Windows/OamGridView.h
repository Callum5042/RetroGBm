#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// Draws the objects in OAM as a grid of cells, each with its index, its image on a checkerboard and a caption.
// Fits as many columns as the width allows, and scrolls vertically when they don't all fit.
class OamGridView
{
public:
	struct Cell
	{
		// OAM index, 0-39
		int index = 0;

		// 8 x height pixels, row by row: 0xFFRRGGBB where drawn, 0 where transparent
		std::vector<uint32_t> pixels;
		int height = 8;

		// Shown under the image, e.g. "$0E · P4"
		std::wstring caption;

		// Not on screen: faded
		bool hidden = false;

		// Skipped on some lines by the 10-objects-per-line limit: marked with a warning triangle
		bool dropped = false;

		bool operator==(const Cell& other) const = default;
	};

	OamGridView() = default;
	virtual ~OamGridView();

	void Create(HWND parent, int id);
	void Destroy();

	inline HWND GetHwnd() const { return m_Hwnd; }

	// Repaints only when something changed, as the window calls this every frame
	void SetCells(const std::vector<Cell>& cells);

	void SetZoom(int zoom);

	// Shown in the middle of the view when there are no cells
	void SetEmptyText(const std::wstring& text);

	// OAM index of the object under the mouse, and of the one picked by clicking or the arrow keys, or -1 for none
	inline int GetHover() const { return m_Hover; }
	inline int GetSelection() const { return m_Selection; }

	// Selects an object without raising SelectionChanged, to follow a selection made elsewhere
	void SetSelection(int index);

	std::function<void()> HoverChanged;
	std::function<void()> SelectionChanged;

	// Double-click or Enter on the selected object
	std::function<void()> ActivateRequested;

	LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	HWND m_Hwnd = NULL;

	HFONT m_Font = NULL;
	HFONT m_BoldFont = NULL;
	HFONT m_MonoFont = NULL;

	std::vector<Cell> m_Cells;
	std::wstring m_EmptyText;
	int m_Zoom = 4;

	// Layout, in content coordinates before scrolling
	static const int m_PaddingX = 16;
	static const int m_PaddingTop = 10;
	static const int m_PaddingBottom = 12;
	static const int m_HeaderHeight = 20;
	static const int m_HeaderGap = 8;
	static const int m_CellGap = 8;
	static const int m_CellPadding = 5;
	static const int m_TextHeight = 14;
	static const int m_TextGap = 4;

	int ObjectHeight() const;
	int CellWidth() const;
	int CellHeight() const;
	int Columns() const;
	int ContentHeight() const;

	// Position of the cell in m_Cells, after scrolling
	RECT CellRect(int position) const;

	// Position in m_Cells of an OAM index, or -1 when it isn't shown
	int PositionOf(int index) const;

	// Scrolling
	int m_ScrollY = 0;
	void UpdateScrollBar();
	void ScrollTo(int y);
	void OnScroll(WORD request);

	// Back buffer the size of the client area
	HDC m_BackDC = NULL;
	HBITMAP m_BackBitmap = NULL;
	HGDIOBJ m_BackPrevious = NULL;
	int m_BackWidth = 0;
	int m_BackHeight = 0;
	void EnsureBackBuffer(int width, int height);
	void ReleaseBackBuffer();

	// Hover and selection, as OAM indices
	int m_Hover = -1;
	int m_Selection = -1;
	bool m_TrackingMouse = false;

	int HitTest(int x, int y) const;
	void UpdateHoverFromCursor();
	void SetHover(int index);
	void Select(int index);
	void MoveSelection(UINT key);
	void EnsureVisible(int index);

	void Paint(HDC hdc);
	void PaintHeader();
	void PaintCell(const Cell& cell, const RECT& rect);
};
