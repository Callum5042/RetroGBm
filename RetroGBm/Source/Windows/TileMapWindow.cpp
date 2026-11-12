#include "Windows/TileMapWindow.h"
#include "Windows/MainWindow.h"
#include "Application.h"
#include "../resource.h"

#include <CommCtrl.h>
#include <ShObjIdl.h>
#include <windowsx.h>
#include <wrl/client.h>
#pragma comment(lib, "comctl32.lib")

#include <RetroGBm/Emulator.h>
#include <RetroGBm/Display.h>
#include <RetroGBm/Ppu.h>
#include <RetroGBm/TileData.h>

// The implementation is compiled in MainWindow.cpp
#include "stb_image_write.h"

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>

namespace
{
	const wchar_t* ClassName = L"RetroGBmTileMap";

	static TileMapWindow* GetWindow(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		TileMapWindow* window = nullptr;
		if (uMsg == WM_NCCREATE)
		{
			CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
			window = reinterpret_cast<TileMapWindow*>(pCreate->lpCreateParams);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
		}
		else
		{
			window = reinterpret_cast<TileMapWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		}

		return window;
	}

	static LRESULT CALLBACK TileMapWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		TileMapWindow* window = GetWindow(hwnd, msg, wParam, lParam);
		if (window == nullptr)
		{
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}

		return window->HandleMessage(hwnd, msg, wParam, lParam);
	}

	int MeasureText(HFONT font, const std::wstring& text)
	{
		HDC hdc = GetDC(NULL);
		HGDIOBJ previous = SelectObject(hdc, font);

		SIZE size = {};
		GetTextExtentPoint32W(hdc, text.c_str(), static_cast<int>(text.size()), &size);

		SelectObject(hdc, previous);
		ReleaseDC(NULL, hdc);
		return size.cx;
	}

	std::wstring GetText(HWND control)
	{
		std::wstring text(static_cast<size_t>(GetWindowTextLength(control)), L'\0');
		GetWindowText(control, text.data(), static_cast<int>(text.size()) + 1);
		return text;
	}

	void SetTextIfChanged(HWND control, const std::wstring& text)
	{
		// Avoids repainting (and flickering) every frame
		if (GetText(control) != text)
		{
			SetWindowText(control, text.c_str());
		}
	}

	COLORREF ToColorRef(uint32_t colour)
	{
		return RGB((colour >> 16) & 0xFF, (colour >> 8) & 0xFF, colour & 0xFF);
	}

	void FillColour(HDC hdc, int x, int y, int width, int height, COLORREF colour)
	{
		RECT rect = { x, y, x + width, y + height };
		SetDCBrushColor(hdc, colour);
		FillRect(hdc, &rect, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
	}

	// Inspector preview: 8 x 8 pixels of 20px, with the palette beside it
	const int PreviewPixelSize = 20;
	const int PreviewSize = 8 * PreviewPixelSize;

	const wchar_t* NoEntryHint = L"Hover over the map to see an entry's address, or click one to pin it here.";
	const wchar_t* EntryHint = L"Click an entry to pin it here. The arrow keys move the selection.";

	// Display colours are 0xAABBGGRR, the map view wants 0x00RRGGBB
	uint32_t ToViewColour(uint32_t colour)
	{
		return ((colour & 0xFF) << 16) | (colour & 0xFF00) | ((colour >> 16) & 0xFF);
	}

	// VRAM is copied from $8000 to $9FFF
	constexpr int VramSize = 0x2000;

	const COLORREF DividerColour = RGB(223, 223, 223);
	const COLORREF InspectorColour = RGB(250, 250, 250);
	const COLORREF SecondaryTextColour = RGB(92, 92, 92);
}

TileMapWindow::TileMapWindow(Application* application) : m_Application(application)
{
}

TileMapWindow::~TileMapWindow()
{
	this->Destroy();
}

void TileMapWindow::Create()
{
	if (IsOpen())
	{
		return;
	}

	// Use the system message font (Segoe UI 9pt on Windows 10/11)
	{
		NONCLIENTMETRICS metrics = { sizeof(NONCLIENTMETRICS) };
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0);
		m_Font = CreateFontIndirect(&metrics.lfMessageFont);

		HDC hdc = GetDC(NULL);
		LOGFONT heading = metrics.lfMessageFont;
		heading.lfWeight = FW_SEMIBOLD;
		heading.lfHeight = -MulDiv(15, GetDeviceCaps(hdc, LOGPIXELSY), 72);
		m_HeadingFont = CreateFontIndirect(&heading);

		// Consolas for addresses
		LOGFONT mono = {};
		mono.lfHeight = -MulDiv(9, GetDeviceCaps(hdc, LOGPIXELSY), 72);
		mono.lfWeight = FW_NORMAL;
		mono.lfQuality = CLEARTYPE_QUALITY;
		mono.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
		wcscpy_s(mono.lfFaceName, L"Consolas");
		m_MonoFont = CreateFontIndirect(&mono);

		mono.lfHeight = -MulDiv(8, GetDeviceCaps(hdc, LOGPIXELSY), 72);
		m_MonoSmallFont = CreateFontIndirect(&mono);
		ReleaseDC(NULL, hdc);
	}

	m_InspectorBrush = CreateSolidBrush(InspectorColour);

	INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_STANDARD_CLASSES | ICC_BAR_CLASSES };
	InitCommonControlsEx(&icex);

	this->WindowCreate(m_ClientWidth, m_ClientHeight);

	// A new window starts with the plain title
	m_TitleFilename.clear();
	this->UpdateTitle();
	this->CreateControls();

	Emulator* emulator = m_Application->GetEmulator();
	m_IsColour = emulator != nullptr && emulator->IsRunning() && emulator->GetDisplay()->IsColour();

	this->Layout();
	this->CaptureSnapshot();
	this->RenderMap();
	this->UpdateStatus();

	ShowWindow(m_Hwnd, SW_SHOWNORMAL);
	SetFocus(m_ComboMap);
}

void TileMapWindow::Destroy()
{
	// WM_DESTROY clears m_Hwnd
	if (m_Hwnd != NULL)
	{
		DestroyWindow(m_Hwnd);
	}

	UnregisterClass(ClassName, GetModuleHandle(NULL));
	ReleaseResources();
}

void TileMapWindow::ReleaseResources()
{
	m_MapView.Destroy();

	for (HFONT* font : { &m_Font, &m_HeadingFont, &m_MonoFont, &m_MonoSmallFont })
	{
		if (*font != NULL)
		{
			DeleteObject(*font);
			*font = NULL;
		}
	}

	if (m_InspectorBrush != NULL)
	{
		DeleteObject(m_InspectorBrush);
		m_InspectorBrush = NULL;
	}
}

void TileMapWindow::Focus()
{
	if (IsIconic(m_Hwnd))
	{
		ShowWindow(m_Hwnd, SW_RESTORE);
	}

	SetForegroundWindow(m_Hwnd);
}

void TileMapWindow::Update()
{
	if (!IsOpen())
	{
		return;
	}

	// A game may have been loaded, or swapped for one of the other type, while the window was open
	Emulator* emulator = m_Application->GetEmulator();
	const bool is_colour = emulator != nullptr && emulator->IsRunning() && emulator->GetDisplay()->IsColour();
	if (is_colour != m_IsColour)
	{
		m_IsColour = is_colour;
		this->CaptureSnapshot();
		this->RenderMap();
	}

	if (Button_GetCheck(m_ButtonPause) != BST_CHECKED)
	{
		this->CaptureSnapshot();
		this->RenderMap();
	}

	this->UpdateTitle();
	this->UpdateStatus();
}

void TileMapWindow::UpdateTitle()
{
	if (m_TitleFilename == m_Application->CurrentFilename)
	{
		return;
	}

	m_TitleFilename = m_Application->CurrentFilename;

	// Show the ROM file name without its extension, e.g. "Tile Map - Pokemon Red (UE)".
	// CurrentFilename is in the ANSI code page, which path converts back.
	const std::wstring rom_name = std::filesystem::path(m_TitleFilename).stem().wstring();
	const std::wstring title = rom_name.empty() ? L"Tile Map" : L"Tile Map \x2014 " + rom_name;
	SetWindowText(m_Hwnd, title.c_str());
}

void TileMapWindow::CaptureSnapshot()
{
	Emulator* emulator = m_Application->GetEmulator();
	if (emulator == nullptr || !emulator->IsRunning())
	{
		return;
	}

	Ppu* ppu = emulator->GetPpu();
	Display* display = emulator->GetDisplay();

	const int banks = m_IsColour ? 2 : 1;
	for (int bank = 0; bank < 2; ++bank)
	{
		if (bank >= banks)
		{
			m_Snapshot.vram[bank].clear();
			continue;
		}

		m_Snapshot.vram[bank].resize(VramSize);
		for (int i = 0; i < VramSize; ++i)
		{
			m_Snapshot.vram[bank][i] = ppu->ReadVideoRam(static_cast<uint16_t>(0x8000 + i), static_cast<uint8_t>(bank));
		}
	}

	m_Snapshot.lcdc = display->Read(0xFF40);
	m_Snapshot.scy = display->Read(0xFF42);
	m_Snapshot.scx = display->Read(0xFF43);
	m_Snapshot.wy = display->Read(0xFF4A);
	m_Snapshot.wx = display->Read(0xFF4B);

	for (uint8_t palette = 0; palette < 8; ++palette)
	{
		for (uint8_t index = 0; index < 4; ++index)
		{
			m_Snapshot.colours[palette][index] = ToViewColour(display->GetColourFromBackgroundPalette(palette, index));
		}
	}

	m_HasSnapshot = true;
}

TileMapWindow::MapChoice TileMapWindow::SelectedMap() const
{
	return static_cast<MapChoice>(ComboBox_GetItemData(m_ComboMap, ComboBox_GetCurSel(m_ComboMap)));
}

uint16_t TileMapWindow::MapAddress() const
{
	switch (SelectedMap())
	{
		case MapChoice::Background: return TileMap::BackgroundMap(m_Snapshot.lcdc);
		case MapChoice::Window: return TileMap::WindowMap(m_Snapshot.lcdc);
		case MapChoice::High: return TileMap::HighMap;
		default: return TileMap::LowMap;
	}
}

TileMap::Addressing TileMapWindow::Addressing() const
{
	switch (static_cast<AddressingChoice>(ComboBox_GetCurSel(m_ComboAddressing)))
	{
		case AddressingChoice::Unsigned: return TileMap::Addressing::Unsigned;
		case AddressingChoice::Signed: return TileMap::Addressing::Signed;
		default: return TileMap::AddressingFromLcdc(m_Snapshot.lcdc);
	}
}

void TileMapWindow::RenderMap()
{
	if (!m_HasSnapshot || m_Snapshot.vram[0].size() != VramSize)
	{
		m_MapView.SetEmptyText(L"Load a game to see its tile maps.");
		m_MapView.SetImage({});
		m_MapPixels.clear();
		UpdateInspector();
		EnableWindow(m_ButtonExport, FALSE);
		return;
	}

	const uint16_t address = MapAddress();
	const bool background = address == TileMap::BackgroundMap(m_Snapshot.lcdc);
	const bool window = address == TileMap::WindowMap(m_Snapshot.lcdc) && !TileMap::WindowOnMap(m_Snapshot.lcdc, m_Snapshot.wx, m_Snapshot.wy).IsEmpty();
	std::wstring description;
	if (background && window) description = L"background and window";
	else if (background) description = L"background";
	else if (window) description = L"window";
	else description = L"not in use";
	m_MapView.SetMapAddress(address, description);

	// Attributes are in bank 1 at the same address as the IDs
	const size_t offset = address - 0x8000;
	const bool colour = m_Snapshot.vram[1].size() == VramSize;
	const uint8_t* ids = m_Snapshot.vram[0].data() + offset;
	const uint8_t* attributes = colour ? m_Snapshot.vram[1].data() + offset : nullptr;
	const uint8_t* bank1 = colour ? m_Snapshot.vram[1].data() : nullptr;

	const TileMap::Image image = TileMap::Decode(ids, attributes, m_Snapshot.vram[0].data(), bank1, Addressing());

	std::vector<uint32_t> pixels(image.colours.size());
	for (size_t i = 0; i < pixels.size(); ++i)
	{
		pixels[i] = m_Snapshot.colours[image.palettes[i]][image.colours[i]];
	}

	m_MapView.SetImage(pixels);
	m_MapPixels = std::move(pixels);

	if (IsWindowEnabled(m_ButtonExport) == FALSE)
	{
		EnableWindow(m_ButtonExport, TRUE);
	}

	// What of this map is on screen: the screen and the part the window hides on the background map,
	// and the part shown as the window on the window map
	TileMapView::Overlay overlay;
	if (background)
	{
		overlay.screen = TileMap::Viewport(m_Snapshot.scx, m_Snapshot.scy);

		const TileMap::Rect covered = TileMap::WindowOnScreen(m_Snapshot.lcdc, m_Snapshot.wx, m_Snapshot.wy);
		if (!covered.IsEmpty())
		{
			overlay.covered = { m_Snapshot.scx + covered.x, m_Snapshot.scy + covered.y, covered.width, covered.height };
		}
	}

	if (window)
	{
		overlay.window = TileMap::WindowOnMap(m_Snapshot.lcdc, m_Snapshot.wx, m_Snapshot.wy);
	}

	m_MapView.SetOverlay(overlay);

	UpdateInspector();
}

std::wstring TileMapWindow::ScreenPositionText(int entry) const
{
	const int column = entry % TileMap::Columns;
	const int row = entry / TileMap::Columns;
	const uint16_t address = MapAddress();
	const uint8_t lcdc = m_Snapshot.lcdc;

	std::wstring text;
	auto add = [&text](const std::wstring& part)
	{
		text += text.empty() ? part : L", " + part;
	};

	if (address == TileMap::BackgroundMap(lcdc))
	{
		if (const auto position = TileMap::ScreenPosition(column, row, m_Snapshot.scx, m_Snapshot.scy))
		{
			add(std::format(L"on screen at {}, {}", position->x, position->y));
		}
	}

	// The window map is drawn from its top left, at WX - 7, WY
	const TileMap::Rect window = TileMap::WindowOnMap(lcdc, m_Snapshot.wx, m_Snapshot.wy);
	if (address == TileMap::WindowMap(lcdc) && !window.IsEmpty())
	{
		const int x = column * 8 - window.x;
		const int y = row * 8 - window.y;
		if (x > -8 && x < window.width && y < window.height)
		{
			const TileMap::Rect screen = TileMap::WindowOnScreen(lcdc, m_Snapshot.wx, m_Snapshot.wy);
			add(std::format(L"in the window at {}, {}", screen.x + x, screen.y + y));
		}
	}

	return text.empty() ? L"off screen" : text;
}

void TileMapWindow::UpdateInspector()
{
	const int entry = m_MapView.GetSelection();
	const bool has_entry = entry >= 0 && m_HasSnapshot && m_Snapshot.vram[0].size() == VramSize;

	// Show the details only while an entry is selected
	if (has_entry != m_InspectorHasEntry)
	{
		m_InspectorHasEntry = has_entry;

		std::vector<HWND> details = { m_LabelSubtitle, m_Preview };
		details.insert(details.end(), m_PropertyLabels.begin(), m_PropertyLabels.end());
		details.insert(details.end(), m_PropertyValues.begin(), m_PropertyValues.end());
		for (HWND control : details)
		{
			ShowWindow(control, has_entry ? SW_SHOWNA : SW_HIDE);
		}

		SetTextIfChanged(m_LabelHint, has_entry ? EntryHint : NoEntryHint);
		Layout();
	}

	if (!has_entry)
	{
		SetTextIfChanged(m_LabelHeading, L"No entry selected");
		return;
	}

	const int column = entry % TileMap::Columns;
	const int row = entry / TileMap::Columns;
	const uint16_t address = static_cast<uint16_t>(MapAddress() + entry);
	const size_t offset = address - 0x8000;
	const uint8_t id = m_Snapshot.vram[0][offset];
	const bool colour = m_Snapshot.vram[1].size() == VramSize;
	const uint8_t attribute_byte = colour ? m_Snapshot.vram[1][offset] : 0;
	const TileMap::Attributes attributes = TileMap::DecodeAttributes(attribute_byte);
	const TileMap::Addressing addressing = Addressing();
	const uint16_t data = TileMap::TileAddress(id, addressing);

	SetTextIfChanged(m_LabelHeading, std::format(L"Entry ({}, {})", column, row));
	SetTextIfChanged(m_LabelSubtitle, ScreenPositionText(entry));

	// In $8800 mode IDs are signed, counting from $9000
	const std::wstring id_text = (addressing == TileMap::Addressing::Signed && id >= 128)
		? std::format(L"${:02X} ({})", id, static_cast<int>(id) - 256)
		: std::format(L"${:02X}", id);

	std::wstring flip = L"None";
	if (attributes.flip_x && attributes.flip_y) flip = L"X and Y";
	else if (attributes.flip_x) flip = L"X";
	else if (attributes.flip_y) flip = L"Y";

	SetTextIfChanged(m_PropertyValues[0], std::format(L"${:04X}", address));
	SetTextIfChanged(m_PropertyValues[1], id_text);
	SetTextIfChanged(m_PropertyValues[2], colour
		? std::format(L"${:04X}\x2013${:04X} \x00B7 bank {}", data, data + TileData::BytesPerTile - 1, attributes.bank)
		: std::format(L"${:04X}\x2013${:04X}", data, data + TileData::BytesPerTile - 1));
	SetTextIfChanged(m_PropertyValues[3], colour ? std::format(L"${:02X}", attribute_byte) : L"\x2014  none on DMG");
	SetTextIfChanged(m_PropertyValues[4], colour ? std::format(L"BG {}", attributes.palette) : L"BGP");
	SetTextIfChanged(m_PropertyValues[5], colour ? flip : L"\x2014");
	SetTextIfChanged(m_PropertyValues[6], colour ? (attributes.priority ? L"Over objects" : L"Normal") : L"\x2014");

	// Preview pixels, flipped as the entry draws them
	const std::vector<uint8_t>& bank = (colour && attributes.bank == 1) ? m_Snapshot.vram[1] : m_Snapshot.vram[0];
	const std::array<uint8_t, 64> indices = TileData::DecodeTile(bank.data() + (data - 0x8000));
	const std::array<uint32_t, 4>& colours = m_Snapshot.colours[attributes.palette];

	std::array<uint32_t, 64> pixels = {};
	for (int y = 0; y < 8; ++y)
	{
		for (int x = 0; x < 8; ++x)
		{
			const int source_x = attributes.flip_x ? 7 - x : x;
			const int source_y = attributes.flip_y ? 7 - y : y;
			pixels[y * 8 + x] = colours[indices[source_y * 8 + source_x]];
		}
	}

	const std::wstring palette_name = colour ? std::format(L"BG {}", attributes.palette) : L"BGP";
	if (pixels != m_PreviewPixels || colours != m_PreviewColours || palette_name != m_PreviewPaletteName)
	{
		m_PreviewPixels = pixels;
		m_PreviewColours = colours;
		m_PreviewPaletteName = palette_name;
		InvalidateRect(m_Preview, NULL, FALSE);
	}
}

void TileMapWindow::DrawPreview(const DRAWITEMSTRUCT* item)
{
	const RECT& rect = item->rcItem;
	const int width = rect.right - rect.left;
	const int height = rect.bottom - rect.top;

	// Draw off screen so the preview doesn't flicker when it changes every frame
	HDC hdc = CreateCompatibleDC(item->hDC);
	HBITMAP bitmap = CreateCompatibleBitmap(item->hDC, width, height);
	HGDIOBJ previous_bitmap = SelectObject(hdc, bitmap);

	FillColour(hdc, 0, 0, width, height, InspectorColour);

	for (int y = 0; y < 8; ++y)
	{
		for (int x = 0; x < 8; ++x)
		{
			FillColour(hdc, x * PreviewPixelSize, y * PreviewPixelSize, PreviewPixelSize, PreviewPixelSize, ToColorRef(m_PreviewPixels[y * 8 + x]));
		}
	}

	// Pixel grid, darkening what's under it
	{
		HPEN pen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
		HGDIOBJ previous_pen = SelectObject(hdc, pen);
		const int previous_rop = SetROP2(hdc, R2_MASKPEN);
		for (int i = 1; i < 8; ++i)
		{
			MoveToEx(hdc, i * PreviewPixelSize, 0, NULL);
			LineTo(hdc, i * PreviewPixelSize, PreviewSize);
			MoveToEx(hdc, 0, i * PreviewPixelSize, NULL);
			LineTo(hdc, PreviewSize, i * PreviewPixelSize);
		}
		SetROP2(hdc, previous_rop);
		SelectObject(hdc, previous_pen);
		DeleteObject(pen);
	}

	RECT outline = { 0, 0, PreviewSize, PreviewSize };
	SetDCBrushColor(hdc, RGB(200, 200, 200));
	FrameRect(hdc, &outline, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));

	// Palette swatches
	const int swatch_x = PreviewSize + 14;
	SetBkMode(hdc, TRANSPARENT);
	SelectObject(hdc, m_Font);
	SetTextColor(hdc, SecondaryTextColour);
	RECT label = { swatch_x, 0, width, 18 };
	const std::wstring title = L"Palette " + m_PreviewPaletteName;
	DrawTextW(hdc, title.c_str(), -1, &label, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);

	SelectObject(hdc, m_MonoSmallFont);
	SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
	for (int i = 0; i < 4; ++i)
	{
		const int y = 24 + i * 28;
		FillColour(hdc, swatch_x, y, 20, 20, ToColorRef(m_PreviewColours[i]));

		RECT swatch = { swatch_x, y, swatch_x + 20, y + 20 };
		SetDCBrushColor(hdc, RGB(200, 200, 200));
		FrameRect(hdc, &swatch, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));

		const std::wstring text = std::format(L"{}  #{:06X}", i, m_PreviewColours[i] & 0xFFFFFF);
		RECT text_rect = { swatch_x + 26, y, width, y + 20 };
		DrawTextW(hdc, text.c_str(), -1, &text_rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	}

	BitBlt(item->hDC, rect.left, rect.top, width, height, hdc, 0, 0, SRCCOPY);

	SelectObject(hdc, previous_bitmap);
	DeleteObject(bitmap);
	DeleteDC(hdc);
}

void TileMapWindow::ExportPng()
{
	if (m_MapPixels.size() != static_cast<size_t>(TileMap::Width * TileMap::Height))
	{
		return;
	}

	// Take the image now, so it matches what was on screen when Export was clicked
	const int zoom = static_cast<int>(ComboBox_GetItemData(m_ComboZoom, ComboBox_GetCurSel(m_ComboZoom)));
	const bool grid = Button_GetCheck(m_CheckGrid) == BST_CHECKED;
	const int width = TileMap::Width * zoom;
	const int height = TileMap::Height * zoom;
	const int cell = 8 * zoom;

	// RGBA bytes, with grid lines darkening the pixels under them as the view does
	std::vector<uint8_t> rgba(static_cast<size_t>(width) * height * 4);
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			uint32_t colour = m_MapPixels[static_cast<size_t>(y / zoom) * TileMap::Width + x / zoom];
			uint32_t r = (colour >> 16) & 0xFF;
			uint32_t g = (colour >> 8) & 0xFF;
			uint32_t b = colour & 0xFF;

			if (grid && ((x > 0 && x % cell == 0) || (y > 0 && y % cell == 0)))
			{
				r = (r * 184) >> 8;
				g = (g * 184) >> 8;
				b = (b * 184) >> 8;
			}

			uint8_t* pixel = rgba.data() + (static_cast<size_t>(y) * width + x) * 4;
			pixel[0] = static_cast<uint8_t>(r);
			pixel[1] = static_cast<uint8_t>(g);
			pixel[2] = static_cast<uint8_t>(b);
			pixel[3] = 0xFF;
		}
	}

	const std::wstring title = L"Export Tile Map";
	const HRESULT initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(initialized) && initialized != RPC_E_CHANGED_MODE)
	{
		MessageBoxW(m_Hwnd, L"Could not open the save dialog.", title.c_str(), MB_OK | MB_ICONERROR);
		return;
	}

	// Release the dialog and shell item before balancing COM initialization
	{
		Microsoft::WRL::ComPtr<IFileSaveDialog> dialog;
		HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&dialog));
		if (SUCCEEDED(hr))
		{
			// "Pokemon Red map $9800.png"
			const std::wstring rom_name = std::filesystem::path(m_Application->CurrentFilename).stem().wstring();
			const std::wstring map_name = std::format(L"map {:04X}", MapAddress());
			const std::wstring filename = (rom_name.empty() ? L"Tile " + map_name : rom_name + L" " + map_name) + L".png";

			const COMDLG_FILTERSPEC filters[] = { { L"PNG image", L"*.png" } };
			hr = dialog->SetFileTypes(1, filters);
			if (SUCCEEDED(hr)) hr = dialog->SetDefaultExtension(L"png");
			if (SUCCEEDED(hr)) hr = dialog->SetFileName(filename.c_str());
			if (SUCCEEDED(hr)) hr = dialog->SetTitle(L"Save Tile Map");
			if (SUCCEEDED(hr)) hr = dialog->SetOptions(FOS_FORCEFILESYSTEM | FOS_OVERWRITEPROMPT | FOS_PATHMUSTEXIST);
			if (SUCCEEDED(hr)) hr = dialog->Show(m_Hwnd);
		}

		if (SUCCEEDED(hr))
		{
			Microsoft::WRL::ComPtr<IShellItem> item;
			hr = dialog->GetResult(&item);
			PWSTR path = nullptr;
			if (SUCCEEDED(hr)) hr = item->GetDisplayName(SIGDN_FILESYSPATH, &path);
			if (SUCCEEDED(hr))
			{
				std::vector<uint8_t> png;
				auto write = [](void* context, void* data, int size)
				{
					std::vector<uint8_t>* output = static_cast<std::vector<uint8_t>*>(context);
					const uint8_t* bytes = static_cast<const uint8_t*>(data);
					output->insert(output->end(), bytes, bytes + size);
				};
				stbi_write_png_to_func(write, &png, width, height, 4, rgba.data(), width * 4);

				std::ofstream output(std::filesystem::path(path), std::ios::binary | std::ios::trunc);
				output.write(reinterpret_cast<const char*>(png.data()), static_cast<std::streamsize>(png.size()));
				output.close();

				if (png.empty() || !output)
				{
					MessageBoxW(m_Hwnd, L"Could not save the image.", title.c_str(), MB_OK | MB_ICONERROR);
				}
			}
			CoTaskMemFree(path);
		}

		if (FAILED(hr) && hr != HRESULT_FROM_WIN32(ERROR_CANCELLED))
		{
			MessageBoxW(m_Hwnd, L"Could not choose a file to save to.", title.c_str(), MB_OK | MB_ICONERROR);
		}
	}

	if (SUCCEEDED(initialized))
	{
		CoUninitialize();
	}
}

void TileMapWindow::UpdateHoverStatus()
{
	const int entry = m_MapView.GetHover();

	std::wstring text;
	if (entry >= 0 && m_HasSnapshot && m_Snapshot.vram[0].size() == VramSize)
	{
		const uint16_t address = static_cast<uint16_t>(MapAddress() + entry);
		text = std::format(L"({}, {}) \x00B7 ${:04X} \x00B7 tile ${:02X}",
			entry % TileMap::Columns, entry / TileMap::Columns, address, m_Snapshot.vram[0][address - 0x8000]);
	}

	SendMessageW(m_StatusBar, SB_SETTEXTW, 1, reinterpret_cast<LPARAM>(text.c_str()));
}

bool TileMapWindow::HandleDialogMessage(MSG* msg)
{
	return m_Hwnd != NULL && IsDialogMessage(m_Hwnd, msg);
}

void TileMapWindow::WindowCreate(int width, int height)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);

	WNDCLASS wc = {};
	wc.style = CS_VREDRAW | CS_HREDRAW;
	wc.lpfnWndProc = TileMapWndProc;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.lpszClassName = ClassName;
	RegisterClass(&wc);

	const DWORD exStyle = WS_EX_CONTROLPARENT;
	const DWORD dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;

	RECT window_rect = { 0, 0, width, height };
	AdjustWindowRectEx(&window_rect, dwStyle, FALSE, exStyle);
	int window_width = window_rect.right - window_rect.left;
	int window_height = window_rect.bottom - window_rect.top;

	// Centre over the main window, shrunk to fit and kept inside the monitor's work area
	HWND owner = m_Application->GetMainWindow()->GetHwnd();

	RECT owner_rect;
	GetWindowRect(owner, &owner_rect);

	int x = owner_rect.left + ((owner_rect.right - owner_rect.left) - window_width) / 2;
	int y = owner_rect.top + ((owner_rect.bottom - owner_rect.top) - window_height) / 2;

	MONITORINFO monitor = { sizeof(MONITORINFO) };
	if (GetMonitorInfo(MonitorFromWindow(owner, MONITOR_DEFAULTTONEAREST), &monitor))
	{
		const RECT& work = monitor.rcWork;
		if (window_width > work.right - work.left) window_width = work.right - work.left;
		if (window_height > work.bottom - work.top) window_height = work.bottom - work.top;
		if (x + window_width > work.right) x = work.right - window_width;
		if (y + window_height > work.bottom) y = work.bottom - window_height;
		if (x < work.left) x = work.left;
		if (y < work.top) y = work.top;
	}

	m_Hwnd = CreateWindowEx(exStyle, ClassName, L"Tile Map", dwStyle, x, y, window_width, window_height, owner, NULL, hInstance, this);
	if (m_Hwnd == NULL)
	{
		throw std::exception("CreateWindow Failed");
	}
}

void TileMapWindow::CreateControls()
{
	auto create = [this](const wchar_t* type, const wchar_t* text, DWORD style, int id, HFONT font)
	{
		HWND control = CreateWindowW(type, text, WS_CHILD | WS_VISIBLE | style, 0, 0, 0, 0, m_Hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), nullptr, nullptr);
		SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
		return control;
	};

	// Map: the ones LCDC picks for the background and window follow it as the game changes LCDC
	m_LabelMap = create(L"STATIC", L"Map", SS_CENTERIMAGE, 0, m_Font);
	m_ComboMap = create(WC_COMBOBOX, L"", WS_TABSTOP | WS_GROUP | CBS_DROPDOWNLIST, m_ControlMapId, m_Font);
	const std::pair<const wchar_t*, MapChoice> maps[] =
	{
		{ L"Background (LCDC.3)", MapChoice::Background },
		{ L"Window (LCDC.6)", MapChoice::Window },
		{ L"$9800", MapChoice::Low },
		{ L"$9C00", MapChoice::High },
	};
	for (const auto& [text, choice] : maps)
	{
		const int index = ComboBox_AddString(m_ComboMap, text);
		ComboBox_SetItemData(m_ComboMap, index, static_cast<int>(choice));
	}
	ComboBox_SetCurSel(m_ComboMap, 0);

	// Tile data addressing
	m_LabelAddressing = create(L"STATIC", L"Tile data", SS_CENTERIMAGE, 0, m_Font);
	m_ComboAddressing = create(WC_COMBOBOX, L"", WS_TABSTOP | CBS_DROPDOWNLIST, m_ControlAddressingId, m_Font);
	for (const wchar_t* text : { L"Auto (LCDC.4)", L"$8000, unsigned", L"$8800, signed" })
	{
		ComboBox_AddString(m_ComboAddressing, text);
	}
	ComboBox_SetCurSel(m_ComboAddressing, 0);

	// Zoom
	m_LabelZoom = create(L"STATIC", L"Zoom", SS_CENTERIMAGE, 0, m_Font);
	m_ComboZoom = create(WC_COMBOBOX, L"", WS_TABSTOP | CBS_DROPDOWNLIST, m_ControlZoomId, m_Font);
	for (int zoom : { 1, 2, 3, 4 })
	{
		const std::wstring text = std::to_wstring(zoom) + L"\x00D7";
		const int index = ComboBox_AddString(m_ComboZoom, text.c_str());
		ComboBox_SetItemData(m_ComboZoom, index, zoom);
	}
	ComboBox_SetCurSel(m_ComboZoom, 1);

	// Grid, and the screen and window outlines
	m_CheckGrid = create(L"BUTTON", L"Grid", WS_TABSTOP | BS_AUTOCHECKBOX, m_ControlGridId, m_Font);
	m_CheckOverlay = create(L"BUTTON", L"Screen", WS_TABSTOP | BS_AUTOCHECKBOX, m_ControlOverlayId, m_Font);
	Button_SetCheck(m_CheckOverlay, BST_CHECKED);

	// Pause updates and export, on the right
	m_ButtonPause = create(L"BUTTON", L"Pause updates", WS_TABSTOP | BS_AUTOCHECKBOX | BS_PUSHLIKE, m_ControlPauseId, m_Font);
	m_ButtonExport = create(L"BUTTON", L"Export PNG\x2026", WS_TABSTOP | BS_PUSHBUTTON | WS_DISABLED, m_ControlExportId, m_Font);

	// Map
	m_MapView.Create(m_Hwnd, m_ControlMapViewId);
	m_MapView.SetZoom(static_cast<int>(ComboBox_GetItemData(m_ComboZoom, ComboBox_GetCurSel(m_ComboZoom))));
	m_MapView.SetGrid(false);
	m_MapView.HoverChanged = [this]() { UpdateHoverStatus(); };
	m_MapView.SelectionChanged = [this]() { UpdateInspector(); };

	// Inspector, with the details hidden until an entry is selected
	m_LabelHeading = create(L"STATIC", L"No entry selected", SS_LEFT | SS_ENDELLIPSIS, 0, m_HeadingFont);
	m_LabelSubtitle = create(L"STATIC", L"", SS_LEFT | SS_ENDELLIPSIS, 0, m_Font);
	m_Preview = create(L"STATIC", L"", SS_OWNERDRAW, m_ControlPreviewId, m_Font);

	const wchar_t* property_names[] = { L"Map address", L"Tile ID", L"Tile data", L"Attributes", L"Palette", L"Flip", L"Priority" };
	for (size_t i = 0; i < m_PropertyLabels.size(); ++i)
	{
		m_PropertyLabels[i] = create(L"STATIC", property_names[i], SS_LEFT, 0, m_Font);
		m_PropertyValues[i] = create(L"STATIC", L"", SS_LEFT | SS_ENDELLIPSIS, 0, m_MonoFont);
	}

	m_LabelHint = create(L"STATIC", NoEntryHint, SS_LEFT, 0, m_Font);

	std::vector<HWND> details = { m_LabelSubtitle, m_Preview };
	details.insert(details.end(), m_PropertyLabels.begin(), m_PropertyLabels.end());
	details.insert(details.end(), m_PropertyValues.begin(), m_PropertyValues.end());
	for (HWND control : details)
	{
		ShowWindow(control, SW_HIDE);
	}
	m_InspectorHasEntry = false;

	// Status bar
	m_StatusBar = CreateWindowExW(0, STATUSCLASSNAME, L"", WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
		0, 0, 0, 0, m_Hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(m_ControlStatusBarId)), nullptr, nullptr);
	SendMessageW(m_StatusBar, WM_SETFONT, reinterpret_cast<WPARAM>(m_Font), TRUE);
}

void TileMapWindow::UpdateStatus()
{
	Emulator* emulator = m_Application->GetEmulator();
	const bool running = emulator != nullptr && emulator->IsRunning();
	const bool paused = Button_GetCheck(m_ButtonPause) == BST_CHECKED;

	std::wstring text;
	if (!running || !m_HasSnapshot)
	{
		text = L"No game loaded";
	}
	else
	{
		// "CGB · LCDC $91 · BG $9800 · window $9C00 · tiles $8000 · Live, updates every frame"
		const uint8_t lcdc = m_Snapshot.lcdc;
		text = std::format(L"{} \x00B7 LCDC ${:02X} \x00B7 BG ${:04X} \x00B7 window ${:04X}{} \x00B7 tiles ${:04X}",
			m_IsColour ? L"CGB" : L"DMG", lcdc, TileMap::BackgroundMap(lcdc), TileMap::WindowMap(lcdc),
			(lcdc & 0x20) ? L"" : L" (off)", TileMap::AddressingFromLcdc(lcdc) == TileMap::Addressing::Unsigned ? 0x8000 : 0x8800);
		text += paused ? L" \x00B7 Paused, showing a snapshot" : L" \x00B7 Live, updates every frame";
	}

	// Only set when changed, as this is called every frame
	wchar_t current[256] = {};
	SendMessageW(m_StatusBar, SB_GETTEXTW, 0, reinterpret_cast<LPARAM>(current));
	if (text != current)
	{
		SendMessageW(m_StatusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(text.c_str()));
	}
}

void TileMapWindow::Layout()
{
	RECT client;
	GetClientRect(m_Hwnd, &client);
	const int width = client.right;

	// Status bar sizes itself to the bottom of the window
	SendMessageW(m_StatusBar, WM_SIZE, 0, 0);
	RECT status_rect;
	GetWindowRect(m_StatusBar, &status_rect);
	const int status_height = status_rect.bottom - status_rect.top;

	int parts[] = { width - 280 > 0 ? width - 280 : 0, -1 };
	SendMessageW(m_StatusBar, SB_SETPARTS, 2, reinterpret_cast<LPARAM>(parts));

	HDWP defer = BeginDeferWindowPos(32);
	auto place = [&defer](HWND control, int x, int y, int w, int h)
	{
		defer = DeferWindowPos(defer, control, NULL, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
	};

	// Command bar
	const int button_height = 26;
	const int button_y = (m_CommandBarHeight - button_height) / 2;
	const int combo_y = (m_CommandBarHeight - 23) / 2;
	const int group_gap = 20;

	int x = 12;
	auto label = [&](HWND control)
	{
		const int w = MeasureText(m_Font, GetText(control));
		place(control, x, button_y, w, button_height);
		x += w + 8;
	};

	label(m_LabelMap);
	place(m_ComboMap, x, combo_y, 160, 200);
	x += 160 + group_gap;

	label(m_LabelAddressing);
	place(m_ComboAddressing, x, combo_y, 128, 200);
	x += 128 + group_gap;

	label(m_LabelZoom);
	place(m_ComboZoom, x, combo_y, 56, 200);
	x += 56 + group_gap;

	const int grid_width = MeasureText(m_Font, GetText(m_CheckGrid)) + 24;
	place(m_CheckGrid, x, button_y, grid_width, button_height);
	x += grid_width + 8;

	place(m_CheckOverlay, x, button_y, MeasureText(m_Font, GetText(m_CheckOverlay)) + 24, button_height);

	const int export_width = MeasureText(m_Font, GetText(m_ButtonExport)) + 28;
	const int pause_width = MeasureText(m_Font, GetText(m_ButtonPause)) + 28;
	place(m_ButtonExport, width - 12 - export_width, button_y, export_width, button_height);
	place(m_ButtonPause, width - 12 - export_width - 8 - pause_width, button_y, pause_width, button_height);

	// Inspector
	const int inspector_x = width - m_InspectorWidth + 1 + 16;
	const int inspector_width = m_InspectorWidth - 1 - 32;
	const int body_top = m_CommandBarHeight + 1;
	int y = body_top + 16;
	place(m_LabelHeading, inspector_x, y, inspector_width, 28);
	y += 30;

	if (m_InspectorHasEntry)
	{
		place(m_LabelSubtitle, inspector_x, y, inspector_width, 20);
		y += 28;

		place(m_Preview, inspector_x, y, inspector_width, PreviewSize);
		y += PreviewSize + 16;

		const int label_width = 96;
		for (size_t i = 0; i < m_PropertyLabels.size(); ++i)
		{
			place(m_PropertyLabels[i], inspector_x, y, label_width, 20);
			place(m_PropertyValues[i], inspector_x + label_width + 4, y, inspector_width - label_width - 4, 20);
			y += 22;
		}
		y += 12;
	}

	place(m_LabelHint, inspector_x, y, inspector_width, 48);

	// Map fills the rest, left of the inspector
	const int body_height = client.bottom - status_height - body_top;
	place(m_MapView.GetHwnd(), 0, body_top, width - m_InspectorWidth, body_height > 0 ? body_height : 0);

	EndDeferWindowPos(defer);

	InvalidateRect(m_Hwnd, NULL, TRUE);
}

void TileMapWindow::PaintBackground(HDC hdc)
{
	RECT client;
	GetClientRect(m_Hwnd, &client);

	HBRUSH dc_brush = reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH));

	// Grey command bar
	RECT command_bar = client;
	command_bar.bottom = m_CommandBarHeight;
	FillRect(hdc, &command_bar, GetSysColorBrush(COLOR_BTNFACE));

	// 1px divider under the command bar
	RECT divider = client;
	divider.top = m_CommandBarHeight;
	divider.bottom = divider.top + 1;
	SetDCBrushColor(hdc, DividerColour);
	FillRect(hdc, &divider, dc_brush);

	// White map area
	RECT map = client;
	map.top = divider.bottom;
	map.right = client.right - m_InspectorWidth;
	FillRect(hdc, &map, GetSysColorBrush(COLOR_WINDOW));

	// 1px divider and the inspector
	RECT inspector_divider = map;
	inspector_divider.left = map.right;
	inspector_divider.right = inspector_divider.left + 1;
	SetDCBrushColor(hdc, DividerColour);
	FillRect(hdc, &inspector_divider, dc_brush);

	RECT inspector = map;
	inspector.left = inspector_divider.right;
	inspector.right = client.right;
	FillRect(hdc, &inspector, m_InspectorBrush);
}

LRESULT TileMapWindow::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_CLOSE:
			DestroyWindow(hwnd);
			return 0;

		case WM_DESTROY:
			m_Hwnd = NULL;
			m_LastFocus = NULL;

			if (m_Application->GetMainWindow() != nullptr)
			{
				m_Application->GetMainWindow()->ToggleTileMapWindowMenuItem(false);
			}

			return 0;

		case WM_NCDESTROY:
			// The controls are gone, so their fonts can go too
			ReleaseResources();
			return DefWindowProc(hwnd, msg, wParam, lParam);

		case WM_SIZE:
			if (wParam != SIZE_MINIMIZED)
			{
				Layout();
			}

			return 0;

		case WM_GETMINMAXINFO:
		{
			MINMAXINFO* info = reinterpret_cast<MINMAXINFO*>(lParam);
			RECT minimum = { 0, 0, 760, 420 };
			AdjustWindowRectEx(&minimum, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_CONTROLPARENT);
			info->ptMinTrackSize.x = minimum.right - minimum.left;
			info->ptMinTrackSize.y = minimum.bottom - minimum.top;
			return 0;
		}

		case WM_ACTIVATE:
		{
			// Remember which control had focus so it is restored when switching back
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				HWND focus = GetFocus();
				m_LastFocus = IsChild(hwnd, focus) ? focus : NULL;
			}
			else
			{
				SetFocus(m_LastFocus != NULL ? m_LastFocus : m_ComboMap);
			}

			return 0;
		}

		case WM_CTLCOLORSTATIC:
		{
			HDC hdc = reinterpret_cast<HDC>(wParam);
			HWND control = reinterpret_cast<HWND>(lParam);
			SetBkMode(hdc, TRANSPARENT);

			const bool secondary = control == m_LabelHint || control == m_LabelSubtitle ||
				std::find(m_PropertyLabels.begin(), m_PropertyLabels.end(), control) != m_PropertyLabels.end();
			const bool inspector = secondary || control == m_LabelHeading ||
				std::find(m_PropertyValues.begin(), m_PropertyValues.end(), control) != m_PropertyValues.end();

			if (inspector)
			{
				SetTextColor(hdc, secondary ? SecondaryTextColour : GetSysColor(COLOR_WINDOWTEXT));
				return reinterpret_cast<LRESULT>(m_InspectorBrush);
			}

			const bool checkbox = control == m_CheckGrid || control == m_CheckOverlay;
			SetTextColor(hdc, checkbox ? GetSysColor(COLOR_WINDOWTEXT) : SecondaryTextColour);
			return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_BTNFACE));
		}

		case WM_DRAWITEM:
			if (static_cast<int>(wParam) == m_ControlPreviewId)
			{
				DrawPreview(reinterpret_cast<const DRAWITEMSTRUCT*>(lParam));
				return TRUE;
			}

			break;

		case WM_ERASEBKGND:
			PaintBackground(reinterpret_cast<HDC>(wParam));
			return 1;

		case WM_PRINTCLIENT:
			// Themed buttons ask the parent to draw the background behind their rounded corners
			PaintBackground(reinterpret_cast<HDC>(wParam));
			return 0;

		case WM_COMMAND:
		{
			const int id = LOWORD(wParam);
			if (id == IDCANCEL)
			{
				// Esc, sent by IsDialogMessage
				DestroyWindow(hwnd);
				return 0;
			}

			const int code = HIWORD(wParam);
			if (id == m_ControlPauseId && code == BN_CLICKED)
			{
				UpdateStatus();
				return 0;
			}

			if ((id == m_ControlMapId || id == m_ControlAddressingId) && code == CBN_SELCHANGE)
			{
				RenderMap();
				return 0;
			}

			if (id == m_ControlZoomId && code == CBN_SELCHANGE)
			{
				m_MapView.SetZoom(static_cast<int>(ComboBox_GetItemData(m_ComboZoom, ComboBox_GetCurSel(m_ComboZoom))));
				return 0;
			}

			if (id == m_ControlGridId && code == BN_CLICKED)
			{
				m_MapView.SetGrid(Button_GetCheck(m_CheckGrid) == BST_CHECKED);
				return 0;
			}

			if (id == m_ControlExportId && code == BN_CLICKED)
			{
				ExportPng();
				return 0;
			}

			if (id == m_ControlOverlayId && code == BN_CLICKED)
			{
				m_MapView.SetShowOverlay(Button_GetCheck(m_CheckOverlay) == BST_CHECKED);
				return 0;
			}

			break;
		}
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}
