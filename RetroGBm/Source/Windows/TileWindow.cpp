#include "Windows/TileWindow.h"
#include "Windows/MainWindow.h"
#include "Application.h"
#include "../resource.h"

#include <CommCtrl.h>
#include <windowsx.h>
#pragma comment(lib, "comctl32.lib")

#include <RetroGBm/Emulator.h>
#include <RetroGBm/Display.h>
#include <RetroGBm/Ppu.h>
#include <RetroGBm/TileData.h>

#include <algorithm>
#include <filesystem>
#include <format>
#include <string>

namespace
{
	const wchar_t* ClassName = L"RetroGBmTileData";

	static TileWindow* GetWindow(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		TileWindow* window = nullptr;
		if (uMsg == WM_NCCREATE)
		{
			CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
			window = reinterpret_cast<TileWindow*>(pCreate->lpCreateParams);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
		}
		else
		{
			window = reinterpret_cast<TileWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		}

		return window;
	}

	static LRESULT CALLBACK TileWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		TileWindow* window = GetWindow(hwnd, msg, wParam, lParam);
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
	const int PreviewHeight = PreviewSize;

	const wchar_t* NoTileHint = L"Hover over a tile to see its address, or click one to pin it here.";
	const wchar_t* TileHint = L"Click a tile to pin it here. The arrow keys move the selection and Ctrl+C copies its bytes.";

	// Palette list entries store what they select as item data
	enum class PaletteKind : int { Background = 0, Object = 1, Greyscale = 2 };

	LPARAM PaletteItemData(PaletteKind kind, int index)
	{
		return (static_cast<int>(kind) << 8) | index;
	}

	// Display colours are 0xAABBGGRR, the sheet view wants 0x00RRGGBB
	uint32_t ToSheetColour(uint32_t colour)
	{
		return ((colour & 0xFF) << 16) | (colour & 0xFF00) | ((colour >> 16) & 0xFF);
	}

	const COLORREF DividerColour = RGB(223, 223, 223);
	const COLORREF InspectorColour = RGB(250, 250, 250);
	const COLORREF SecondaryTextColour = RGB(92, 92, 92);
}

TileWindow::TileWindow(Application* application) : m_Application(application)
{
}

TileWindow::~TileWindow()
{
	this->Destroy();
}

void TileWindow::Create()
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

		LOGFONT bold = metrics.lfMessageFont;
		bold.lfWeight = FW_SEMIBOLD;
		m_BoldFont = CreateFontIndirect(&bold);

		// Consolas for addresses and bytes
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
	this->ApplyMode();

	this->Layout();
	this->CaptureSnapshot();
	this->RenderSheets();
	this->UpdateStatus();

	ShowWindow(m_Hwnd, SW_SHOWNORMAL);
	SetFocus(m_ButtonBank0);
}

void TileWindow::Destroy()
{
	// WM_DESTROY clears m_Hwnd
	if (m_Hwnd != NULL)
	{
		DestroyWindow(m_Hwnd);
	}

	UnregisterClass(ClassName, GetModuleHandle(NULL));
	ReleaseResources();
}

void TileWindow::ReleaseResources()
{
	m_ExportDialog.reset();
	m_SheetView.Destroy();

	for (HFONT* font : { &m_Font, &m_HeadingFont, &m_BoldFont, &m_MonoFont, &m_MonoSmallFont })
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

void TileWindow::Focus()
{
	if (IsIconic(m_Hwnd))
	{
		ShowWindow(m_Hwnd, SW_RESTORE);
	}

	SetForegroundWindow(m_Hwnd);
}

void TileWindow::Update()
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
		this->ApplyMode();
		this->CaptureSnapshot();
		this->RenderSheets();
	}

	if (Button_GetCheck(m_ButtonPause) != BST_CHECKED)
	{
		this->CaptureSnapshot();
		this->RenderSheets();
	}

	this->UpdateTitle();
	this->UpdateStatus();
}

void TileWindow::SelectTile(int bank, int tile)
{
	if (!IsOpen() || tile < 0 || tile >= TileData::TilesPerBank || bank < 0 || bank > (m_IsColour ? 1 : 0))
	{
		return;
	}

	// Show both banks when the one asked for is hidden
	const std::vector<int> banks = VisibleBanks();
	if (std::find(banks.begin(), banks.end(), bank) == banks.end())
	{
		Button_SetCheck(m_ButtonBank0, BST_UNCHECKED);
		Button_SetCheck(m_ButtonBank1, BST_UNCHECKED);
		Button_SetCheck(m_ButtonBankBoth, BST_CHECKED);
		RenderSheets();
	}

	const TileSheetView::TileRef selection = { bank, tile };
	m_SheetView.SetSelection(selection);
	m_SheetView.EnsureVisible(selection);
	UpdateInspector();
}

void TileWindow::UpdateTitle()
{
	if (m_TitleFilename == m_Application->CurrentFilename)
	{
		return;
	}

	m_TitleFilename = m_Application->CurrentFilename;

	// Show the ROM file name without its extension, e.g. "Tile Data - Pokemon Red (UE)".
	// CurrentFilename is in the ANSI code page, which path converts back.
	const std::wstring rom_name = std::filesystem::path(m_TitleFilename).stem().wstring();
	const std::wstring title = rom_name.empty() ? L"Tile Data" : L"Tile Data \x2014 " + rom_name;
	SetWindowText(m_Hwnd, title.c_str());
}

void TileWindow::CaptureSnapshot()
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
			m_Vram[bank].clear();
			continue;
		}

		m_Vram[bank].resize(TileData::BytesPerBank);
		for (int i = 0; i < TileData::BytesPerBank; ++i)
		{
			m_Vram[bank][i] = ppu->ReadVideoRam(static_cast<uint16_t>(0x8000 + i), static_cast<uint8_t>(bank));
		}
	}

	for (uint8_t palette = 0; palette < 8; ++palette)
	{
		for (uint8_t index = 0; index < 4; ++index)
		{
			m_BackgroundColours[palette][index] = ToSheetColour(display->GetColourFromBackgroundPalette(palette, index));
			m_ObjectColours[palette][index] = ToSheetColour(display->GetColourFromObjectPalette(palette, index));
		}
	}

	m_HasSnapshot = true;
}

std::vector<int> TileWindow::VisibleBanks() const
{
	if (!m_IsColour || Button_GetCheck(m_ButtonBank0) == BST_CHECKED)
	{
		return { 0 };
	}

	if (Button_GetCheck(m_ButtonBank1) == BST_CHECKED)
	{
		return { 1 };
	}

	return { 0, 1 };
}

void TileWindow::RenderSheets()
{
	if (!m_HasSnapshot)
	{
		m_SheetView.SetEmptyText(L"Load a game to see its tiles.");
		m_SheetView.SetSheets({});
		return;
	}

	// Transparent colour 0 is shown as a checkerboard
	const PaletteColours palette = SelectedPalette();
	const std::array<uint32_t, 4>& colours = palette.colours;
	const bool transparent = palette.transparent;

	std::vector<TileSheetView::Sheet> sheets;
	for (int bank : VisibleBanks())
	{
		if (m_Vram[bank].size() != TileData::BytesPerBank)
		{
			continue;
		}

		const std::vector<uint8_t> indices = TileData::DecodeSheet(m_Vram[bank].data());

		TileSheetView::Sheet sheet;
		sheet.bank = bank;
		sheet.pixels.resize(indices.size());
		for (int y = 0; y < TileData::SheetHeight; ++y)
		{
			for (int x = 0; x < TileData::SheetWidth; ++x)
			{
				const size_t i = static_cast<size_t>(y) * TileData::SheetWidth + x;
				if (transparent && indices[i] == 0)
				{
					sheet.pixels[i] = (((x >> 2) + (y >> 2)) & 1) ? 0xE6E6E6 : 0xFFFFFF;
				}
				else
				{
					sheet.pixels[i] = colours[indices[i]];
				}
			}
		}

		sheets.push_back(std::move(sheet));
	}

	m_SheetView.SetSheets(sheets);
	UpdateInspector();

	if ((IsWindowEnabled(m_ButtonExport) != FALSE) != m_HasSnapshot)
	{
		EnableWindow(m_ButtonExport, m_HasSnapshot);
	}
}

void TileWindow::OpenExportDialog()
{
	if (!m_HasSnapshot)
	{
		return;
	}

	// The dialog keeps its own copy, so the export matches its preview while the game runs on
	TileExportDialog::Source source;
	source.is_colour = m_IsColour;
	source.vram = m_Vram;
	source.palette = ComboBox_GetCurSel(m_ComboPalette);

	const int count = ComboBox_GetCount(m_ComboPalette);
	for (int i = 0; i < count; ++i)
	{
		std::wstring name(static_cast<size_t>(ComboBox_GetLBTextLen(m_ComboPalette, i)), L'\0');
		ComboBox_GetLBText(m_ComboPalette, i, name.data());

		const PaletteColours colours = PaletteFromItemData(ComboBox_GetItemData(m_ComboPalette, i));
		source.palettes.push_back({ name, colours.colours, colours.transparent });
	}

	const std::vector<int> banks = VisibleBanks();
	if (banks.size() == 2)
	{
		source.layout = TileExportDialog::Layout::SideBySide;
	}
	else
	{
		source.layout = banks[0] == 1 ? TileExportDialog::Layout::Bank1 : TileExportDialog::Layout::Bank0;
	}

	source.rom_name = std::filesystem::path(m_Application->CurrentFilename).stem().wstring();

	if (m_ExportDialog == nullptr)
	{
		m_ExportDialog = std::make_unique<TileExportDialog>();
	}

	m_ExportDialog->Show(m_Hwnd, source);
}

TileWindow::PaletteColours TileWindow::SelectedPalette() const
{
	return PaletteFromItemData(ComboBox_GetItemData(m_ComboPalette, ComboBox_GetCurSel(m_ComboPalette)));
}

TileWindow::PaletteColours TileWindow::PaletteFromItemData(LRESULT data) const
{
	const PaletteKind kind = static_cast<PaletteKind>((data >> 8) & 0xFF);
	const int index = static_cast<int>(data & 7);

	PaletteColours palette;
	palette.colours = { 0xFFFFFF, 0xAAAAAA, 0x555555, 0x000000 };
	if (kind == PaletteKind::Background)
	{
		palette.colours = m_BackgroundColours[index];
	}
	else if (kind == PaletteKind::Object)
	{
		palette.colours = m_ObjectColours[index];
		palette.transparent = true;
	}

	return palette;
}

void TileWindow::UpdateInspector()
{
	const TileSheetView::TileRef selection = m_SheetView.GetSelection();
	const bool has_tile = selection.IsValid() && m_HasSnapshot && m_Vram[selection.bank].size() == TileData::BytesPerBank;

	// Show the details only while a tile is selected
	if (has_tile != m_InspectorHasTile)
	{
		m_InspectorHasTile = has_tile;

		std::vector<HWND> details = { m_LabelTileSubtitle, m_TilePreview, m_LabelBytes, m_ButtonCopy, m_EditBytes };
		details.insert(details.end(), m_PropertyLabels.begin(), m_PropertyLabels.end());
		details.insert(details.end(), m_PropertyValues.begin(), m_PropertyValues.end());
		for (HWND control : details)
		{
			ShowWindow(control, has_tile ? SW_SHOWNA : SW_HIDE);
		}

		SetTextIfChanged(m_LabelTileHint, has_tile ? TileHint : NoTileHint);
		Layout();
	}

	if (!has_tile)
	{
		SetTextIfChanged(m_LabelTileHeading, L"No tile selected");
		return;
	}

	const int tile = selection.tile;
	const int id = tile & 0xFF;
	const int address = 0x8000 + tile * TileData::BytesPerTile;
	const int block = tile / 128;
	const int block_start = 0x8000 + block * 0x800;
	const int signed_id = id < 128 ? id : id - 256;

	SetTextIfChanged(m_LabelTileHeading, std::format(L"Tile ${:02X}", id));
	SetTextIfChanged(m_LabelTileSubtitle, m_IsColour
		? std::format(L"#{} of 384 \x00B7 bank {}", tile, selection.bank)
		: std::format(L"#{} of 384", tile));

	// Tiles $00-$FF from $8000 are used by objects, and by the background when LCDC.4 is set.
	// Tiles from $8800 are numbered -128 to 127 from $9000 and are used by the background when LCDC.4 is clear.
	SetTextIfChanged(m_PropertyValues[0], std::format(L"${:04X}\x2013${:04X}", address, address + TileData::BytesPerTile - 1));
	SetTextIfChanged(m_PropertyValues[1], m_IsColour ? std::to_wstring(selection.bank) : L"0 (DMG has one bank)");
	SetTextIfChanged(m_PropertyValues[2], std::format(L"{}  ${:04X}\x2013${:04X}", block, block_start, block_start + 0x7FF));
	SetTextIfChanged(m_PropertyValues[3], tile < 256 ? std::format(L"${:02X}", id) : L"\x2014  not reachable");
	SetTextIfChanged(m_PropertyValues[4], tile >= 128 ? std::format(L"${:02X} ({})", id, signed_id) : L"\x2014  not reachable");

	// Bytes, two rows of the tile per line
	const uint8_t* bytes = m_Vram[selection.bank].data() + tile * TileData::BytesPerTile;
	std::wstring text;
	for (int line = 0; line < 4; ++line)
	{
		const int offset = line * 4;
		text += std::format(L"+{:X}  {:02X} {:02X}    +{:X}  {:02X} {:02X}",
			offset, bytes[offset], bytes[offset + 1], offset + 2, bytes[offset + 2], bytes[offset + 3]);
		if (line < 3)
		{
			text += L"\r\n";
		}
	}
	SetTextIfChanged(m_EditBytes, text);

	// Preview pixels, with transparent pixels marked by the top byte
	const PaletteColours palette = SelectedPalette();
	const std::array<uint8_t, 64> indices = TileData::DecodeTile(bytes);
	std::array<uint32_t, 64> pixels = {};
	for (size_t i = 0; i < pixels.size(); ++i)
	{
		pixels[i] = (palette.transparent && indices[i] == 0) ? 0xFF000000 : palette.colours[indices[i]];
	}

	if (pixels != m_PreviewPixels || palette.colours != m_PreviewPalette.colours || palette.transparent != m_PreviewPalette.transparent)
	{
		m_PreviewPixels = pixels;
		m_PreviewPalette = palette;
		InvalidateRect(m_TilePreview, NULL, FALSE);
	}
}

void TileWindow::DrawPreview(const DRAWITEMSTRUCT* item)
{
	const RECT& rect = item->rcItem;
	const int width = rect.right - rect.left;
	const int height = rect.bottom - rect.top;

	// Draw off screen so the preview doesn't flicker when it changes every frame
	HDC hdc = CreateCompatibleDC(item->hDC);
	HBITMAP bitmap = CreateCompatibleBitmap(item->hDC, width, height);
	HGDIOBJ previous_bitmap = SelectObject(hdc, bitmap);

	FillColour(hdc, 0, 0, width, height, InspectorColour);

	// Pixels, transparent ones as a checkerboard
	for (int y = 0; y < 8; ++y)
	{
		for (int x = 0; x < 8; ++x)
		{
			const uint32_t pixel = m_PreviewPixels[y * 8 + x];
			const int px = x * PreviewPixelSize;
			const int py = y * PreviewPixelSize;
			if (pixel & 0xFF000000)
			{
				const int half = PreviewPixelSize / 2;
				FillColour(hdc, px, py, PreviewPixelSize, PreviewPixelSize, RGB(255, 255, 255));
				FillColour(hdc, px, py, half, half, RGB(230, 230, 230));
				FillColour(hdc, px + half, py + half, half, half, RGB(230, 230, 230));
			}
			else
			{
				FillColour(hdc, px, py, PreviewPixelSize, PreviewPixelSize, ToColorRef(pixel));
			}
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
	DrawTextW(hdc, L"Palette", -1, &label, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);

	SelectObject(hdc, m_MonoSmallFont);
	SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
	for (int i = 0; i < 4; ++i)
	{
		const int y = 24 + i * 28;
		const bool transparent = i == 0 && m_PreviewPalette.transparent;
		if (transparent)
		{
			FillColour(hdc, swatch_x, y, 20, 20, RGB(255, 255, 255));
			FillColour(hdc, swatch_x, y, 10, 10, RGB(230, 230, 230));
			FillColour(hdc, swatch_x + 10, y + 10, 10, 10, RGB(230, 230, 230));
		}
		else
		{
			FillColour(hdc, swatch_x, y, 20, 20, ToColorRef(m_PreviewPalette.colours[i]));
		}

		RECT swatch = { swatch_x, y, swatch_x + 20, y + 20 };
		SetDCBrushColor(hdc, RGB(200, 200, 200));
		FrameRect(hdc, &swatch, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));

		const std::wstring text = transparent
			? std::format(L"{}  clear", i)
			: std::format(L"{}  #{:06X}", i, m_PreviewPalette.colours[i] & 0xFFFFFF);
		RECT text_rect = { swatch_x + 26, y, width, y + 20 };
		DrawTextW(hdc, text.c_str(), -1, &text_rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	}

	BitBlt(item->hDC, rect.left, rect.top, width, height, hdc, 0, 0, SRCCOPY);

	SelectObject(hdc, previous_bitmap);
	DeleteObject(bitmap);
	DeleteDC(hdc);
}

void TileWindow::CopySelectedBytes()
{
	const TileSheetView::TileRef selection = m_SheetView.GetSelection();
	if (!m_InspectorHasTile || !selection.IsValid())
	{
		return;
	}

	// "3C 7E 42 42 ..."
	const uint8_t* bytes = m_Vram[selection.bank].data() + selection.tile * TileData::BytesPerTile;
	std::wstring text;
	for (int i = 0; i < TileData::BytesPerTile; ++i)
	{
		if (i > 0)
		{
			text += L" ";
		}

		text += std::format(L"{:02X}", bytes[i]);
	}

	if (!OpenClipboard(m_Hwnd))
	{
		return;
	}

	EmptyClipboard();
	const size_t size = (text.size() + 1) * sizeof(wchar_t);
	HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, size);
	if (memory != NULL)
	{
		memcpy(GlobalLock(memory), text.c_str(), size);
		GlobalUnlock(memory);
		if (SetClipboardData(CF_UNICODETEXT, memory) == NULL)
		{
			GlobalFree(memory);
		}
	}

	CloseClipboard();
}

void TileWindow::UpdateHoverStatus()
{
	const TileSheetView::TileRef hover = m_SheetView.GetHover();

	std::wstring text;
	if (hover.IsValid())
	{
		const int address = 0x8000 + hover.tile * TileData::BytesPerTile;
		text = m_IsColour
			? std::format(L"Bank {} \x00B7 tile ${:02X} \x00B7 ${:04X}", hover.bank, hover.tile & 0xFF, address)
			: std::format(L"Tile ${:02X} \x00B7 ${:04X}", hover.tile & 0xFF, address);
	}

	SendMessageW(m_StatusBar, SB_SETTEXTW, 1, reinterpret_cast<LPARAM>(text.c_str()));
}

bool TileWindow::HandleDialogMessage(MSG* msg)
{
	if (m_ExportDialog != nullptr && m_ExportDialog->HandleDialogMessage(msg))
	{
		return true;
	}

	return m_Hwnd != NULL && IsDialogMessage(m_Hwnd, msg);
}

void TileWindow::WindowCreate(int width, int height)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);

	WNDCLASS wc = {};
	wc.style = CS_VREDRAW | CS_HREDRAW;
	wc.lpfnWndProc = TileWndProc;
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

	m_Hwnd = CreateWindowEx(exStyle, ClassName, L"Tile Data", dwStyle, x, y, window_width, window_height, owner, NULL, hInstance, this);
	if (m_Hwnd == NULL)
	{
		throw std::exception("CreateWindow Failed");
	}
}

void TileWindow::CreateControls()
{
	auto create = [this](const wchar_t* type, const wchar_t* text, DWORD style, int id, HFONT font)
	{
		HWND control = CreateWindowW(type, text, WS_CHILD | WS_VISIBLE | style, 0, 0, 0, 0, m_Hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), nullptr, nullptr);
		SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
		return control;
	};

	// VRAM bank: push-like radio buttons act as a segmented control
	m_LabelBank = create(L"STATIC", L"VRAM bank", SS_CENTERIMAGE, 0, m_Font);
	m_ButtonBank0 = create(L"BUTTON", L"Bank 0", WS_TABSTOP | WS_GROUP | BS_AUTORADIOBUTTON | BS_PUSHLIKE, m_ControlBank0Id, m_Font);
	m_ButtonBank1 = create(L"BUTTON", L"Bank 1", BS_AUTORADIOBUTTON | BS_PUSHLIKE, m_ControlBank1Id, m_Font);
	m_ButtonBankBoth = create(L"BUTTON", L"Both", BS_AUTORADIOBUTTON | BS_PUSHLIKE, m_ControlBankBothId, m_Font);

	// Palette
	m_LabelPalette = create(L"STATIC", L"Palette", SS_CENTERIMAGE | WS_GROUP, 0, m_Font);
	m_ComboPalette = create(WC_COMBOBOX, L"", WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST, m_ControlPaletteId, m_Font);

	// Zoom
	m_LabelZoom = create(L"STATIC", L"Zoom", SS_CENTERIMAGE, 0, m_Font);
	m_ComboZoom = create(WC_COMBOBOX, L"", WS_TABSTOP | CBS_DROPDOWNLIST, m_ControlZoomId, m_Font);
	for (int zoom : { 2, 3, 4, 6 })
	{
		const std::wstring text = std::to_wstring(zoom) + L"\x00D7";
		const int index = ComboBox_AddString(m_ComboZoom, text.c_str());
		ComboBox_SetItemData(m_ComboZoom, index, zoom);
	}
	ComboBox_SetCurSel(m_ComboZoom, 1);

	// Grid
	m_CheckGrid = create(L"BUTTON", L"Grid", WS_TABSTOP | BS_AUTOCHECKBOX, m_ControlGridId, m_Font);
	Button_SetCheck(m_CheckGrid, BST_CHECKED);

	// Pause updates and export, on the right
	m_ButtonPause = create(L"BUTTON", L"Pause updates", WS_TABSTOP | BS_AUTOCHECKBOX | BS_PUSHLIKE, m_ControlPauseId, m_Font);
	m_ButtonExport = create(L"BUTTON", L"Export PNG\x2026", WS_TABSTOP | BS_PUSHBUTTON | WS_DISABLED, m_ControlExportId, m_Font);

	// Tile sheets
	m_SheetView.Create(m_Hwnd, m_ControlSheetViewId);
	m_SheetView.SetZoom(static_cast<int>(ComboBox_GetItemData(m_ComboZoom, ComboBox_GetCurSel(m_ComboZoom))));
	m_SheetView.SetGrid(true);
	m_SheetView.HoverChanged = [this]() { UpdateHoverStatus(); };
	m_SheetView.SelectionChanged = [this]() { UpdateInspector(); };
	m_SheetView.CopyRequested = [this]() { CopySelectedBytes(); };

	// Inspector, with the details hidden until a tile is selected
	m_LabelTileHeading = create(L"STATIC", L"No tile selected", SS_LEFT | SS_ENDELLIPSIS, 0, m_HeadingFont);
	m_LabelTileSubtitle = create(L"STATIC", L"", SS_LEFT | SS_ENDELLIPSIS, 0, m_Font);
	m_TilePreview = create(L"STATIC", L"", SS_OWNERDRAW, m_ControlPreviewId, m_Font);

	const wchar_t* property_names[] = { L"Address", L"Bank", L"Block", L"ID, $8000 mode", L"ID, $8800 mode" };
	for (size_t i = 0; i < m_PropertyLabels.size(); ++i)
	{
		m_PropertyLabels[i] = create(L"STATIC", property_names[i], SS_LEFT, 0, m_Font);
		m_PropertyValues[i] = create(L"STATIC", L"", SS_LEFT | SS_ENDELLIPSIS, 0, m_MonoFont);
	}

	m_LabelBytes = create(L"STATIC", L"Bytes", SS_LEFT | SS_CENTERIMAGE, 0, m_BoldFont);
	m_ButtonCopy = create(L"BUTTON", L"Copy", WS_TABSTOP | BS_PUSHBUTTON, m_ControlCopyId, m_Font);
	m_EditBytes = create(L"EDIT", L"", WS_TABSTOP | WS_BORDER | ES_MULTILINE | ES_READONLY, m_ControlBytesId, m_MonoFont);

	m_LabelTileHint = create(L"STATIC", NoTileHint, SS_LEFT, 0, m_Font);

	std::vector<HWND> details = { m_LabelTileSubtitle, m_TilePreview, m_LabelBytes, m_ButtonCopy, m_EditBytes };
	details.insert(details.end(), m_PropertyLabels.begin(), m_PropertyLabels.end());
	details.insert(details.end(), m_PropertyValues.begin(), m_PropertyValues.end());
	for (HWND control : details)
	{
		ShowWindow(control, SW_HIDE);
	}
	m_InspectorHasTile = false;

	// Status bar
	m_StatusBar = CreateWindowExW(0, STATUSCLASSNAME, L"", WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
		0, 0, 0, 0, m_Hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(m_ControlStatusBarId)), nullptr, nullptr);
	SendMessageW(m_StatusBar, WM_SETFONT, reinterpret_cast<WPARAM>(m_Font), TRUE);
}

void TileWindow::ApplyMode()
{
	// DMG only has bank 0
	EnableWindow(m_ButtonBank1, m_IsColour);
	EnableWindow(m_ButtonBankBoth, m_IsColour);

	// Bank 0 for DMG, both banks for CGB
	Button_SetCheck(m_ButtonBank0, m_IsColour ? BST_UNCHECKED : BST_CHECKED);
	Button_SetCheck(m_ButtonBank1, BST_UNCHECKED);
	Button_SetCheck(m_ButtonBankBoth, m_IsColour ? BST_CHECKED : BST_UNCHECKED);

	// Palette list
	ComboBox_ResetContent(m_ComboPalette);

	auto add = [this](const std::wstring& text, PaletteKind kind, int index)
	{
		const int item = ComboBox_AddString(m_ComboPalette, text.c_str());
		ComboBox_SetItemData(m_ComboPalette, item, PaletteItemData(kind, index));
	};

	if (m_IsColour)
	{
		for (int i = 0; i < 8; ++i) add(L"BG " + std::to_wstring(i), PaletteKind::Background, i);
		for (int i = 0; i < 8; ++i) add(L"OBJ " + std::to_wstring(i), PaletteKind::Object, i);
	}
	else
	{
		add(L"BGP", PaletteKind::Background, 0);
		add(L"OBP0", PaletteKind::Object, 0);
		add(L"OBP1", PaletteKind::Object, 1);
	}

	add(L"Greyscale (raw 0\x2013" L"3)", PaletteKind::Greyscale, 0);
	ComboBox_SetCurSel(m_ComboPalette, 0);

	// A different game, so the selected tile no longer means anything
	m_SheetView.SetSelection({});
	UpdateInspector();
}

void TileWindow::UpdateStatus()
{
	Emulator* emulator = m_Application->GetEmulator();
	const bool running = emulator != nullptr && emulator->IsRunning();
	const bool paused = Button_GetCheck(m_ButtonPause) == BST_CHECKED;

	std::wstring text;
	if (!running)
	{
		text = L"No game loaded";
	}
	else
	{
		text = m_IsColour ? L"CGB \x00B7 768 tiles in 2 banks" : L"DMG \x00B7 384 tiles";
		text += paused ? L" \x00B7 Paused, showing a snapshot" : L" \x00B7 Live, updates every frame";
	}

	// Only set when changed, as this is called every frame
	wchar_t current[128] = {};
	SendMessageW(m_StatusBar, SB_GETTEXTW, 0, reinterpret_cast<LPARAM>(current));
	if (text != current)
	{
		SendMessageW(m_StatusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(text.c_str()));
	}
}

void TileWindow::Layout()
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

	label(m_LabelBank);
	for (HWND button : { m_ButtonBank0, m_ButtonBank1, m_ButtonBankBoth })
	{
		const int w = MeasureText(m_Font, GetText(button)) + 24;
		place(button, x, button_y, w, button_height);
		x += w + 2;
	}

	x += group_gap;
	label(m_LabelPalette);
	place(m_ComboPalette, x, combo_y, 168, 400);
	x += 168 + group_gap;

	label(m_LabelZoom);
	place(m_ComboZoom, x, combo_y, 64, 200);
	x += 64 + group_gap;

	place(m_CheckGrid, x, button_y, MeasureText(m_Font, GetText(m_CheckGrid)) + 24, button_height);

	const int export_width = MeasureText(m_Font, GetText(m_ButtonExport)) + 28;
	const int pause_width = MeasureText(m_Font, GetText(m_ButtonPause)) + 28;
	place(m_ButtonExport, width - 12 - export_width, button_y, export_width, button_height);
	place(m_ButtonPause, width - 12 - export_width - 8 - pause_width, button_y, pause_width, button_height);

	// Inspector
	const int inspector_x = width - m_InspectorWidth + 1 + 16;
	const int inspector_width = m_InspectorWidth - 1 - 32;
	const int body_top = m_CommandBarHeight + 1;
	int y = body_top + 16;
	place(m_LabelTileHeading, inspector_x, y, inspector_width, 28);
	y += 30;

	if (m_InspectorHasTile)
	{
		place(m_LabelTileSubtitle, inspector_x, y, inspector_width, 20);
		y += 28;

		place(m_TilePreview, inspector_x, y, inspector_width, PreviewHeight);
		y += PreviewHeight + 16;

		const int label_width = 104;
		for (size_t i = 0; i < m_PropertyLabels.size(); ++i)
		{
			place(m_PropertyLabels[i], inspector_x, y, label_width, 20);
			place(m_PropertyValues[i], inspector_x + label_width + 4, y, inspector_width - label_width - 4, 20);
			y += 22;
		}
		y += 12;

		place(m_LabelBytes, inspector_x, y, inspector_width - 72, 26);
		place(m_ButtonCopy, inspector_x + inspector_width - 64, y + 1, 64, 24);
		y += 30;

		place(m_EditBytes, inspector_x, y, inspector_width, 78);
		y += 78 + 12;
	}

	place(m_LabelTileHint, inspector_x, y, inspector_width, 48);

	// Tile sheets fill the rest
	const int body_height = client.bottom - status_height - body_top;
	place(m_SheetView.GetHwnd(), 0, body_top, width - m_InspectorWidth, body_height > 0 ? body_height : 0);

	EndDeferWindowPos(defer);

	InvalidateRect(m_Hwnd, NULL, TRUE);
}

void TileWindow::PaintBackground(HDC hdc)
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

	// White tile sheet area
	RECT sheets = client;
	sheets.top = divider.bottom;
	sheets.right = client.right - m_InspectorWidth;
	FillRect(hdc, &sheets, GetSysColorBrush(COLOR_WINDOW));

	// 1px divider and the inspector
	RECT inspector_divider = sheets;
	inspector_divider.left = sheets.right;
	inspector_divider.right = inspector_divider.left + 1;
	SetDCBrushColor(hdc, DividerColour);
	FillRect(hdc, &inspector_divider, dc_brush);

	RECT inspector = sheets;
	inspector.left = inspector_divider.right;
	inspector.right = client.right;
	FillRect(hdc, &inspector, m_InspectorBrush);
}

LRESULT TileWindow::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
				m_Application->GetMainWindow()->ToggleTileWindowMenuItem(false);
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
				SetFocus(m_LastFocus != NULL ? m_LastFocus : m_ButtonBank0);
			}

			return 0;
		}

		case WM_CTLCOLORSTATIC:
		{
			HDC hdc = reinterpret_cast<HDC>(wParam);
			HWND control = reinterpret_cast<HWND>(lParam);
			SetBkMode(hdc, TRANSPARENT);

			if (control == m_EditBytes)
			{
				// Read-only edits are grey by default
				SetBkMode(hdc, OPAQUE);
				SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
				SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
				return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_WINDOW));
			}

			const bool secondary = control == m_LabelTileHint || control == m_LabelTileSubtitle ||
				std::find(m_PropertyLabels.begin(), m_PropertyLabels.end(), control) != m_PropertyLabels.end();
			const bool inspector = secondary || control == m_LabelTileHeading || control == m_LabelBytes ||
				std::find(m_PropertyValues.begin(), m_PropertyValues.end(), control) != m_PropertyValues.end();

			if (inspector)
			{
				SetTextColor(hdc, secondary ? SecondaryTextColour : GetSysColor(COLOR_WINDOWTEXT));
				return reinterpret_cast<LRESULT>(m_InspectorBrush);
			}

			SetTextColor(hdc, control == m_CheckGrid ? GetSysColor(COLOR_WINDOWTEXT) : SecondaryTextColour);
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

			if ((id == m_ControlBank0Id || id == m_ControlBank1Id || id == m_ControlBankBothId) && code == BN_CLICKED)
			{
				RenderSheets();
				return 0;
			}

			if (id == m_ControlPaletteId && code == CBN_SELCHANGE)
			{
				RenderSheets();
				return 0;
			}

			if (id == m_ControlZoomId && code == CBN_SELCHANGE)
			{
				m_SheetView.SetZoom(static_cast<int>(ComboBox_GetItemData(m_ComboZoom, ComboBox_GetCurSel(m_ComboZoom))));
				return 0;
			}

			if (id == m_ControlGridId && code == BN_CLICKED)
			{
				m_SheetView.SetGrid(Button_GetCheck(m_CheckGrid) == BST_CHECKED);
				return 0;
			}

			if (id == m_ControlCopyId && code == BN_CLICKED)
			{
				CopySelectedBytes();
				return 0;
			}

			if (id == m_ControlExportId && code == BN_CLICKED)
			{
				OpenExportDialog();
				return 0;
			}

			break;
		}
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}
