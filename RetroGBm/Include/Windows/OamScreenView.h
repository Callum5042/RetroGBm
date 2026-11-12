#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <RetroGBm/Oam.h>

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// Draws the objects where they are on the 160 x 144 screen, each with an outline that can be clicked,
// with the lines over the 10-objects-per-line limit shaded and a chart of how many objects each line has
class OamScreenView
{
public:
	struct Object
	{
		// OAM index, 0-39
		int index = 0;

		// Top-left on screen, which can be off the top or left edge
		int x = 0;
		int y = 0;

		// 8 x height pixels, row by row: 0xFFRRGGBB where drawn, 0 where transparent, with flips applied
		std::vector<uint32_t> pixels;
		int height = 8;

		// Screen lines the limit drops this object from, which are left out
		std::vector<uint8_t> dropped;

		bool operator==(const Object& other) const = default;
	};

	struct Scene
	{
		// The objects that can be seen, back to front in the order the PPU's priority puts them
		std::vector<Object> objects;

		// How many objects cover each line, including those past the limit
		std::array<uint8_t, Oam::ScreenHeight> lines = {};

		// Last frame, ScreenWidth x ScreenHeight pixels as 0x00RRGGBB, drawn faded underneath. Empty for none.
		std::vector<uint32_t> background;

		// Shown under the screen, e.g. objects that use line slots but can't be seen
		std::wstring note;

		bool operator==(const Scene& other) const = default;
	};

	OamScreenView() = default;
	virtual ~OamScreenView();

	void Create(HWND parent, int id);
	void Destroy();

	inline HWND GetHwnd() const { return m_Hwnd; }

	// Repaints only when something changed, as the window calls this every frame
	void SetScene(const Scene& scene);

	void SetShowOutlines(bool show);
	void SetShowLineLimit(bool show);

	// Shown in the middle of the view when there is no game
	void SetEmptyText(const std::wstring& text);
	void SetEmpty(bool empty);

	// OAM index of the object under the mouse, and of the selected one, or -1 for none
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

	Scene m_Scene;
	bool m_Empty = true;
	std::wstring m_EmptyText;
	bool m_ShowOutlines = true;
	bool m_ShowLineLimit = true;

	// Layout
	static const int m_Padding = 16;
	static const int m_HeaderHeight = 20;
	static const int m_HeaderGap = 6;
	static const int m_ChartGap = 24;
	static const int m_BarWidth = 6;
	static const int m_ChartWidth = 16 * m_BarWidth;
	static const int m_LegendGap = 12;
	static const int m_LegendHeight = 18;

	// Largest whole scale (1-3) the screen fits at
	int Zoom() const;

	// Top-left of the screen's pixels
	POINT ScreenOrigin() const;

	// The object's outline in client coordinates
	RECT ObjectRect(const Object& object) const;

	// Back buffer, a DIB section the size of the client area so pixels can be blended into it
	HDC m_BackDC = NULL;
	HBITMAP m_BackBitmap = NULL;
	HGDIOBJ m_BackPrevious = NULL;
	uint32_t* m_BackPixels = nullptr;
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

	void Paint(HDC hdc);
	void PaintScreen();
	void PaintOutlines();
	void PaintChart();
	void PaintLegend();
	void SetPixel(int x, int y, uint32_t colour);
	void TintRect(int x, int y, int width, int height, uint32_t colour, int alpha);
};
