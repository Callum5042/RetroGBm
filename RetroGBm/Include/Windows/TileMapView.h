#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <RetroGBm/TileMap.h>

#include <cstdint>
#include <string>
#include <vector>

// Draws a whole 32 x 32 tile map (256 x 256 pixels) with a title, the address of each row, an
// optional grid and the parts of it on screen. Scrolls when larger than the view.
class TileMapView
{
public:
	// Areas of the map on screen, in map pixels. The rectangles can run off the right and bottom
	// edges, where they carry on from the other side.
	struct Overlay
	{
		// The screen, at SCX and SCY, when this is the background map
		TileMap::Rect screen;

		// The part of the screen hidden under the window, when this is the background map
		TileMap::Rect covered;

		// The part of the map shown as the window, when this is the window map
		TileMap::Rect window;

		bool operator==(const Overlay& other) const = default;
	};

	TileMapView() = default;
	virtual ~TileMapView();

	void Create(HWND parent, int id);
	void Destroy();

	inline HWND GetHwnd() const { return m_Hwnd; }

	// TileMap::Width x TileMap::Height pixels, 0x00RRGGBB, row by row, or empty to show the empty text.
	// Repaints only when something changed, as the window calls this every frame.
	void SetImage(const std::vector<uint32_t>& pixels);

	// The map's address, for the title and the row addresses
	void SetMapAddress(uint16_t address, const std::wstring& description);

	void SetZoom(int zoom);
	void SetGrid(bool grid);

	void SetOverlay(const Overlay& overlay);
	void SetShowOverlay(bool show);

	// Shown in the middle of the view when there is no image
	void SetEmptyText(const std::wstring& text);

	LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	HWND m_Hwnd = NULL;

	HFONT m_Font = NULL;
	HFONT m_BoldFont = NULL;
	HFONT m_MonoFont = NULL;
	HFONT m_MonoBoldFont = NULL;

	std::vector<uint32_t> m_Pixels;
	uint16_t m_MapAddress = 0x9800;
	std::wstring m_Description;
	std::wstring m_EmptyText;
	int m_Zoom = 2;
	bool m_Grid = false;
	Overlay m_Overlay;
	bool m_ShowOverlay = true;

	// Layout, in content coordinates before scrolling
	static const int m_PaddingX = 20;
	static const int m_PaddingY = 16;
	static const int m_HeaderHeight = 20;
	static const int m_HeaderGap = 8;
	static const int m_GutterWidth = 48;
	static const int m_GutterGap = 8;
	static const int m_LegendGap = 10;
	static const int m_LegendHeight = 18;

	int CellSize() const { return 8 * m_Zoom; }
	SIZE ContentSize() const;

	// Top-left of the map's pixels, after scrolling
	POINT MapOrigin() const;

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

	void Paint(HDC hdc);
	void PaintMap();
	void PaintOverlay();
	void PaintLegend();
	void BlendRect(int x, int y, int width, int height);
	void LightenRect(int x, int y, int width, int height);
};
