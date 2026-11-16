#include "Windows/MemoryHexView.h"

#include <windowsx.h>

#include <algorithm>
#include <format>

namespace
{
	const wchar_t* ClassName = L"RetroGBmMemoryHexView";

	const COLORREF BackgroundColour = RGB(255, 255, 255);
	const COLORREF TextColour = RGB(26, 26, 26);
	const COLORREF ZeroColour = RGB(176, 176, 176);
	const COLORREF AddressColour = RGB(110, 110, 110);
	const COLORREF HeaderColour = RGB(92, 92, 92);
	const COLORREF DividerColour = RGB(223, 223, 223);
	const COLORREF BankLineColour = RGB(236, 236, 236);
	const COLORREF SelectionColour = RGB(204, 228, 247);
	const COLORREF InactiveSelectionColour = RGB(229, 229, 229);
	const COLORREF CaretColour = RGB(0, 95, 184);

	static LRESULT CALLBACK HexViewWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		MemoryHexView* view = nullptr;
		if (msg == WM_NCCREATE)
		{
			CREATESTRUCT* create = reinterpret_cast<CREATESTRUCT*>(lParam);
			view = reinterpret_cast<MemoryHexView*>(create->lpCreateParams);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(view));
		}
		else
		{
			view = reinterpret_cast<MemoryHexView*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		}

		if (view == nullptr)
		{
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}

		return view->HandleMessage(hwnd, msg, wParam, lParam);
	}

	void FillColour(HDC hdc, int x, int y, int width, int height, COLORREF colour)
	{
		RECT rect = { x, y, x + width, y + height };
		SetDCBrushColor(hdc, colour);
		FillRect(hdc, &rect, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
	}

	int HexDigit(wchar_t c)
	{
		if (c >= L'0' && c <= L'9') return c - L'0';
		if (c >= L'a' && c <= L'f') return c - L'a' + 10;
		if (c >= L'A' && c <= L'F') return c - L'A' + 10;
		return -1;
	}

	void DrawTextAt(HDC hdc, int x, int y, const std::wstring& text, COLORREF colour)
	{
		SetTextColor(hdc, colour);
		ExtTextOutW(hdc, x, y, 0, nullptr, text.c_str(), static_cast<UINT>(text.size()), nullptr);
	}
}

MemoryHexView::~MemoryHexView()
{
	this->Destroy();
}

void MemoryHexView::Create(HWND parent, int id)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);

	WNDCLASS wc = {};
	wc.lpfnWndProc = HexViewWndProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.lpszClassName = ClassName;
	RegisterClass(&wc);

	// Fonts: the system message font for the empty text, and Consolas for the bytes
	{
		NONCLIENTMETRICS metrics = { sizeof(NONCLIENTMETRICS) };
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0);
		m_Font = CreateFontIndirect(&metrics.lfMessageFont);

		HDC hdc = GetDC(NULL);
		LOGFONT mono = {};
		mono.lfHeight = -MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72);
		mono.lfWeight = FW_NORMAL;
		mono.lfQuality = CLEARTYPE_QUALITY;
		mono.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
		wcscpy_s(mono.lfFaceName, L"Consolas");
		m_MonoFont = CreateFontIndirect(&mono);
		ReleaseDC(NULL, hdc);
	}

	MeasureFont();

	m_Hwnd = CreateWindowEx(0, ClassName, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | WS_CLIPSIBLINGS,
		0, 0, 0, 0, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), hInstance, this);
	if (m_Hwnd == NULL)
	{
		throw std::exception("CreateWindow Failed");
	}

	UpdateScrollBar();
}

void MemoryHexView::Destroy()
{
	if (m_Hwnd != NULL)
	{
		DestroyWindow(m_Hwnd);
		m_Hwnd = NULL;
	}

	UnregisterClass(ClassName, GetModuleHandle(NULL));
	ReleaseBackBuffer();

	for (HFONT* font : { &m_Font, &m_MonoFont })
	{
		if (*font != NULL)
		{
			DeleteObject(*font);
			*font = NULL;
		}
	}
}

void MemoryHexView::MeasureFont()
{
	HDC hdc = GetDC(NULL);
	HGDIOBJ previous = SelectObject(hdc, m_MonoFont);

	TEXTMETRIC metrics = {};
	GetTextMetrics(hdc, &metrics);

	SelectObject(hdc, previous);
	ReleaseDC(NULL, hdc);

	m_CharWidth = metrics.tmAveCharWidth;
	m_RowHeight = metrics.tmHeight + 4;
	m_HeaderHeight = m_RowHeight + 8;
	m_PaddingX = m_CharWidth * 2;
}

void MemoryHexView::SetBlock(const Addressing& addressing, std::function<std::span<const uint8_t>()> source)
{
	m_Addressing = addressing;
	m_Source = std::move(source);
	m_TopRow = 0;
	m_Painted.clear();

	// A new block starts with its first byte selected
	m_Anchor = 0;
	m_Caret = 0;
	m_CaretInAscii = false;
	m_LowNibble = false;
	UpdateBankDigits(ReadSource().size());

	UpdateScrollBar();
	InvalidateRect(m_Hwnd, NULL, FALSE);
}

size_t MemoryHexView::GetSelectionStart() const
{
	return (std::min)(m_Anchor, m_Caret);
}

size_t MemoryHexView::GetSelectionEnd() const
{
	return (std::max)(m_Anchor, m_Caret);
}

void MemoryHexView::ClampSelection(size_t size)
{
	const size_t last = size == 0 ? 0 : size - 1;
	m_Anchor = (std::min)(m_Anchor, last);
	m_Caret = (std::min)(m_Caret, last);
}

void MemoryHexView::UpdateBankDigits(size_t size)
{
	// At least two, more for a ROM with over 256 banks
	m_BankDigits = 2;
	if (m_Addressing.bank_size != 0 && size != 0)
	{
		for (size_t bank = (size - 1) / m_Addressing.bank_size; bank > 0xFF; bank >>= 4)
		{
			++m_BankDigits;
		}
	}
}

size_t MemoryHexView::HitTest(int x, int y, bool* ascii) const
{
	const size_t size = ReadSource().size();
	if (size == 0)
	{
		return 0;
	}

	// Above the first row or below the last counts as the row just outside, so a drag can extend past the view
	const long long row = static_cast<long long>(m_TopRow) + (y < m_HeaderHeight ? -1 : (y - m_HeaderHeight) / m_RowHeight);

	// Halfway between the hex and ASCII columns splits them
	int column = 0;
	*ascii = x >= AsciiX(0) - m_CharWidth * 2;
	if (*ascii)
	{
		column = (x - AsciiX(0)) / m_CharWidth;
	}
	else
	{
		// Each byte owns its two digits and half the space either side
		column = m_BytesPerRow - 1;
		for (int c = 0; c < m_BytesPerRow - 1; ++c)
		{
			if (x < (HexX(c) + HexX(c + 1) + m_CharWidth * 2) / 2)
			{
				column = c;
				break;
			}
		}
	}

	column = std::clamp(column, 0, m_BytesPerRow - 1);
	const long long offset = row * m_BytesPerRow + column;
	return static_cast<size_t>(std::clamp(offset, 0LL, static_cast<long long>(size) - 1));
}

void MemoryHexView::EnsureVisible(size_t offset)
{
	const size_t row = offset / m_BytesPerRow;
	const size_t visible = static_cast<size_t>(VisibleRows());

	if (row < m_TopRow)
	{
		ScrollTo(static_cast<long long>(row));
	}
	else if (row >= m_TopRow + visible)
	{
		ScrollTo(static_cast<long long>(row - visible + 1));
	}
}

void MemoryHexView::MoveCaret(long long offset, bool extend)
{
	Select(extend ? static_cast<long long>(m_Anchor) : offset, offset);
}

void MemoryHexView::SelectRange(size_t first, size_t last, bool ascii)
{
	if (m_CaretInAscii != ascii)
	{
		m_CaretInAscii = ascii;
		InvalidateRect(m_Hwnd, NULL, FALSE);
	}

	// Off screen: bring it a third of the way down, so the rows before it show too
	const size_t row = first / m_BytesPerRow;
	const size_t visible = static_cast<size_t>(VisibleRows());
	if (row < m_TopRow || row >= m_TopRow + visible)
	{
		ScrollTo(static_cast<long long>(row) - static_cast<long long>(visible / 3));
	}

	Select(static_cast<long long>(first), static_cast<long long>(last), false);

	// A long match can run past the bottom: show where it starts
	EnsureVisible(first);
}

void MemoryHexView::Select(long long anchor_offset, long long caret_offset, bool notify)
{
	const size_t size = ReadSource().size();
	if (size == 0)
	{
		return;
	}

	const long long last = static_cast<long long>(size) - 1;
	const size_t caret = static_cast<size_t>(std::clamp(caret_offset, 0LL, last));
	const size_t anchor = static_cast<size_t>(std::clamp(anchor_offset, 0LL, last));
	EnsureVisible(caret);

	if (caret == m_Caret && anchor == m_Anchor)
	{
		return;
	}

	m_Caret = caret;
	m_Anchor = anchor;
	m_LowNibble = false;
	InvalidateRect(m_Hwnd, NULL, FALSE);

	if (notify && SelectionChanged)
	{
		SelectionChanged();
	}
}

void MemoryHexView::OnChar(wchar_t character)
{
	const std::span<const uint8_t> data = ReadSource();
	if (data.empty() || !WriteRequested || m_Caret >= data.size())
	{
		return;
	}

	if (m_CaretInAscii)
	{
		if (character < 0x20 || character > 0x7E)
		{
			return;
		}

		if (WriteRequested(m_Caret, static_cast<uint8_t>(character)))
		{
			AdvanceAfterWrite();
		}

		return;
	}

	const int digit = HexDigit(character);
	if (digit < 0)
	{
		return;
	}

	const uint8_t old_value = data[m_Caret];
	const uint8_t value = m_LowNibble
		? static_cast<uint8_t>((old_value & 0xF0) | digit)
		: static_cast<uint8_t>((digit << 4) | (old_value & 0x0F));

	if (!WriteRequested(m_Caret, value))
	{
		return;
	}

	if (m_LowNibble)
	{
		AdvanceAfterWrite();
		return;
	}

	// Typing works on the caret's byte alone, then waits for the low nibble
	Select(static_cast<long long>(m_Caret), static_cast<long long>(m_Caret));
	m_LowNibble = true;
	InvalidateRect(m_Hwnd, NULL, FALSE);
}

void MemoryHexView::AdvanceAfterWrite()
{
	// On to the next byte, or stay on the last one
	const long long next = static_cast<long long>(m_Caret) + 1;
	m_LowNibble = false;
	Select(next, next);
	InvalidateRect(m_Hwnd, NULL, FALSE);
}

void MemoryHexView::OnKeyDown(WPARAM key)
{
	const bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
	const bool control = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
	const long long caret = static_cast<long long>(m_Caret);
	const long long page = static_cast<long long>(VisibleRows()) * m_BytesPerRow;
	const long long row_start = caret - caret % m_BytesPerRow;

	switch (key)
	{
		case VK_LEFT: MoveCaret(caret - 1, shift); break;
		case VK_RIGHT: MoveCaret(caret + 1, shift); break;
		case VK_UP: MoveCaret(caret - m_BytesPerRow, shift); break;
		case VK_DOWN: MoveCaret(caret + m_BytesPerRow, shift); break;
		case VK_PRIOR: MoveCaret(caret - page, shift); break;
		case VK_NEXT: MoveCaret(caret + page, shift); break;

		// Start and end of the row, or of the block with Ctrl
		case VK_HOME: MoveCaret(control ? 0 : row_start, shift); break;
		case VK_END: MoveCaret(control ? static_cast<long long>(ReadSource().size()) : row_start + m_BytesPerRow - 1, shift); break;

		case 'A':
			if (control)
			{
				// Select the whole block, with the caret at the end
				Select(0, static_cast<long long>(ReadSource().size()));
			}

			break;
	}
}

void MemoryHexView::SetEmptyText(const std::wstring& text)
{
	if (text == m_EmptyText)
	{
		return;
	}

	m_EmptyText = text;
	InvalidateRect(m_Hwnd, NULL, FALSE);
}

std::span<const uint8_t> MemoryHexView::ReadSource() const
{
	return m_Source ? m_Source() : std::span<const uint8_t>();
}

void MemoryHexView::Refresh()
{
	if (m_Hwnd == NULL)
	{
		return;
	}

	// The block can grow or shrink, e.g. a new game with a different amount of cartridge RAM
	const size_t size = ReadSource().size();
	if (size != m_PaintedSize)
	{
		ClampSelection(size);
		UpdateBankDigits(size);
		UpdateScrollBar();
		InvalidateRect(m_Hwnd, NULL, FALSE);
		return;
	}

	if (ViewChanged())
	{
		InvalidateRect(m_Hwnd, NULL, FALSE);
	}
}

bool MemoryHexView::ViewChanged() const
{
	const std::span<const uint8_t> data = ReadSource();
	if (data.size() != m_PaintedSize || m_TopRow != m_PaintedTopRow)
	{
		return true;
	}

	const size_t first = (std::min)(m_TopRow * m_BytesPerRow, data.size());
	const size_t count = (std::min)(m_Painted.size(), data.size() - first);
	return count != m_Painted.size() || !std::equal(m_Painted.begin(), m_Painted.end(), data.begin() + first);
}

size_t MemoryHexView::RowCount() const
{
	return (ReadSource().size() + m_BytesPerRow - 1) / m_BytesPerRow;
}

int MemoryHexView::VisibleRows() const
{
	RECT client = {};
	GetClientRect(m_Hwnd, &client);
	return (std::max)(1, static_cast<int>((client.bottom - m_HeaderHeight) / m_RowHeight));
}

void MemoryHexView::UpdateScrollBar()
{
	if (m_Hwnd == NULL)
	{
		return;
	}

	const size_t rows = RowCount();
	const size_t visible = static_cast<size_t>(VisibleRows());
	const size_t max_top = rows > visible ? rows - visible : 0;
	m_TopRow = (std::min)(m_TopRow, max_top);

	// Kept visible, so the hex columns do not shift as blocks change
	SCROLLINFO info = { sizeof(SCROLLINFO) };
	info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS | SIF_DISABLENOSCROLL;
	info.nMin = 0;
	info.nMax = rows == 0 ? 0 : static_cast<int>(rows - 1);
	info.nPage = static_cast<UINT>(visible);
	info.nPos = static_cast<int>(m_TopRow);
	SetScrollInfo(m_Hwnd, SB_VERT, &info, TRUE);
}

void MemoryHexView::ScrollTo(long long row)
{
	const long long rows = static_cast<long long>(RowCount());
	const long long max_top = (std::max)(0LL, rows - VisibleRows());
	const size_t top = static_cast<size_t>(std::clamp(row, 0LL, max_top));
	if (top == m_TopRow)
	{
		return;
	}

	m_TopRow = top;
	SetScrollPos(m_Hwnd, SB_VERT, static_cast<int>(m_TopRow), TRUE);
	InvalidateRect(m_Hwnd, NULL, FALSE);
}

void MemoryHexView::OnScroll(WORD request)
{
	const long long top = static_cast<long long>(m_TopRow);
	const long long page = VisibleRows();

	switch (request)
	{
		case SB_LINEUP: ScrollTo(top - 1); break;
		case SB_LINEDOWN: ScrollTo(top + 1); break;
		case SB_PAGEUP: ScrollTo(top - page); break;
		case SB_PAGEDOWN: ScrollTo(top + page); break;
		case SB_TOP: ScrollTo(0); break;
		case SB_BOTTOM: ScrollTo(static_cast<long long>(RowCount())); break;

		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
		{
			// The 32-bit track position, as a ROM can have more rows than the message carries
			SCROLLINFO info = { sizeof(SCROLLINFO), SIF_TRACKPOS };
			GetScrollInfo(m_Hwnd, SB_VERT, &info);
			ScrollTo(info.nTrackPos);
			break;
		}
	}
}

int MemoryHexView::AddressWidth() const
{
	// "FE00", or "01:4000" when the block has banks
	const int chars = 4 + (m_Addressing.bank_size != 0 ? m_BankDigits + 1 : 0);
	return chars * m_CharWidth;
}

int MemoryHexView::HexX(int column) const
{
	// Three characters a byte, and an extra space after the first eight
	return m_PaddingX + AddressWidth() + m_CharWidth * 3 + column * m_CharWidth * 3 + (column >= 8 ? m_CharWidth : 0);
}

int MemoryHexView::AsciiX(int column) const
{
	return HexX(m_BytesPerRow - 1) + m_CharWidth * 5 + column * m_CharWidth;
}

std::wstring MemoryHexView::FormatAddress(size_t offset) const
{
	const size_t bank_size = m_Addressing.bank_size;
	if (bank_size == 0)
	{
		return std::format(L"{:04X}", m_Addressing.base + offset);
	}

	const size_t bank = offset / bank_size;
	const size_t window = (m_Addressing.fixed_bank0 && bank > 0) ? bank_size : 0;
	const size_t address = m_Addressing.base + window + (offset % bank_size);
	return std::format(L"{:0{}X}:{:04X}", bank, m_BankDigits, address);
}

void MemoryHexView::EnsureBackBuffer(HDC hdc, int width, int height)
{
	if (m_BackDC != NULL && width == m_BackWidth && height == m_BackHeight)
	{
		return;
	}

	ReleaseBackBuffer();

	m_BackDC = CreateCompatibleDC(hdc);
	m_BackBitmap = CreateCompatibleBitmap(hdc, width, height);
	m_BackPrevious = SelectObject(m_BackDC, m_BackBitmap);
	m_BackWidth = width;
	m_BackHeight = height;
}

void MemoryHexView::ReleaseBackBuffer()
{
	if (m_BackDC != NULL)
	{
		SelectObject(m_BackDC, m_BackPrevious);
		DeleteDC(m_BackDC);
		m_BackDC = NULL;
	}

	if (m_BackBitmap != NULL)
	{
		DeleteObject(m_BackBitmap);
		m_BackBitmap = NULL;
	}

	m_BackPrevious = NULL;
	m_BackWidth = 0;
	m_BackHeight = 0;
}

void MemoryHexView::Paint(HDC hdc)
{
	RECT client;
	GetClientRect(m_Hwnd, &client);
	const int width = (std::max)(1, static_cast<int>(client.right));
	const int height = (std::max)(1, static_cast<int>(client.bottom));

	EnsureBackBuffer(hdc, width, height);
	HDC dc = m_BackDC;

	FillColour(dc, 0, 0, width, height, BackgroundColour);
	SetBkMode(dc, TRANSPARENT);

	const std::span<const uint8_t> data = ReadSource();
	m_PaintedSize = data.size();
	m_PaintedTopRow = m_TopRow;

	if (data.empty())
	{
		m_Painted.clear();

		HGDIOBJ previous = SelectObject(dc, m_Font);
		SetTextColor(dc, HeaderColour);
		DrawTextW(dc, m_EmptyText.c_str(), -1, &client, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		SelectObject(dc, previous);

		BitBlt(hdc, 0, 0, width, height, dc, 0, 0, SRCCOPY);
		return;
	}

	ClampSelection(data.size());
	UpdateBankDigits(data.size());

	HGDIOBJ previous = SelectObject(dc, m_MonoFont);
	const int text_offset = 2;

	// Column headings and the line under them
	for (int column = 0; column < m_BytesPerRow; ++column)
	{
		DrawTextAt(dc, HexX(column), (m_HeaderHeight - m_RowHeight) / 2 + text_offset, std::format(L"{:02X}", column), HeaderColour);
	}

	FillColour(dc, 0, m_HeaderHeight - 1, width, 1, DividerColour);

	// Rows, including a partly visible one at the bottom
	const size_t rows = RowCount();
	const size_t last_row = (std::min)(rows, m_TopRow + static_cast<size_t>(VisibleRows()) + 1);

	for (size_t row = m_TopRow; row < last_row; ++row)
	{
		const size_t offset = row * m_BytesPerRow;
		const int y = m_HeaderHeight + static_cast<int>(row - m_TopRow) * m_RowHeight;

		// A faint line where a new bank starts
		if (m_Addressing.bank_size != 0 && offset != 0 && offset % m_Addressing.bank_size == 0)
		{
			FillColour(dc, 0, y, width, 1, BankLineColour);
		}

		PaintSelection(dc, row, y, data.size());

		DrawTextAt(dc, m_PaddingX, y + text_offset, FormatAddress(offset), AddressColour);

		for (int column = 0; column < m_BytesPerRow; ++column)
		{
			const size_t index = offset + column;
			if (index >= data.size())
			{
				break;
			}

			// Zero bytes are dimmed so the rest stand out
			const uint8_t value = data[index];
			DrawTextAt(dc, HexX(column), y + text_offset, std::format(L"{:02X}", value), value == 0 ? ZeroColour : TextColour);

			const bool printable = value >= 0x20 && value < 0x7F;
			const wchar_t character = printable ? static_cast<wchar_t>(value) : L'.';
			DrawTextAt(dc, AsciiX(column), y + text_offset, std::wstring(1, character), printable ? TextColour : ZeroColour);
		}
	}

	SelectObject(dc, previous);

	// Remember what is in view, for Refresh
	const size_t first = (std::min)(m_TopRow * m_BytesPerRow, data.size());
	const size_t last = (std::min)(last_row * m_BytesPerRow, data.size());
	m_Painted.assign(data.begin() + first, data.begin() + last);

	BitBlt(hdc, 0, 0, width, height, dc, 0, 0, SRCCOPY);
}

void MemoryHexView::PaintSelection(HDC hdc, size_t row, int y, size_t size)
{
	const size_t row_first = row * m_BytesPerRow;
	const size_t row_last = (std::min)(row_first + m_BytesPerRow, size) - 1;
	const size_t first = (std::max)(GetSelectionStart(), row_first);
	const size_t last = (std::min)(GetSelectionEnd(), row_last);

	// Greyed out while another control has focus
	const bool focused = GetFocus() == m_Hwnd;
	const COLORREF colour = focused ? SelectionColour : InactiveSelectionColour;

	// Each hex byte fills its three character cell, so a run of bytes joins up
	for (size_t index = first; index <= last && first <= last; ++index)
	{
		const int column = static_cast<int>(index - row_first);
		FillColour(hdc, HexX(column) - m_CharWidth / 2, y, m_CharWidth * 3, m_RowHeight, colour);
		FillColour(hdc, AsciiX(column), y, m_CharWidth, m_RowHeight, colour);
	}

	// Outline the caret in the column it was placed in
	if (focused && m_Caret >= row_first && m_Caret <= row_last)
	{
		const int column = static_cast<int>(m_Caret - row_first);
		// Halfway through typing a byte, just the low digit
		RECT caret = m_CaretInAscii
			? RECT{ AsciiX(column), y, AsciiX(column) + m_CharWidth, y + m_RowHeight }
			: m_LowNibble
				? RECT{ HexX(column) + m_CharWidth, y, HexX(column) + m_CharWidth * 2 + m_CharWidth / 2, y + m_RowHeight }
				: RECT{ HexX(column) - m_CharWidth / 2, y, HexX(column) + m_CharWidth * 2 + m_CharWidth / 2, y + m_RowHeight };

		SetDCBrushColor(hdc, CaretColour);
		FrameRect(hdc, &caret, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
	}
}

LRESULT MemoryHexView::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_ERASEBKGND:
			return 1;

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			Paint(hdc);
			EndPaint(hwnd, &ps);
			return 0;
		}

		case WM_SIZE:
			UpdateScrollBar();
			InvalidateRect(hwnd, NULL, FALSE);
			return 0;

		case WM_VSCROLL:
			OnScroll(LOWORD(wParam));
			return 0;

		case WM_MOUSEWHEEL:
		{
			// Three rows a notch
			const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
			ScrollTo(static_cast<long long>(m_TopRow) - delta * 3 / WHEEL_DELTA);
			return 0;
		}

		case WM_LBUTTONDOWN:
		{
			SetFocus(hwnd);

			// Click selects a byte, Shift+click extends from the anchor, and dragging extends as it goes
			bool ascii = false;
			const size_t offset = HitTest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), &ascii);
			const bool extend = (wParam & MK_SHIFT) != 0;
			if (m_CaretInAscii != ascii)
			{
				m_CaretInAscii = ascii;
				InvalidateRect(hwnd, NULL, FALSE);
			}

			MoveCaret(static_cast<long long>(offset), extend);

			m_Dragging = true;
			SetCapture(hwnd);
			return 0;
		}

		case WM_MOUSEMOVE:
			if (m_Dragging)
			{
				// Past the top or bottom scrolls a row at a time
				RECT client;
				GetClientRect(hwnd, &client);
				const int y = GET_Y_LPARAM(lParam);
				if (y < m_HeaderHeight)
				{
					ScrollTo(static_cast<long long>(m_TopRow) - 1);
				}
				else if (y >= client.bottom)
				{
					ScrollTo(static_cast<long long>(m_TopRow) + 1);
				}

				bool ascii = false;
				MoveCaret(static_cast<long long>(HitTest(GET_X_LPARAM(lParam), y, &ascii)), true);
			}

			return 0;

		case WM_LBUTTONUP:
			if (m_Dragging)
			{
				ReleaseCapture();
			}

			return 0;

		case WM_CAPTURECHANGED:
			m_Dragging = false;
			return 0;

		case WM_SETFOCUS:
		case WM_KILLFOCUS:
			// The selection colour and caret depend on focus
			InvalidateRect(hwnd, NULL, FALSE);
			return 0;

		case WM_GETDLGCODE:
			// Keep the arrow keys rather than letting the dialog manager move focus with them,
			// and characters for typing over bytes
			return DLGC_WANTARROWS | DLGC_WANTCHARS;

		case WM_KEYDOWN:
			OnKeyDown(wParam);
			return 0;

		case WM_CHAR:
			OnChar(static_cast<wchar_t>(wParam));
			return 0;

		case WM_DESTROY:
			m_Hwnd = NULL;
			m_Dragging = false;
			return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}
