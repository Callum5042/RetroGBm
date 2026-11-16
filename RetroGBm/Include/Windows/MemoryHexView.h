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
// A range of bytes can be selected with the mouse or keyboard; the caret is the end that moves.
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

	// Selection as block offsets, first to last inclusive. Empty when the block is.
	inline bool HasSelection() const { return !ReadSource().empty(); }
	size_t GetSelectionStart() const;
	size_t GetSelectionEnd() const;

	// Selects first to last and scrolls to it, without raising SelectionChanged. The caret
	// goes in the ASCII column when ascii is true, e.g. for text that was found.
	void SelectRange(size_t first, size_t last, bool ascii);

	// "FE00", or "01:4000" for a block with banks
	std::wstring FormatAddress(size_t offset) const;

	// Clicks, drags and keys that move the selection
	std::function<void()> SelectionChanged;

	// Typing over bytes at the caret: hex digits in the hex column, printable characters in the
	// ASCII column. Return false when the byte can't be written, e.g. ROM, and nothing moves.
	std::function<bool(size_t offset, uint8_t value)> WriteRequested;

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

	// Bank digits in the address column, enough for the block's highest bank
	int m_BankDigits = 2;
	void UpdateBankDigits(size_t size);

	// Selection: the anchor stays put while Shift+click, Shift+arrows or a drag move the caret
	size_t m_Anchor = 0;
	size_t m_Caret = 0;
	bool m_CaretInAscii = false;
	bool m_Dragging = false;

	// The byte under a point, clamped into the block. Sets whether it is in the ASCII column.
	size_t HitTest(int x, int y, bool* ascii) const;
	void MoveCaret(long long offset, bool extend);
	void Select(long long anchor_offset, long long caret_offset, bool notify = true);
	void EnsureVisible(size_t offset);
	void ClampSelection(size_t size);
	void OnKeyDown(WPARAM key);

	// In the hex column the first digit typed sets the high nibble, the second the low nibble,
	// then the caret moves on. Moving the caret starts again at the high nibble.
	bool m_LowNibble = false;
	void OnChar(wchar_t character);
	void AdvanceAfterWrite();

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
	void PaintSelection(HDC hdc, size_t row, int y, size_t size);
};
