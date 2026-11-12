#include "Windows/OamGridView.h"

#include <RetroGBm/Oam.h>

#include <windowsx.h>

#include <algorithm>
#include <format>

namespace
{
	const wchar_t* ClassName = L"RetroGBmOamGridView";

	const COLORREF BackgroundColour = RGB(243, 243, 243);
	const COLORREF CellColour = RGB(255, 255, 255);
	const COLORREF HoverCellColour = RGB(242, 248, 253);
	const COLORREF SelectedCellColour = RGB(229, 241, 251);
	const COLORREF BorderColour = RGB(214, 214, 214);
	const COLORREF TextColour = RGB(26, 26, 26);
	const COLORREF SecondaryTextColour = RGB(92, 92, 92);
	const COLORREF SelectedTextColour = RGB(0, 78, 146);
	const COLORREF SelectionColour = RGB(0, 95, 184);
	const COLORREF WarningColour = RGB(197, 90, 0);
	const COLORREF CheckerLight = RGB(255, 255, 255);
	const COLORREF CheckerDark = RGB(230, 230, 230);

	static LRESULT CALLBACK GridWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		OamGridView* view = nullptr;
		if (msg == WM_NCCREATE)
		{
			CREATESTRUCT* create = reinterpret_cast<CREATESTRUCT*>(lParam);
			view = reinterpret_cast<OamGridView*>(create->lpCreateParams);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(view));
		}
		else
		{
			view = reinterpret_cast<OamGridView*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
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

	void FrameColour(HDC hdc, RECT rect, int inset, COLORREF colour)
	{
		InflateRect(&rect, -inset, -inset);
		SetDCBrushColor(hdc, colour);
		FrameRect(hdc, &rect, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
	}

	// 0x00RRGGBB to a COLORREF, faded most of the way to white for hidden objects
	COLORREF ToColorRef(uint32_t colour, bool faded)
	{
		int r = (colour >> 16) & 0xFF;
		int g = (colour >> 8) & 0xFF;
		int b = colour & 0xFF;
		if (faded)
		{
			r += (255 - r) * 3 / 5;
			g += (255 - g) * 3 / 5;
			b += (255 - b) * 3 / 5;
		}

		return RGB(r, g, b);
	}
}

void OamGridView::Create(HWND parent, int id)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);

	WNDCLASS wc = {};
	wc.style = CS_DBLCLKS;
	wc.lpfnWndProc = GridWndProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.lpszClassName = ClassName;
	RegisterClass(&wc);

	// Fonts: the system message font, and Consolas for indices and tiles
	{
		NONCLIENTMETRICS metrics = { sizeof(NONCLIENTMETRICS) };
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0);
		m_Font = CreateFontIndirect(&metrics.lfMessageFont);

		LOGFONT bold = metrics.lfMessageFont;
		bold.lfWeight = FW_SEMIBOLD;
		m_BoldFont = CreateFontIndirect(&bold);

		HDC hdc = GetDC(NULL);
		LOGFONT mono = {};
		mono.lfHeight = -MulDiv(8, GetDeviceCaps(hdc, LOGPIXELSY), 72);
		mono.lfWeight = FW_NORMAL;
		mono.lfQuality = CLEARTYPE_QUALITY;
		mono.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
		wcscpy_s(mono.lfFaceName, L"Consolas");
		m_MonoFont = CreateFontIndirect(&mono);
		ReleaseDC(NULL, hdc);
	}

	m_Hwnd = CreateWindowEx(0, ClassName, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | WS_CLIPSIBLINGS,
		0, 0, 0, 0, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), hInstance, this);
	if (m_Hwnd == NULL)
	{
		throw std::exception("CreateWindow Failed");
	}
}

OamGridView::~OamGridView()
{
	this->Destroy();
}

void OamGridView::Destroy()
{
	if (m_Hwnd != NULL)
	{
		DestroyWindow(m_Hwnd);
		m_Hwnd = NULL;
	}

	UnregisterClass(ClassName, GetModuleHandle(NULL));
	ReleaseBackBuffer();

	for (HFONT* font : { &m_Font, &m_BoldFont, &m_MonoFont })
	{
		if (*font != NULL)
		{
			DeleteObject(*font);
			*font = NULL;
		}
	}
}

void OamGridView::SetCells(const std::vector<Cell>& cells)
{
	if (cells == m_Cells)
	{
		return;
	}

	const bool layout_changed = cells.size() != m_Cells.size() || ObjectHeight() != (cells.empty() ? 16 : cells.front().height);
	m_Cells = cells;

	if (layout_changed)
	{
		UpdateScrollBar();
	}

	InvalidateRect(m_Hwnd, NULL, FALSE);

	// The hovered object may have moved under the mouse, or gone
	if (m_Hover >= 0 && PositionOf(m_Hover) < 0)
	{
		SetHover(-1);
	}
	else
	{
		UpdateHoverFromCursor();
	}
}

void OamGridView::SetZoom(int zoom)
{
	if (zoom == m_Zoom)
	{
		return;
	}

	m_Zoom = zoom;
	UpdateScrollBar();
	EnsureVisible(m_Selection);
	InvalidateRect(m_Hwnd, NULL, FALSE);
}

void OamGridView::SetEmptyText(const std::wstring& text)
{
	if (text != m_EmptyText)
	{
		m_EmptyText = text;
		InvalidateRect(m_Hwnd, NULL, FALSE);
	}
}

int OamGridView::ObjectHeight() const
{
	return m_Cells.empty() ? 16 : m_Cells.front().height;
}

int OamGridView::CellWidth() const
{
	return (std::max)(8 * m_Zoom + 24, 80);
}

int OamGridView::CellHeight() const
{
	return m_CellPadding * 2 + m_TextHeight * 2 + m_TextGap * 2 + ObjectHeight() * m_Zoom;
}

int OamGridView::Columns() const
{
	RECT client = {};
	GetClientRect(m_Hwnd, &client);
	const int available = client.right - m_PaddingX * 2 + m_CellGap;
	return (std::max)(1, available / (CellWidth() + m_CellGap));
}

int OamGridView::ContentHeight() const
{
	if (m_Cells.empty())
	{
		return 0;
	}

	const int count = static_cast<int>(m_Cells.size());
	const int rows = (count + Columns() - 1) / Columns();
	return m_PaddingTop + m_HeaderHeight + m_HeaderGap + rows * CellHeight() + (rows - 1) * m_CellGap + m_PaddingBottom;
}

RECT OamGridView::CellRect(int position) const
{
	const int columns = Columns();
	const int x = m_PaddingX + (position % columns) * (CellWidth() + m_CellGap);
	const int y = m_PaddingTop + m_HeaderHeight + m_HeaderGap + (position / columns) * (CellHeight() + m_CellGap) - m_ScrollY;
	return { x, y, x + CellWidth(), y + CellHeight() };
}

int OamGridView::PositionOf(int index) const
{
	for (size_t i = 0; i < m_Cells.size(); ++i)
	{
		if (m_Cells[i].index == index)
		{
			return static_cast<int>(i);
		}
	}

	return -1;
}

void OamGridView::UpdateScrollBar()
{
	if (m_Hwnd == NULL)
	{
		return;
	}

	// Showing or hiding the scroll bar changes the width, and with it the columns, so go round until it settles
	for (int pass = 0; pass < 3; ++pass)
	{
		RECT client;
		GetClientRect(m_Hwnd, &client);
		const int content = ContentHeight();

		m_ScrollY = std::clamp(m_ScrollY, 0, (std::max)(0, content - static_cast<int>(client.bottom)));

		SCROLLINFO info = { sizeof(SCROLLINFO) };
		info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
		info.nMin = 0;
		info.nMax = (std::max)(0, content - 1);
		info.nPage = client.bottom;
		info.nPos = m_ScrollY;
		SetScrollInfo(m_Hwnd, SB_VERT, &info, TRUE);

		RECT after;
		GetClientRect(m_Hwnd, &after);
		if (after.right == client.right && after.bottom == client.bottom)
		{
			break;
		}
	}
}

void OamGridView::ScrollTo(int y)
{
	RECT client;
	GetClientRect(m_Hwnd, &client);

	y = std::clamp(y, 0, (std::max)(0, ContentHeight() - static_cast<int>(client.bottom)));
	if (y == m_ScrollY)
	{
		return;
	}

	m_ScrollY = y;
	SetScrollPos(m_Hwnd, SB_VERT, m_ScrollY, TRUE);
	InvalidateRect(m_Hwnd, NULL, FALSE);

	// The cell under the mouse moved
	UpdateHoverFromCursor();
}

void OamGridView::OnScroll(WORD request)
{
	SCROLLINFO info = { sizeof(SCROLLINFO) };
	info.fMask = SIF_ALL;
	GetScrollInfo(m_Hwnd, SB_VERT, &info);

	const int line = CellHeight() + m_CellGap;
	int position = info.nPos;
	switch (request)
	{
		case SB_LINEUP: position -= line; break;
		case SB_LINEDOWN: position += line; break;
		case SB_PAGEUP: position -= static_cast<int>(info.nPage); break;
		case SB_PAGEDOWN: position += static_cast<int>(info.nPage); break;
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION: position = info.nTrackPos; break;
		case SB_TOP: position = 0; break;
		case SB_BOTTOM: position = info.nMax; break;
		default: return;
	}

	ScrollTo(position);
}

int OamGridView::HitTest(int x, int y) const
{
	for (size_t i = 0; i < m_Cells.size(); ++i)
	{
		const RECT rect = CellRect(static_cast<int>(i));
		const POINT point = { x, y };
		if (PtInRect(&rect, point))
		{
			return m_Cells[i].index;
		}
	}

	return -1;
}

void OamGridView::UpdateHoverFromCursor()
{
	if (!m_TrackingMouse)
	{
		return;
	}

	POINT cursor;
	GetCursorPos(&cursor);
	ScreenToClient(m_Hwnd, &cursor);
	SetHover(HitTest(cursor.x, cursor.y));
}

void OamGridView::SetHover(int index)
{
	if (index == m_Hover)
	{
		return;
	}

	m_Hover = index;
	InvalidateRect(m_Hwnd, NULL, FALSE);

	if (HoverChanged)
	{
		HoverChanged();
	}
}

void OamGridView::Select(int index)
{
	if (index == m_Selection)
	{
		return;
	}

	m_Selection = index;
	EnsureVisible(index);
	InvalidateRect(m_Hwnd, NULL, FALSE);

	if (SelectionChanged)
	{
		SelectionChanged();
	}
}

void OamGridView::MoveSelection(UINT key)
{
	if (m_Cells.empty())
	{
		return;
	}

	const int count = static_cast<int>(m_Cells.size());
	const int current = PositionOf(m_Selection);
	if (current < 0)
	{
		Select(m_Cells.front().index);
		return;
	}

	const int columns = Columns();
	int position = current;
	switch (key)
	{
		case VK_LEFT: position -= 1; break;
		case VK_RIGHT: position += 1; break;
		case VK_UP: position -= columns; break;
		case VK_DOWN: position += columns; break;
		case VK_HOME: position = 0; break;
		case VK_END: position = count - 1; break;
	}

	if (position >= 0 && position < count)
	{
		Select(m_Cells[position].index);
	}
}

void OamGridView::EnsureVisible(int index)
{
	const int position = PositionOf(index);
	if (position < 0)
	{
		return;
	}

	RECT client;
	GetClientRect(m_Hwnd, &client);
	const RECT rect = CellRect(position);
	const int margin = 8;

	int y = m_ScrollY;
	if (rect.top - margin < 0) y += rect.top - margin;
	else if (rect.bottom + margin > client.bottom) y += rect.bottom + margin - client.bottom;

	ScrollTo(y);
}

void OamGridView::EnsureBackBuffer(int width, int height)
{
	if (m_BackDC != NULL && width == m_BackWidth && height == m_BackHeight)
	{
		return;
	}

	ReleaseBackBuffer();

	HDC screen = GetDC(NULL);
	m_BackDC = CreateCompatibleDC(screen);
	m_BackBitmap = CreateCompatibleBitmap(screen, width, height);
	ReleaseDC(NULL, screen);

	m_BackPrevious = SelectObject(m_BackDC, m_BackBitmap);
	m_BackWidth = width;
	m_BackHeight = height;
}

void OamGridView::ReleaseBackBuffer()
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

	m_BackWidth = 0;
	m_BackHeight = 0;
}

void OamGridView::Paint(HDC hdc)
{
	RECT client;
	GetClientRect(m_Hwnd, &client);
	if (client.right <= 0 || client.bottom <= 0)
	{
		return;
	}

	EnsureBackBuffer(client.right, client.bottom);
	FillColour(m_BackDC, 0, 0, client.right, client.bottom, BackgroundColour);
	SetBkMode(m_BackDC, TRANSPARENT);

	if (m_Cells.empty())
	{
		SelectObject(m_BackDC, m_Font);
		SetTextColor(m_BackDC, SecondaryTextColour);
		DrawTextW(m_BackDC, m_EmptyText.c_str(), -1, &client, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	}
	else
	{
		PaintHeader();

		for (size_t i = 0; i < m_Cells.size(); ++i)
		{
			const RECT rect = CellRect(static_cast<int>(i));
			if (rect.bottom >= 0 && rect.top < client.bottom)
			{
				PaintCell(m_Cells[i], rect);
			}
		}
	}

	BitBlt(hdc, 0, 0, client.right, client.bottom, m_BackDC, 0, 0, SRCCOPY);
}

void OamGridView::PaintHeader()
{
	RECT client;
	GetClientRect(m_Hwnd, &client);

	const int top = m_PaddingTop - m_ScrollY;
	RECT rect = { m_PaddingX, top, client.right - m_PaddingX, top + m_HeaderHeight };

	// "40 objects  $FE00-$FE9F", or "21 of 40 objects" when hidden ones are left out
	const int count = static_cast<int>(m_Cells.size());
	const std::wstring title = count == Oam::Count ? std::format(L"{} objects", count) : std::format(L"{} of {} objects", count, Oam::Count);

	SelectObject(m_BackDC, m_BoldFont);
	SetTextColor(m_BackDC, TextColour);
	DrawTextW(m_BackDC, title.c_str(), -1, &rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

	SIZE title_size = {};
	GetTextExtentPoint32W(m_BackDC, title.c_str(), static_cast<int>(title.size()), &title_size);

	RECT range = rect;
	range.left += title_size.cx + 8;
	const std::wstring addresses = std::format(L"${:04X}\x2013${:04X}", Oam::Address, Oam::Address + Oam::Size - 1);
	SelectObject(m_BackDC, m_MonoFont);
	SetTextColor(m_BackDC, SecondaryTextColour);
	DrawTextW(m_BackDC, addresses.c_str(), -1, &range, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

	// Key on the right: "▲ Partly dropped   Faded: hidden"
	SelectObject(m_BackDC, m_Font);
	const std::wstring hidden = L"Faded: hidden";
	const std::wstring dropped = L"Partly dropped";

	SIZE hidden_size = {};
	GetTextExtentPoint32W(m_BackDC, hidden.c_str(), static_cast<int>(hidden.size()), &hidden_size);
	SIZE dropped_size = {};
	GetTextExtentPoint32W(m_BackDC, dropped.c_str(), static_cast<int>(dropped.size()), &dropped_size);

	int x = rect.right - hidden_size.cx;
	if (x - dropped_size.cx - 34 < range.left + 120)
	{
		return;
	}

	RECT hidden_rect = { x, rect.top, rect.right, rect.bottom };
	DrawTextW(m_BackDC, hidden.c_str(), -1, &hidden_rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

	x -= 16 + dropped_size.cx;
	RECT dropped_rect = { x, rect.top, x + dropped_size.cx, rect.bottom };
	DrawTextW(m_BackDC, dropped.c_str(), -1, &dropped_rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

	const int middle = (rect.top + rect.bottom) / 2;
	const POINT triangle[] = { { x - 10, middle - 5 }, { x - 5, middle + 4 }, { x - 15, middle + 4 } };
	SetDCBrushColor(m_BackDC, WarningColour);
	SetDCPenColor(m_BackDC, WarningColour);
	HGDIOBJ previous_brush = SelectObject(m_BackDC, GetStockObject(DC_BRUSH));
	HGDIOBJ previous_pen = SelectObject(m_BackDC, GetStockObject(DC_PEN));
	Polygon(m_BackDC, triangle, 3);
	SelectObject(m_BackDC, previous_brush);
	SelectObject(m_BackDC, previous_pen);
}

void OamGridView::PaintCell(const Cell& cell, const RECT& rect)
{
	const bool selected = cell.index == m_Selection;
	const bool hover = cell.index == m_Hover;

	FillColour(m_BackDC, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
		selected ? SelectedCellColour : (hover ? HoverCellColour : CellColour));

	if (selected)
	{
		FrameColour(m_BackDC, rect, 0, SelectionColour);
		FrameColour(m_BackDC, rect, 1, SelectionColour);
	}
	else
	{
		FrameColour(m_BackDC, rect, 0, hover ? SelectionColour : BorderColour);
	}

	// Index, top left
	const int text_left = rect.left + m_CellPadding + 2;
	const int text_right = rect.right - m_CellPadding - 2;
	RECT index_rect = { text_left, rect.top + m_CellPadding, text_right, rect.top + m_CellPadding + m_TextHeight };
	const std::wstring index = std::format(L"{:02}", cell.index);
	SelectObject(m_BackDC, m_MonoFont);
	SetTextColor(m_BackDC, selected ? SelectedTextColour : SecondaryTextColour);
	DrawTextW(m_BackDC, index.c_str(), -1, &index_rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

	// Warning triangle, top right, when the line limit drops part of it
	if (cell.dropped)
	{
		const int right = text_right;
		const int middle = (index_rect.top + index_rect.bottom) / 2;
		const POINT triangle[] = { { right - 5, middle - 5 }, { right, middle + 4 }, { right - 10, middle + 4 } };
		SetDCBrushColor(m_BackDC, WarningColour);
		SetDCPenColor(m_BackDC, WarningColour);
		HGDIOBJ previous_brush = SelectObject(m_BackDC, GetStockObject(DC_BRUSH));
		HGDIOBJ previous_pen = SelectObject(m_BackDC, GetStockObject(DC_PEN));
		Polygon(m_BackDC, triangle, 3);
		SelectObject(m_BackDC, previous_brush);
		SelectObject(m_BackDC, previous_pen);
	}

	// Image on a checkerboard, so transparent pixels show as such
	const int zoom = m_Zoom;
	const int image_width = 8 * zoom;
	const int image_height = cell.height * zoom;
	const int image_x = rect.left + (rect.right - rect.left - image_width) / 2;
	const int image_y = index_rect.bottom + m_TextGap;

	const int square = (std::max)(4, zoom);
	for (int y = 0; y < image_height; y += square)
	{
		for (int x = 0; x < image_width; x += square)
		{
			const bool dark = ((x / square) + (y / square)) % 2 == 1;
			FillColour(m_BackDC, image_x + x, image_y + y, (std::min)(square, image_width - x), (std::min)(square, image_height - y), dark ? CheckerDark : CheckerLight);
		}
	}

	if (cell.pixels.size() == static_cast<size_t>(8 * cell.height))
	{
		for (int y = 0; y < cell.height; ++y)
		{
			for (int x = 0; x < 8; ++x)
			{
				const uint32_t pixel = cell.pixels[static_cast<size_t>(y) * 8 + x];
				if ((pixel & 0xFF000000) != 0)
				{
					FillColour(m_BackDC, image_x + x * zoom, image_y + y * zoom, zoom, zoom, ToColorRef(pixel, cell.hidden));
				}
			}
		}
	}

	// Caption, bottom
	RECT caption_rect = { text_left, image_y + image_height + m_TextGap, text_right, image_y + image_height + m_TextGap + m_TextHeight };
	SetTextColor(m_BackDC, SecondaryTextColour);
	DrawTextW(m_BackDC, cell.caption.c_str(), -1, &caption_rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
}

LRESULT OamGridView::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
			const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
			ScrollTo(m_ScrollY - delta * (CellHeight() + m_CellGap) / WHEEL_DELTA);
			return 0;
		}

		case WM_MOUSEMOVE:
		{
			if (!m_TrackingMouse)
			{
				TRACKMOUSEEVENT track = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, hwnd, 0 };
				m_TrackingMouse = TrackMouseEvent(&track) != FALSE;
			}

			SetHover(HitTest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));
			return 0;
		}

		case WM_MOUSELEAVE:
			m_TrackingMouse = false;
			SetHover(-1);
			return 0;

		case WM_LBUTTONDOWN:
		{
			SetFocus(hwnd);

			const int index = HitTest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			if (index >= 0)
			{
				Select(index);
			}

			return 0;
		}

		case WM_LBUTTONDBLCLK:
		{
			const int index = HitTest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			if (index >= 0)
			{
				Select(index);
				if (ActivateRequested)
				{
					ActivateRequested();
				}
			}

			return 0;
		}

		case WM_GETDLGCODE:
			// Keep the arrow keys and Enter rather than letting the dialog manager use them
			if (lParam != 0 && reinterpret_cast<const MSG*>(lParam)->message == WM_KEYDOWN && reinterpret_cast<const MSG*>(lParam)->wParam == VK_RETURN)
			{
				return DLGC_WANTARROWS | DLGC_WANTMESSAGE;
			}

			return DLGC_WANTARROWS;

		case WM_KEYDOWN:
			if (wParam == VK_LEFT || wParam == VK_RIGHT || wParam == VK_UP || wParam == VK_DOWN || wParam == VK_HOME || wParam == VK_END)
			{
				MoveSelection(static_cast<UINT>(wParam));
				return 0;
			}

			if (wParam == VK_RETURN && m_Selection >= 0 && ActivateRequested)
			{
				ActivateRequested();
				return 0;
			}

			break;

		case WM_DESTROY:
			m_Hwnd = NULL;
			m_TrackingMouse = false;
			m_Hover = -1;
			m_Selection = -1;
			return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}
