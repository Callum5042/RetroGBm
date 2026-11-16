#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <vector>

// Read-only hex dump of one block of memory: an address column, 16 bytes per row and their
// ASCII. Only the rows in view are read, so a block can be as large as a whole ROM.
class MemoryHexView
{
public:
	// How offsets in a block map to Game Boy addresses, for the address column
	struct Addressing
	{
		// Address of the first byte, e.g. $8000 for VRAM
		uint16_t base = 0;

		// Bytes per bank, or 0 when the block has no banks
		size_t bank_size = 0;

		// ROM and WRAM: bank 0 sits below the switchable window, which every other bank shares
		bool fixed_bank0 = false;
	};

	MemoryHexView() = default;
	virtual ~MemoryHexView();

	void Create(HWND parent, int id);
	void Destroy();

	inline HWND GetHwnd() const { return m_Hwnd; }

	// Shows a new block from the top. The source is asked for the block's bytes every time
	// the view reads them, as the memory behind it can move. An empty span shows the empty text.
	void SetBlock(const Addressing& addressing, std::function<std::span<const uint8_t>()> source);

	// Shown in the middle of the view when the block is empty
	void SetEmptyText(const std::wstring& text);

	// Called every frame: repaints only when a byte in view has changed
	void Refresh();

	LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	HWND m_Hwnd = NULL;

	HFONT m_Font = NULL;
	HFONT m_MonoFont = NULL;

	Addressing m_Addressing;
	std::function<std::span<const uint8_t>()> m_Source;
	std::wstring m_EmptyText;

	std::span<const uint8_t> ReadSource() const;

	// Layout, measured from the mono font
	static const int m_BytesPerRow = 16;
	int m_CharWidth = 8;
	int m_RowHeight = 16;
	int m_PaddingX = 16;
	int m_HeaderHeight = 24;

	int AddressWidth() const;
	int HexX(int column) const;
	int AsciiX(int column) const;
	std::wstring FormatAddress(size_t offset) const;

	// Bank digits in the address column, enough for the block's highest bank
	int m_BankDigits = 0;

	// Scrolling, in rows
	size_t m_TopRow = 0;
	size_t RowCount() const;
	int VisibleRows() const;
	void UpdateScrollBar();
	void ScrollTo(long long row);
	void OnScroll(WORD request);

	// The bytes last painted, so Refresh can tell when something in view changed
	std::vector<uint8_t> m_Painted;
	size_t m_PaintedTopRow = 0;
	size_t m_PaintedSize = 0;
	bool ViewChanged() const;

	// Back buffer the size of the client area
	HDC m_BackDC = NULL;
	HBITMAP m_BackBitmap = NULL;
	HGDIOBJ m_BackPrevious = NULL;
	int m_BackWidth = 0;
	int m_BackHeight = 0;
	void EnsureBackBuffer(HDC hdc, int width, int height);
	void ReleaseBackBuffer();

	void MeasureFont();
	void Paint(HDC hdc);
};
