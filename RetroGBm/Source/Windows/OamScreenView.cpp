#include "Windows/OamScreenView.h"

#include <windowsx.h>

#include <algorithm>
#include <format>

namespace
{
	const wchar_t* ClassName = L"RetroGBmOamScreenView";

	const COLORREF BackgroundColour = RGB(243, 243, 243);
	const COLORREF TextColour = RGB(26, 26, 26);
	const COLORREF SecondaryTextColour = RGB(92, 92, 92);
	const COLORREF OutlineColour = RGB(200, 200, 200);
	const COLORREF ObjectOutlineColour = RGB(90, 90, 90);
	const COLORREF SelectionColour = RGB(0, 95, 184);
	const COLORREF WarningColour = RGB(197, 90, 0);
	const COLORREF BarColour = RGB(142, 155, 168);
	const COLORREF AxisColour = RGB(168, 168, 168);

	// Screen without a background: a pale green with a faint line every 8 pixels
	const uint32_t ScreenColour = 0xEEF1E8;
	const uint32_t ScreenGridColour = 0xE2E6DA;

	static LRESULT CALLBACK ScreenWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		OamScreenView* view = nullptr;
		if (msg == WM_NCCREATE)
		{
			CREATESTRUCT* create = reinterpret_cast<CREATESTRUCT*>(lParam);
			view = reinterpret_cast<OamScreenView*>(create->lpCreateParams);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(view));
		}
		else
		{
			view = reinterpret_cast<OamScreenView*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
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

	uint32_t ToPixel(COLORREF colour)
	{
		return (static_cast<uint32_t>(GetRValue(colour)) << 16) | (static_cast<uint32_t>(GetGValue(colour)) << 8) | GetBValue(colour);
	}

	// Halfway to white, so outlines and bands stand out over the game's own picture
	uint32_t Fade(uint32_t colour)
	{
		const uint32_t r = (((colour >> 16) & 0xFF) + 255) >> 1;
		const uint32_t g = (((colour >> 8) & 0xFF) + 255) >> 1;
		const uint32_t b = ((colour & 0xFF) + 255) >> 1;
		return (r << 16) | (g << 8) | b;
	}

	// Text from x, centred in a band of height from y
	void DrawTextLeft(HDC hdc, const std::wstring& text, int x, int y, int height)
	{
		RECT rect = { x, y, x + 2000, y + height };
		DrawTextW(hdc, text.c_str(), -1, &rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	}

	int TextWidth(HDC hdc, const std::wstring& text)
	{
		SIZE size = {};
		GetTextExtentPoint32W(hdc, text.c_str(), static_cast<int>(text.size()), &size);
		return size.cx;
	}
}

OamScreenView::~OamScreenView()
{
	this->Destroy();
}

void OamScreenView::Create(HWND parent, int id)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);

	WNDCLASS wc = {};
	wc.style = CS_DBLCLKS;
	wc.lpfnWndProc = ScreenWndProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.lpszClassName = ClassName;
	RegisterClass(&wc);

	// Fonts: the system message font, and Consolas for indices
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

	m_Hwnd = CreateWindowEx(0, ClassName, L"", WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS,
		0, 0, 0, 0, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), hInstance, this);
	if (m_Hwnd == NULL)
	{
		throw std::exception("CreateWindow Failed");
	}
}

void OamScreenView::Destroy()
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

void OamScreenView::SetScene(const Scene& scene)
{
	if (scene == m_Scene)
	{
		return;
	}

	m_Scene = scene;
	InvalidateRect(m_Hwnd, NULL, FALSE);

	// The object under the mouse may have moved, or gone
	if (m_TrackingMouse)
	{
		UpdateHoverFromCursor();
	}
}

void OamScreenView::SetShowOutlines(bool show)
{
	if (show != m_ShowOutlines)
	{
		m_ShowOutlines = show;
		InvalidateRect(m_Hwnd, NULL, FALSE);
	}
}

void OamScreenView::SetShowLineLimit(bool show)
{
	if (show != m_ShowLineLimit)
	{
		m_ShowLineLimit = show;
		InvalidateRect(m_Hwnd, NULL, FALSE);
	}
}

void OamScreenView::SetEmptyText(const std::wstring& text)
{
	if (text != m_EmptyText)
	{
		m_EmptyText = text;
		InvalidateRect(m_Hwnd, NULL, FALSE);
	}
}

void OamScreenView::SetEmpty(bool empty)
{
	if (empty != m_Empty)
	{
		m_Empty = empty;
		InvalidateRect(m_Hwnd, NULL, FALSE);
	}
}

void OamScreenView::SetSelection(int index)
{
	if (index != m_Selection)
	{
		m_Selection = index;
		InvalidateRect(m_Hwnd, NULL, FALSE);
	}
}

int OamScreenView::Zoom() const
{
	RECT client = {};
	GetClientRect(m_Hwnd, &client);

	const int width = client.right - m_Padding * 2 - m_ChartGap - m_ChartWidth;
	const int height = client.bottom - m_Padding * 2 - m_HeaderHeight - m_HeaderGap - m_LegendGap - m_LegendHeight * 2;
	const int zoom = (std::min)(width / Oam::ScreenWidth, height / Oam::ScreenHeight);
	return std::clamp(zoom, 1, 3);
}

POINT OamScreenView::ScreenOrigin() const
{
	return { m_Padding, m_Padding + m_HeaderHeight + m_HeaderGap };
}

RECT OamScreenView::ObjectRect(const Object& object) const
{
	const POINT origin = ScreenOrigin();
	const int zoom = Zoom();
	const int x = origin.x + object.x * zoom;
	const int y = origin.y + object.y * zoom;
	return { x, y, x + 8 * zoom, y + object.height * zoom };
}

void OamScreenView::EnsureBackBuffer(int width, int height)
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

void OamScreenView::ReleaseBackBuffer()
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

int OamScreenView::HitTest(int x, int y) const
{
	if (m_Empty)
	{
		return -1;
	}

	// Front to back, so the object drawn on top wins
	const POINT point = { x, y };
	for (auto it = m_Scene.objects.rbegin(); it != m_Scene.objects.rend(); ++it)
	{
		const RECT rect = ObjectRect(*it);
		if (PtInRect(&rect, point))
		{
			return it->index;
		}
	}

	return -1;
}

void OamScreenView::UpdateHoverFromCursor()
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

void OamScreenView::SetHover(int index)
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

void OamScreenView::Select(int index)
{
	if (index == m_Selection)
	{
		return;
	}

	m_Selection = index;
	InvalidateRect(m_Hwnd, NULL, FALSE);

	if (SelectionChanged)
	{
		SelectionChanged();
	}
}

void OamScreenView::MoveSelection(UINT key)
{
	// The arrow keys step through the objects on screen in OAM order
	std::vector<int> indices;
	for (const Object& object : m_Scene.objects)
	{
		indices.push_back(object.index);
	}

	if (indices.empty())
	{
		return;
	}

	std::sort(indices.begin(), indices.end());
	const auto current = std::find(indices.begin(), indices.end(), m_Selection);
	if (current == indices.end())
	{
		Select(indices.front());
		return;
	}

	const bool forward = key == VK_RIGHT || key == VK_DOWN;
	if (forward && current + 1 != indices.end())
	{
		Select(*(current + 1));
	}
	else if (!forward && current != indices.begin())
	{
		Select(*(current - 1));
	}
}

void OamScreenView::SetPixel(int x, int y, uint32_t colour)
{
	if (x >= 0 && y >= 0 && x < m_BackWidth && y < m_BackHeight)
	{
		m_BackPixels[y * m_BackWidth + x] = colour;
	}
}

void OamScreenView::TintRect(int x, int y, int width, int height, uint32_t colour, int alpha)
{
	const int left = (std::max)(0, x);
	const int top = (std::max)(0, y);
	const int right = (std::min)(m_BackWidth, x + width);
	const int bottom = (std::min)(m_BackHeight, y + height);

	const uint32_t tr = (colour >> 16) & 0xFF;
	const uint32_t tg = (colour >> 8) & 0xFF;
	const uint32_t tb = colour & 0xFF;
	for (int py = top; py < bottom; ++py)
	{
		uint32_t* row = m_BackPixels + py * m_BackWidth;
		for (int px = left; px < right; ++px)
		{
			// alpha out of 256
			const uint32_t c = row[px];
			const uint32_t r = (((c >> 16) & 0xFF) * (256 - alpha) + tr * alpha) >> 8;
			const uint32_t g = (((c >> 8) & 0xFF) * (256 - alpha) + tg * alpha) >> 8;
			const uint32_t b = ((c & 0xFF) * (256 - alpha) + tb * alpha) >> 8;
			row[px] = (r << 16) | (g << 8) | b;
		}
	}
}

void OamScreenView::Paint(HDC hdc)
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

	if (m_Empty)
	{
		SelectObject(m_BackDC, m_Font);
		SetTextColor(m_BackDC, SecondaryTextColour);
		DrawTextW(m_BackDC, m_EmptyText.c_str(), -1, &client, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	}
	else
	{
		PaintScreen();
		PaintOutlines();
		PaintChart();
		PaintLegend();
	}

	BitBlt(hdc, 0, 0, client.right, client.bottom, m_BackDC, 0, 0, SRCCOPY);
}

void OamScreenView::PaintScreen()
{
	const POINT origin = ScreenOrigin();
	const int zoom = Zoom();
	const int width = Oam::ScreenWidth * zoom;
	const int height = Oam::ScreenHeight * zoom;

	// Header: "Screen  160×144 · 3×"
	{
		RECT rect = { origin.x, m_Padding, origin.x + width, m_Padding + m_HeaderHeight };
		SelectObject(m_BackDC, m_BoldFont);
		SetTextColor(m_BackDC, TextColour);
		DrawTextW(m_BackDC, L"Screen", -1, &rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

		rect.left += TextWidth(m_BackDC, L"Screen") + 8;
		const std::wstring size = std::format(L"{}\x00D7{} \x00B7 {}\x00D7", Oam::ScreenWidth, Oam::ScreenHeight, zoom);
		SelectObject(m_BackDC, m_MonoFont);
		SetTextColor(m_BackDC, SecondaryTextColour);
		DrawTextW(m_BackDC, size.c_str(), -1, &rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	}

	RECT outline = { origin.x - 1, origin.y - 1, origin.x + width + 1, origin.y + height + 1 };
	SetDCBrushColor(m_BackDC, OutlineColour);
	FrameRect(m_BackDC, &outline, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));

	GdiFlush();

	// The game's last frame faded, or a plain screen with a line every 8 pixels
	const bool background = m_Scene.background.size() == static_cast<size_t>(Oam::ScreenWidth * Oam::ScreenHeight);
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			const int sx = x / zoom;
			const int sy = y / zoom;
			uint32_t colour;
			if (background)
			{
				colour = Fade(m_Scene.background[static_cast<size_t>(sy) * Oam::ScreenWidth + sx]);
			}
			else
			{
				const bool grid = (sx > 0 && sx % 8 == 0 && x % zoom == 0) || (sy > 0 && sy % 8 == 0 && y % zoom == 0);
				colour = grid ? ScreenGridColour : ScreenColour;
			}

			SetPixel(origin.x + x, origin.y + y, colour);
		}
	}

	// Objects, back to front, without the rows the line limit drops
	for (const Object& object : m_Scene.objects)
	{
		if (object.pixels.size() != static_cast<size_t>(8 * object.height))
		{
			continue;
		}

		for (int row = 0; row < object.height; ++row)
		{
			const int line = object.y + row;
			if (line < 0 || line >= Oam::ScreenHeight || std::find(object.dropped.begin(), object.dropped.end(), static_cast<uint8_t>(line)) != object.dropped.end())
			{
				continue;
			}

			for (int column = 0; column < 8; ++column)
			{
				const int sx = object.x + column;
				const uint32_t pixel = object.pixels[static_cast<size_t>(row) * 8 + column];
				if (sx < 0 || sx >= Oam::ScreenWidth || (pixel & 0xFF000000) == 0)
				{
					continue;
				}

				for (int dy = 0; dy < zoom; ++dy)
				{
					for (int dx = 0; dx < zoom; ++dx)
					{
						SetPixel(origin.x + sx * zoom + dx, origin.y + line * zoom + dy, pixel & 0xFFFFFF);
					}
				}
			}
		}
	}

	// Lines over the limit, shaded orange
	if (m_ShowLineLimit)
	{
		for (int line = 0; line < Oam::ScreenHeight; ++line)
		{
			if (m_Scene.lines[line] > Oam::ObjectsPerLine)
			{
				TintRect(origin.x, origin.y + line * zoom, width, zoom, ToPixel(WarningColour), 48);
			}
		}
	}
}

void OamScreenView::PaintOutlines()
{
	const POINT origin = ScreenOrigin();
	const int zoom = Zoom();

	// Clipped to the screen, as objects can hang off its edges
	const int saved = SaveDC(m_BackDC);
	IntersectClipRect(m_BackDC, origin.x, origin.y, origin.x + Oam::ScreenWidth * zoom, origin.y + Oam::ScreenHeight * zoom);

	if (m_ShowOutlines)
	{
		LOGBRUSH brush = { BS_SOLID, ObjectOutlineColour, 0 };
		HPEN pen = ExtCreatePen(PS_COSMETIC | PS_ALTERNATE, 1, &brush, 0, nullptr);
		HGDIOBJ previous_pen = SelectObject(m_BackDC, pen);
		HGDIOBJ previous_brush = SelectObject(m_BackDC, GetStockObject(NULL_BRUSH));

		for (const Object& object : m_Scene.objects)
		{
			if (object.index != m_Selection && object.index != m_Hover)
			{
				const RECT rect = ObjectRect(object);
				Rectangle(m_BackDC, rect.left, rect.top, rect.right, rect.bottom);
			}
		}

		SelectObject(m_BackDC, previous_brush);
		SelectObject(m_BackDC, previous_pen);
		DeleteObject(pen);
	}

	for (const Object& object : m_Scene.objects)
	{
		const RECT rect = ObjectRect(object);
		if (object.index == m_Selection)
		{
			// Blue inside a white outline, so it shows on any colour
			FrameColour(m_BackDC, rect, -1, RGB(255, 255, 255));
			FrameColour(m_BackDC, rect, 0, SelectionColour);
			FrameColour(m_BackDC, rect, 1, SelectionColour);
		}
		else if (object.index == m_Hover)
		{
			FrameColour(m_BackDC, rect, 0, SelectionColour);
		}
	}

	RestoreDC(m_BackDC, saved);

	// Index tag above the selected object, or below it when it's at the top of the screen
	for (const Object& object : m_Scene.objects)
	{
		if (object.index != m_Selection)
		{
			continue;
		}

		const RECT rect = ObjectRect(object);
		const std::wstring text = std::format(L"{:02}", object.index);
		SelectObject(m_BackDC, m_MonoFont);
		const int tag_width = TextWidth(m_BackDC, text) + 8;
		const int tag_height = 16;
		const int left = (std::max)(static_cast<int>(origin.x), static_cast<int>(rect.left));
		const int top = rect.top - tag_height - 2 >= origin.y ? rect.top - tag_height - 2 : rect.bottom + 2;

		FillColour(m_BackDC, left, top, tag_width, tag_height, SelectionColour);
		SetTextColor(m_BackDC, RGB(255, 255, 255));
		RECT tag = { left, top, left + tag_width, top + tag_height };
		DrawTextW(m_BackDC, text.c_str(), -1, &tag, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	}
}

void OamScreenView::PaintChart()
{
	if (!m_ShowLineLimit)
	{
		return;
	}

	const POINT origin = ScreenOrigin();
	const int zoom = Zoom();
	const int x = origin.x + Oam::ScreenWidth * zoom + m_ChartGap;
	const int top = origin.y;
	const int height = Oam::ScreenHeight * zoom;

	// Title
	SelectObject(m_BackDC, m_BoldFont);
	SetTextColor(m_BackDC, TextColour);
	DrawTextLeft(m_BackDC, L"Objects per line", x, m_Padding, m_HeaderHeight);

	// Axis on the left, a bar per line, and a dashed line at the limit
	FillColour(m_BackDC, x, top, 1, height, AxisColour);
	for (int line = 0; line < Oam::ScreenHeight; ++line)
	{
		const int count = (std::min)(static_cast<int>(m_Scene.lines[line]), m_ChartWidth / m_BarWidth);
		if (count > 0)
		{
			FillColour(m_BackDC, x + 1, top + line * zoom, count * m_BarWidth, zoom,
				m_Scene.lines[line] > Oam::ObjectsPerLine ? WarningColour : BarColour);
		}
	}

	const int limit_x = x + 1 + Oam::ObjectsPerLine * m_BarWidth;
	for (int y = top; y < top + height; y += 6)
	{
		FillColour(m_BackDC, limit_x, y, 1, (std::min)(3, top + height - y), SecondaryTextColour);
	}

	// Which lines the selected object covers
	for (const Object& object : m_Scene.objects)
	{
		if (object.index == m_Selection)
		{
			const int first = std::clamp(object.y, 0, static_cast<int>(Oam::ScreenHeight));
			const int last = std::clamp(object.y + object.height, 0, static_cast<int>(Oam::ScreenHeight));
			FillColour(m_BackDC, x - 6, top + first * zoom, 3, (last - first) * zoom, SelectionColour);
		}
	}

	// Scale under the chart
	SelectObject(m_BackDC, m_MonoFont);
	SetTextColor(m_BackDC, SecondaryTextColour);
	DrawTextLeft(m_BackDC, L"0", x - 2, top + height + 2, 14);
	const std::wstring limit = std::to_wstring(Oam::ObjectsPerLine);
	DrawTextLeft(m_BackDC, limit, limit_x - TextWidth(m_BackDC, limit) / 2, top + height + 2, 14);
}

void OamScreenView::PaintLegend()
{
	const POINT origin = ScreenOrigin();
	const int zoom = Zoom();
	int x = origin.x;
	const int y = origin.y + Oam::ScreenHeight * zoom + m_LegendGap;
	const int middle = y + m_LegendHeight / 2;

	SelectObject(m_BackDC, m_Font);
	SetTextColor(m_BackDC, SecondaryTextColour);

	auto item = [&](const std::wstring& text)
	{
		x += 20;
		DrawTextLeft(m_BackDC, text, x, y, m_LegendHeight);
		x += TextWidth(m_BackDC, text) + 20;
	};

	if (m_ShowLineLimit)
	{
		GdiFlush();
		FillColour(m_BackDC, x, middle - 5, 14, 10, RGB(255, 255, 255));
		GdiFlush();
		TintRect(x, middle - 5, 14, 10, ToPixel(WarningColour), 48);
		item(L"Over the " + std::to_wstring(Oam::ObjectsPerLine) + L"-object limit");
	}

	if (m_ShowOutlines)
	{
		LOGBRUSH brush = { BS_SOLID, ObjectOutlineColour, 0 };
		HPEN pen = ExtCreatePen(PS_COSMETIC | PS_ALTERNATE, 1, &brush, 0, nullptr);
		HGDIOBJ previous_pen = SelectObject(m_BackDC, pen);
		HGDIOBJ previous_brush = SelectObject(m_BackDC, GetStockObject(NULL_BRUSH));
		Rectangle(m_BackDC, x + 2, middle - 7, x + 12, middle + 7);
		SelectObject(m_BackDC, previous_brush);
		SelectObject(m_BackDC, previous_pen);
		DeleteObject(pen);
		item(L"Object");
	}

	RECT selected = { x + 2, middle - 7, x + 12, middle + 7 };
	FrameColour(m_BackDC, selected, 0, SelectionColour);
	FrameColour(m_BackDC, selected, 1, SelectionColour);
	item(L"Selected");

	if (!m_Scene.note.empty())
	{
		RECT note = { origin.x, y + m_LegendHeight + 2, m_BackWidth - m_Padding, y + m_LegendHeight * 2 + 2 };
		DrawTextW(m_BackDC, m_Scene.note.c_str(), -1, &note, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
	}
}

LRESULT OamScreenView::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
			InvalidateRect(hwnd, NULL, FALSE);
			return 0;

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
			if (wParam == VK_LEFT || wParam == VK_RIGHT || wParam == VK_UP || wParam == VK_DOWN)
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
