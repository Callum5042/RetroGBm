#include "Windows/TileMapView.h"

#include <RetroGBm/TileMap.h>

#include <windowsx.h>

#include <algorithm>
#include <format>

namespace
{
	const wchar_t* ClassName = L"RetroGBmTileMapView";

	const COLORREF BackgroundColour = RGB(255, 255, 255);
	const COLORREF TextColour = RGB(26, 26, 26);
	const COLORREF SecondaryTextColour = RGB(92, 92, 92);
	const COLORREF AddressColour = RGB(110, 110, 110);
	const COLORREF OutlineColour = RGB(200, 200, 200);

	static LRESULT CALLBACK MapWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		TileMapView* view = nullptr;
		if (msg == WM_NCCREATE)
		{
			CREATESTRUCT* create = reinterpret_cast<CREATESTRUCT*>(lParam);
			view = reinterpret_cast<TileMapView*>(create->lpCreateParams);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(view));
		}
		else
		{
			view = reinterpret_cast<TileMapView*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
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
}

TileMapView::~TileMapView()
{
	this->Destroy();
}

void TileMapView::Create(HWND parent, int id)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);

	WNDCLASS wc = {};
	wc.lpfnWndProc = MapWndProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.lpszClassName = ClassName;
	RegisterClass(&wc);

	// Fonts: the system message font, and Consolas for addresses
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

		mono.lfWeight = FW_BOLD;
		m_MonoBoldFont = CreateFontIndirect(&mono);
		ReleaseDC(NULL, hdc);
	}

	m_Hwnd = CreateWindowEx(0, ClassName, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_HSCROLL | WS_VSCROLL | WS_CLIPSIBLINGS,
		0, 0, 0, 0, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), hInstance, this);
	if (m_Hwnd == NULL)
	{
		throw std::exception("CreateWindow Failed");
	}
}

void TileMapView::Destroy()
{
	if (m_Hwnd != NULL)
	{
		DestroyWindow(m_Hwnd);
		m_Hwnd = NULL;
	}

	UnregisterClass(ClassName, GetModuleHandle(NULL));
	ReleaseBackBuffer();

	for (HFONT* font : { &m_Font, &m_BoldFont, &m_MonoFont, &m_MonoBoldFont })
	{
		if (*font != NULL)
		{
			DeleteObject(*font);
			*font = NULL;
		}
	}
}

void TileMapView::SetImage(const std::vector<uint32_t>& pixels)
{
	if (pixels == m_Pixels)
	{
		return;
	}

	const bool layout_changed = pixels.empty() != m_Pixels.empty();
	m_Pixels = pixels;

	if (layout_changed)
	{
		UpdateScrollBars();
	}

	InvalidateRect(m_Hwnd, NULL, FALSE);
}

void TileMapView::SetMapAddress(uint16_t address, const std::wstring& description)
{
	if (address != m_MapAddress || description != m_Description)
	{
		m_MapAddress = address;
		m_Description = description;
		InvalidateRect(m_Hwnd, NULL, FALSE);
	}
}

void TileMapView::SetZoom(int zoom)
{
	if (zoom == m_Zoom)
	{
		return;
	}

	// Keep the same point in the middle of the view
	RECT client;
	GetClientRect(m_Hwnd, &client);
	const double scale = static_cast<double>(zoom) / m_Zoom;
	const int centre_x = static_cast<int>((m_ScrollX + client.right / 2) * scale) - client.right / 2;
	const int centre_y = static_cast<int>((m_ScrollY + client.bottom / 2) * scale) - client.bottom / 2;

	m_Zoom = zoom;
	m_ScrollX = centre_x;
	m_ScrollY = centre_y;
	UpdateScrollBars();
	InvalidateRect(m_Hwnd, NULL, FALSE);
}

void TileMapView::SetGrid(bool grid)
{
	if (grid != m_Grid)
	{
		m_Grid = grid;
		InvalidateRect(m_Hwnd, NULL, FALSE);
	}
}

void TileMapView::SetEmptyText(const std::wstring& text)
{
	if (text != m_EmptyText)
	{
		m_EmptyText = text;
		InvalidateRect(m_Hwnd, NULL, FALSE);
	}
}

SIZE TileMapView::ContentSize() const
{
	if (m_Pixels.empty())
	{
		return { 0, 0 };
	}

	const int width = m_PaddingX * 2 + m_GutterWidth + m_GutterGap + TileMap::Columns * CellSize();
	const int height = m_PaddingY * 2 + m_HeaderHeight + m_HeaderGap + TileMap::Rows * CellSize();
	return { width, height };
}

POINT TileMapView::MapOrigin() const
{
	return { m_PaddingX + m_GutterWidth + m_GutterGap - m_ScrollX, m_PaddingY + m_HeaderHeight + m_HeaderGap - m_ScrollY };
}

void TileMapView::UpdateScrollBars()
{
	if (m_Hwnd == NULL)
	{
		return;
	}

	// Showing or hiding a scroll bar resizes the client area, so go round until it settles
	for (int pass = 0; pass < 3; ++pass)
	{
		RECT client;
		GetClientRect(m_Hwnd, &client);
		const SIZE content = ContentSize();

		m_ScrollX = std::clamp(m_ScrollX, 0, (std::max)(0, static_cast<int>(content.cx - client.right)));
		m_ScrollY = std::clamp(m_ScrollY, 0, (std::max)(0, static_cast<int>(content.cy - client.bottom)));

		SCROLLINFO info = { sizeof(SCROLLINFO) };
		info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
		info.nMin = 0;

		info.nMax = (std::max)(0, static_cast<int>(content.cx) - 1);
		info.nPage = client.right;
		info.nPos = m_ScrollX;
		SetScrollInfo(m_Hwnd, SB_HORZ, &info, TRUE);

		info.nMax = (std::max)(0, static_cast<int>(content.cy) - 1);
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

void TileMapView::ScrollTo(int x, int y)
{
	RECT client;
	GetClientRect(m_Hwnd, &client);
	const SIZE content = ContentSize();

	x = std::clamp(x, 0, (std::max)(0, static_cast<int>(content.cx - client.right)));
	y = std::clamp(y, 0, (std::max)(0, static_cast<int>(content.cy - client.bottom)));
	if (x == m_ScrollX && y == m_ScrollY)
	{
		return;
	}

	m_ScrollX = x;
	m_ScrollY = y;
	SetScrollPos(m_Hwnd, SB_HORZ, m_ScrollX, TRUE);
	SetScrollPos(m_Hwnd, SB_VERT, m_ScrollY, TRUE);
	InvalidateRect(m_Hwnd, NULL, FALSE);
}

void TileMapView::OnScroll(int bar, WORD request)
{
	SCROLLINFO info = { sizeof(SCROLLINFO) };
	info.fMask = SIF_ALL;
	GetScrollInfo(m_Hwnd, bar, &info);

	int position = info.nPos;
	switch (request)
	{
		case SB_LINEUP: position -= CellSize(); break;
		case SB_LINEDOWN: position += CellSize(); break;
		case SB_PAGEUP: position -= static_cast<int>(info.nPage); break;
		case SB_PAGEDOWN: position += static_cast<int>(info.nPage); break;
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION: position = info.nTrackPos; break;
		case SB_TOP: position = 0; break;
		case SB_BOTTOM: position = info.nMax; break;
		default: return;
	}

	if (bar == SB_HORZ)
	{
		ScrollTo(position, m_ScrollY);
	}
	else
	{
		ScrollTo(m_ScrollX, position);
	}
}

void TileMapView::EnsureBackBuffer(int width, int height)
{
	if (m_BackDC != NULL && width == m_BackWidth && height == m_BackHeight)
	{
		return;
	}

	ReleaseBackBuffer();

	BITMAPINFO info = {};
	info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	info.bmiHeader.biWidth = width;
	info.bmiHeader.biHeight = -height;
	info.bmiHeader.biPlanes = 1;
	info.bmiHeader.biBitCount = 32;
	info.bmiHeader.biCompression = BI_RGB;

	void* bits = nullptr;
	m_BackBitmap = CreateDIBSection(NULL, &info, DIB_RGB_COLORS, &bits, NULL, 0);
	m_BackPixels = static_cast<uint32_t*>(bits);
	m_BackDC = CreateCompatibleDC(NULL);
	m_BackPrevious = SelectObject(m_BackDC, m_BackBitmap);
	m_BackWidth = width;
	m_BackHeight = height;
}

void TileMapView::ReleaseBackBuffer()
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

	m_BackPixels = nullptr;
	m_BackWidth = 0;
	m_BackHeight = 0;
}

void TileMapView::Paint(HDC hdc)
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

	if (m_Pixels.size() == static_cast<size_t>(TileMap::Width * TileMap::Height))
	{
		PaintMap();
	}
	else
	{
		SelectObject(m_BackDC, m_Font);
		SetTextColor(m_BackDC, SecondaryTextColour);
		DrawTextW(m_BackDC, m_EmptyText.c_str(), -1, &client, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	}

	BitBlt(hdc, 0, 0, client.right, client.bottom, m_BackDC, 0, 0, SRCCOPY);
}

void TileMapView::PaintMap()
{
	const POINT origin = MapOrigin();
	const int x = origin.x;
	const int y = origin.y;
	const int cell = CellSize();
	const int width = TileMap::Columns * cell;
	const int height = TileMap::Rows * cell;

	// Header: "Map $9800  $9800-$9BFF · background"
	{
		const std::wstring title = std::format(L"Map ${:04X}", m_MapAddress);
		RECT rect = { x, y - m_HeaderGap - m_HeaderHeight, x + width, y - m_HeaderGap };

		SelectObject(m_BackDC, m_BoldFont);
		SetTextColor(m_BackDC, TextColour);
		DrawTextW(m_BackDC, title.c_str(), -1, &rect, DT_LEFT | DT_BOTTOM | DT_SINGLELINE | DT_NOPREFIX);

		SIZE title_size = {};
		GetTextExtentPoint32W(m_BackDC, title.c_str(), static_cast<int>(title.size()), &title_size);
		rect.left += title_size.cx + 8;

		std::wstring subtitle = std::format(L"${:04X}\x2013${:04X}", m_MapAddress, m_MapAddress + TileMap::Entries - 1);
		if (!m_Description.empty())
		{
			subtitle += L" \x00B7 " + m_Description;
		}

		SelectObject(m_BackDC, m_MonoFont);
		SetTextColor(m_BackDC, SecondaryTextColour);
		DrawTextW(m_BackDC, subtitle.c_str(), -1, &rect, DT_LEFT | DT_BOTTOM | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
	}

	// Address of each row of 32 entries, with every eighth row in bold. Only those fit at 1x.
	for (int row = 0; row < TileMap::Rows; ++row)
	{
		const bool major = row % 8 == 0;
		if (!major && cell < 12)
		{
			continue;
		}

		const std::wstring label = std::format(L"${:04X}", m_MapAddress + row * TileMap::Columns);
		RECT rect = { x - m_GutterGap - m_GutterWidth, y + row * cell, x - m_GutterGap, y + (row + 1) * cell };

		SelectObject(m_BackDC, major ? m_MonoBoldFont : m_MonoFont);
		SetTextColor(m_BackDC, major ? TextColour : AddressColour);
		DrawTextW(m_BackDC, label.c_str(), -1, &rect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	}

	// Outline, then the map scaled up without filtering
	{
		RECT outline = { x - 1, y - 1, x + width + 1, y + height + 1 };
		SetDCBrushColor(m_BackDC, OutlineColour);
		FrameRect(m_BackDC, &outline, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));

		BITMAPINFO info = {};
		info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		info.bmiHeader.biWidth = TileMap::Width;
		info.bmiHeader.biHeight = -TileMap::Height;
		info.bmiHeader.biPlanes = 1;
		info.bmiHeader.biBitCount = 32;
		info.bmiHeader.biCompression = BI_RGB;

		SetStretchBltMode(m_BackDC, COLORONCOLOR);
		StretchDIBits(m_BackDC, x, y, width, height, 0, 0, TileMap::Width, TileMap::Height,
			m_Pixels.data(), &info, DIB_RGB_COLORS, SRCCOPY);
	}

	// Grid lines darken the pixels under them so they show on any colour
	if (m_Grid)
	{
		GdiFlush();
		for (int column = 1; column < TileMap::Columns; ++column)
		{
			BlendRect(x + column * cell, y, 1, height);
		}

		for (int row = 1; row < TileMap::Rows; ++row)
		{
			BlendRect(x, y + row * cell, width, 1);
		}
	}
}

void TileMapView::BlendRect(int x, int y, int width, int height)
{
	const int left = (std::max)(0, x);
	const int top = (std::max)(0, y);
	const int right = (std::min)(m_BackWidth, x + width);
	const int bottom = (std::min)(m_BackHeight, y + height);

	for (int py = top; py < bottom; ++py)
	{
		uint32_t* row = m_BackPixels + py * m_BackWidth;
		for (int px = left; px < right; ++px)
		{
			// Each channel to 72%
			const uint32_t c = row[px];
			const uint32_t r = (((c >> 16) & 0xFF) * 184) >> 8;
			const uint32_t g = (((c >> 8) & 0xFF) * 184) >> 8;
			const uint32_t b = ((c & 0xFF) * 184) >> 8;
			row[px] = (r << 16) | (g << 8) | b;
		}
	}
}

LRESULT TileMapView::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
			UpdateScrollBars();
			InvalidateRect(hwnd, NULL, FALSE);
			return 0;

		case WM_HSCROLL:
			OnScroll(SB_HORZ, LOWORD(wParam));
			return 0;

		case WM_VSCROLL:
			OnScroll(SB_VERT, LOWORD(wParam));
			return 0;

		case WM_MOUSEWHEEL:
		{
			// Shift scrolls sideways
			const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
			const int step = delta * CellSize() * 3 / WHEEL_DELTA;
			if (GET_KEYSTATE_WPARAM(wParam) & MK_SHIFT)
			{
				ScrollTo(m_ScrollX - step, m_ScrollY);
			}
			else
			{
				ScrollTo(m_ScrollX, m_ScrollY - step);
			}

			return 0;
		}

		case WM_MOUSEHWHEEL:
		{
			const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
			ScrollTo(m_ScrollX + delta * CellSize() * 3 / WHEEL_DELTA, m_ScrollY);
			return 0;
		}

		case WM_LBUTTONDOWN:
			SetFocus(hwnd);
			return 0;

		case WM_DESTROY:
			m_Hwnd = NULL;
			return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}
