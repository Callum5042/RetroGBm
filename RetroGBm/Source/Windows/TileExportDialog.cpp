#include "Windows/TileExportDialog.h"

#include <CommCtrl.h>
#include <ShObjIdl.h>
#include <windowsx.h>
#include <wrl/client.h>

// The implementation is compiled in MainWindow.cpp
#include "stb_image_write.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>

namespace
{
	const wchar_t* ClassName = L"RetroGBmTileExport";
	const wchar_t* Title = L"Export Tile Data";

	static LRESULT CALLBACK ExportWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		TileExportDialog* dialog = nullptr;
		if (msg == WM_NCCREATE)
		{
			CREATESTRUCT* create = reinterpret_cast<CREATESTRUCT*>(lParam);
			dialog = reinterpret_cast<TileExportDialog*>(create->lpCreateParams);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(dialog));
		}
		else
		{
			dialog = reinterpret_cast<TileExportDialog*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		}

		if (dialog == nullptr)
		{
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}

		return dialog->HandleMessage(hwnd, msg, wParam, lParam);
	}

	void FillColour(HDC hdc, int x, int y, int width, int height, COLORREF colour)
	{
		RECT rect = { x, y, x + width, y + height };
		SetDCBrushColor(hdc, colour);
		FillRect(hdc, &rect, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
	}

	// 0x00RRGGBB to the R, G, B, A byte order PNG and stb want (0xAABBGGRR)
	uint32_t ToRgba(uint32_t colour)
	{
		return 0xFF000000 | ((colour >> 16) & 0xFF) | (colour & 0xFF00) | ((colour & 0xFF) << 16);
	}

	// 0xAABBGGRR to the 0x00RRGGBB a DIB wants
	uint32_t ToDib(uint32_t rgba)
	{
		return ((rgba & 0xFF) << 16) | (rgba & 0xFF00) | ((rgba >> 16) & 0xFF);
	}

	const int Scales[] = { 1, 2, 4, 8 };
	const uint32_t GridColour = 0xFF000000;
	const COLORREF DividerColour = RGB(223, 223, 223);
	const COLORREF SecondaryTextColour = RGB(92, 92, 92);
}

TileExportDialog::~TileExportDialog()
{
	Close();
	UnregisterClass(ClassName, GetModuleHandle(NULL));
	ReleaseFonts();
}

void TileExportDialog::Show(HWND owner, const Source& source)
{
	if (IsOpen())
	{
		SetForegroundWindow(m_Hwnd);
		return;
	}

	m_Owner = owner;
	m_Source = source;

	ReleaseFonts();
	{
		NONCLIENTMETRICS metrics = { sizeof(NONCLIENTMETRICS) };
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0);
		m_Font = CreateFontIndirect(&metrics.lfMessageFont);

		LOGFONT bold = metrics.lfMessageFont;
		bold.lfWeight = FW_SEMIBOLD;
		m_BoldFont = CreateFontIndirect(&bold);

		HDC hdc = GetDC(NULL);
		LOGFONT mono = {};
		mono.lfHeight = -MulDiv(9, GetDeviceCaps(hdc, LOGPIXELSY), 72);
		mono.lfWeight = FW_NORMAL;
		mono.lfQuality = CLEARTYPE_QUALITY;
		mono.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
		wcscpy_s(mono.lfFaceName, L"Consolas");
		m_MonoFont = CreateFontIndirect(&mono);
		ReleaseDC(NULL, hdc);
	}

	HINSTANCE hInstance = GetModuleHandle(NULL);
	WNDCLASS wc = {};
	wc.lpfnWndProc = ExportWndProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.lpszClassName = ClassName;
	RegisterClass(&wc);

	const DWORD exStyle = WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT;
	const DWORD dwStyle = WS_CAPTION | WS_POPUP | WS_SYSMENU;

	RECT window_rect = { 0, 0, m_ClientWidth, m_ClientHeight };
	AdjustWindowRectEx(&window_rect, dwStyle, FALSE, exStyle);
	const int window_width = window_rect.right - window_rect.left;
	const int window_height = window_rect.bottom - window_rect.top;

	// Centre over the tile window, kept inside the monitor's work area
	RECT owner_rect;
	GetWindowRect(owner, &owner_rect);
	int x = owner_rect.left + ((owner_rect.right - owner_rect.left) - window_width) / 2;
	int y = owner_rect.top + ((owner_rect.bottom - owner_rect.top) - window_height) / 2;

	MONITORINFO monitor = { sizeof(MONITORINFO) };
	if (GetMonitorInfo(MonitorFromWindow(owner, MONITOR_DEFAULTTONEAREST), &monitor))
	{
		const RECT& work = monitor.rcWork;
		x = std::clamp(x, static_cast<int>(work.left), (std::max)(static_cast<int>(work.left), static_cast<int>(work.right) - window_width));
		y = std::clamp(y, static_cast<int>(work.top), (std::max)(static_cast<int>(work.top), static_cast<int>(work.bottom) - window_height));
	}

	m_Hwnd = CreateWindowEx(exStyle, ClassName, Title, dwStyle, x, y, window_width, window_height, owner, NULL, hInstance, this);
	if (m_Hwnd == NULL)
	{
		throw std::exception("CreateWindow Failed");
	}

	CreateControls();
	Rebuild();

	// Modal to the tile window
	EnableWindow(m_Owner, FALSE);
	ShowWindow(m_Hwnd, SW_SHOWNORMAL);
	SetFocus(m_ButtonSave);
}

void TileExportDialog::Close()
{
	if (m_Hwnd == NULL)
	{
		return;
	}

	// Enable the owner first so it is activated when this window goes
	if (m_Owner != NULL)
	{
		EnableWindow(m_Owner, TRUE);
	}

	DestroyWindow(m_Hwnd);
}

void TileExportDialog::ReleaseFonts()
{
	for (HFONT* font : { &m_Font, &m_BoldFont, &m_MonoFont })
	{
		if (*font != NULL)
		{
			DeleteObject(*font);
			*font = NULL;
		}
	}
}

bool TileExportDialog::HandleDialogMessage(MSG* msg)
{
	return m_Hwnd != NULL && IsDialogMessage(m_Hwnd, msg);
}

void TileExportDialog::CreateControls()
{
	auto create = [this](const wchar_t* type, const wchar_t* text, DWORD style, int id, HFONT font, int x, int y, int w, int h)
	{
		HWND control = CreateWindowW(type, text, WS_CHILD | WS_VISIBLE | style, x, y, w, h, m_Hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), nullptr, nullptr);
		SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
		return control;
	};

	const int left = 16;
	const int right = 260;
	const int right_width = m_ClientWidth - 16 - right;

	// What to export
	m_LabelSource = create(L"STATIC", L"What to export", SS_LEFT, 0, m_BoldFont, left, 16, 220, 18);
	const wchar_t* layouts[] = { L"Bank 0 only", L"Bank 1 only", L"Both banks, side by side", L"Both banks, stacked" };
	for (int i = 0; i < 4; ++i)
	{
		const DWORD style = BS_AUTORADIOBUTTON | (i == 0 ? WS_GROUP | WS_TABSTOP : 0);
		m_RadioLayout[i] = create(L"BUTTON", layouts[i], style, m_ControlLayoutId + i, m_Font, left, 40 + i * 24, 220, 22);

		// DMG has one bank
		if (i > 0 && !m_Source.is_colour)
		{
			EnableWindow(m_RadioLayout[i], FALSE);
		}
	}

	const Layout layout = m_Source.is_colour ? m_Source.layout : Layout::Bank0;
	Button_SetCheck(m_RadioLayout[static_cast<int>(layout)], BST_CHECKED);

	// Palette, starting on the viewer's
	m_LabelPalette = create(L"STATIC", L"Palette", SS_LEFT | WS_GROUP, 0, m_BoldFont, right, 16, right_width, 18);
	m_ComboPalette = create(WC_COMBOBOX, L"", WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST, m_ControlPaletteId, m_Font, right, 38, right_width, 320);
	for (const Palette& palette : m_Source.palettes)
	{
		ComboBox_AddString(m_ComboPalette, palette.name.c_str());
	}
	ComboBox_SetCurSel(m_ComboPalette, m_Source.palette);

	// Scale: push-like radio buttons act as a segmented control
	m_LabelScale = create(L"STATIC", L"Scale", SS_LEFT, 0, m_BoldFont, right, 74, right_width, 18);
	for (int i = 0; i < 4; ++i)
	{
		const DWORD style = BS_AUTORADIOBUTTON | BS_PUSHLIKE | (i == 0 ? WS_GROUP | WS_TABSTOP : 0);
		const std::wstring text = std::to_wstring(Scales[i]) + L"\x00D7";
		m_RadioScale[i] = create(L"BUTTON", text.c_str(), style, m_ControlScaleId + i, m_Font, right + i * 50, 96, 48, 26);
	}
	Button_SetCheck(m_RadioScale[1], BST_CHECKED);

	// Options. Colour 0 starts transparent for object palettes.
	m_CheckGrid = create(L"BUTTON", L"Grid lines between tiles", WS_GROUP | WS_TABSTOP | BS_AUTOCHECKBOX, m_ControlGridId, m_Font, left, 148, 220, 22);
	m_CheckTransparent = create(L"BUTTON", L"Colour 0 transparent", WS_TABSTOP | BS_AUTOCHECKBOX, m_ControlTransparentId, m_Font, right, 148, right_width, 22);
	const bool object = m_Source.palette >= 0 && m_Source.palette < static_cast<int>(m_Source.palettes.size()) && m_Source.palettes[m_Source.palette].object;
	Button_SetCheck(m_CheckTransparent, object ? BST_CHECKED : BST_UNCHECKED);

	// Preview with the output size
	m_LabelPreview = create(L"STATIC", L"Preview", SS_LEFT, 0, m_BoldFont, left, 186, 200, 18);
	m_LabelSize = create(L"STATIC", L"", SS_RIGHT, 0, m_MonoFont, right, 186, right_width, 18);
	m_Preview = create(L"STATIC", L"", SS_OWNERDRAW, m_ControlPreviewId, m_Font, left, 208, m_ClientWidth - 32, 264);

	// Footer
	const int footer_top = m_ClientHeight - m_FooterHeight;
	const int button_y = footer_top + (m_FooterHeight - 26) / 2;
	m_ButtonCopy = create(L"BUTTON", L"Copy to clipboard", WS_TABSTOP | BS_PUSHBUTTON, m_ControlCopyId, m_Font, left, button_y, 130, 26);
	m_ButtonCancel = create(L"BUTTON", L"Cancel", WS_TABSTOP | BS_PUSHBUTTON, IDCANCEL, m_Font, m_ClientWidth - 16 - 100 - 8 - 88, button_y, 88, 26);
	m_ButtonSave = create(L"BUTTON", L"Save PNG\x2026", WS_TABSTOP | BS_DEFPUSHBUTTON, IDOK, m_Font, m_ClientWidth - 16 - 100, button_y, 100, 26);
}

TileExportDialog::Layout TileExportDialog::SelectedLayout() const
{
	for (int i = 0; i < 4; ++i)
	{
		if (Button_GetCheck(m_RadioLayout[i]) == BST_CHECKED)
		{
			return static_cast<Layout>(i);
		}
	}

	return Layout::Bank0;
}

int TileExportDialog::SelectedScale() const
{
	for (int i = 0; i < 4; ++i)
	{
		if (Button_GetCheck(m_RadioScale[i]) == BST_CHECKED)
		{
			return Scales[i];
		}
	}

	return 1;
}

void TileExportDialog::Rebuild()
{
	const Layout layout = SelectedLayout();
	std::vector<int> banks;
	switch (layout)
	{
		case Layout::Bank0: banks = { 0 }; break;
		case Layout::Bank1: banks = { 1 }; break;
		default: banks = { 0, 1 }; break;
	}

	const int palette_index = ComboBox_GetCurSel(m_ComboPalette);
	Palette palette;
	if (palette_index >= 0 && palette_index < static_cast<int>(m_Source.palettes.size()))
	{
		palette = m_Source.palettes[palette_index];
	}

	const bool transparent = Button_GetCheck(m_CheckTransparent) == BST_CHECKED;

	std::vector<std::vector<uint32_t>> sheets;
	for (int bank : banks)
	{
		if (m_Source.vram[bank].size() != TileData::BytesPerBank)
		{
			continue;
		}

		const std::vector<uint8_t> indices = TileData::DecodeSheet(m_Source.vram[bank].data());
		std::vector<uint32_t> pixels(indices.size());
		for (size_t i = 0; i < indices.size(); ++i)
		{
			pixels[i] = (transparent && indices[i] == 0) ? 0x00000000 : ToRgba(palette.colours[indices[i]]);
		}

		sheets.push_back(std::move(pixels));
	}

	const TileData::SheetLayout sheet_layout = layout == Layout::Stacked ? TileData::SheetLayout::Stacked : TileData::SheetLayout::SideBySide;
	const bool grid = Button_GetCheck(m_CheckGrid) == BST_CHECKED;
	m_Image = TileData::ComposeSheets(sheets, sheet_layout, SelectedScale(), grid, GridColour);

	const std::wstring size = std::format(L"{} \x00D7 {} px \x00B7 {} tiles", m_Image.width, m_Image.height, sheets.size() * TileData::TilesPerBank);
	SetWindowText(m_LabelSize, size.c_str());

	EnableWindow(m_ButtonSave, !m_Image.pixels.empty());
	EnableWindow(m_ButtonCopy, !m_Image.pixels.empty());
	InvalidateRect(m_Preview, NULL, FALSE);
}

void TileExportDialog::DrawPreview(const DRAWITEMSTRUCT* item)
{
	const RECT& rect = item->rcItem;
	const int width = rect.right - rect.left;
	const int height = rect.bottom - rect.top;

	HDC hdc = CreateCompatibleDC(item->hDC);
	HBITMAP bitmap = CreateCompatibleBitmap(item->hDC, width, height);
	HGDIOBJ previous_bitmap = SelectObject(hdc, bitmap);

	FillColour(hdc, 0, 0, width, height, GetSysColor(COLOR_WINDOW));

	if (!m_Image.pixels.empty())
	{
		// Fit inside the box, at a whole number scale when it's smaller so pixels stay square
		const int margin = 12;
		const double fit = (std::min)(static_cast<double>(width - margin * 2) / m_Image.width, static_cast<double>(height - margin * 2) / m_Image.height);
		const double scale = fit >= 1.0 ? static_cast<int>(fit) : fit;
		const int draw_width = (std::max)(1, static_cast<int>(m_Image.width * scale));
		const int draw_height = (std::max)(1, static_cast<int>(m_Image.height * scale));
		const int draw_x = (width - draw_width) / 2;
		const int draw_y = (height - draw_height) / 2;

		// Transparent pixels as a checkerboard of 4 x 4 image pixels
		std::vector<uint32_t> pixels(m_Image.pixels.size());
		for (int y = 0; y < m_Image.height; ++y)
		{
			for (int x = 0; x < m_Image.width; ++x)
			{
				const size_t i = static_cast<size_t>(y) * m_Image.width + x;
				const uint32_t rgba = m_Image.pixels[i];
				pixels[i] = (rgba >> 24) == 0 ? ((((x >> 2) + (y >> 2)) & 1) ? 0xE6E6E6 : 0xFFFFFF) : ToDib(rgba);
			}
		}

		BITMAPINFO info = {};
		info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		info.bmiHeader.biWidth = m_Image.width;
		info.bmiHeader.biHeight = -m_Image.height;
		info.bmiHeader.biPlanes = 1;
		info.bmiHeader.biBitCount = 32;
		info.bmiHeader.biCompression = BI_RGB;

		SetStretchBltMode(hdc, scale >= 1.0 ? COLORONCOLOR : HALFTONE);
		SetBrushOrgEx(hdc, 0, 0, NULL);
		StretchDIBits(hdc, draw_x, draw_y, draw_width, draw_height, 0, 0, m_Image.width, m_Image.height,
			pixels.data(), &info, DIB_RGB_COLORS, SRCCOPY);

		RECT outline = { draw_x - 1, draw_y - 1, draw_x + draw_width + 1, draw_y + draw_height + 1 };
		SetDCBrushColor(hdc, RGB(200, 200, 200));
		FrameRect(hdc, &outline, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
	}

	RECT border = { 0, 0, width, height };
	SetDCBrushColor(hdc, DividerColour);
	FrameRect(hdc, &border, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));

	BitBlt(item->hDC, rect.left, rect.top, width, height, hdc, 0, 0, SRCCOPY);

	SelectObject(hdc, previous_bitmap);
	DeleteObject(bitmap);
	DeleteDC(hdc);
}

std::vector<uint8_t> TileExportDialog::EncodePng() const
{
	std::vector<uint8_t> png;
	if (m_Image.pixels.empty())
	{
		return png;
	}

	auto write = [](void* context, void* data, int size)
	{
		std::vector<uint8_t>* output = static_cast<std::vector<uint8_t>*>(context);
		const uint8_t* bytes = static_cast<const uint8_t*>(data);
		output->insert(output->end(), bytes, bytes + size);
	};

	stbi_write_png_to_func(write, &png, m_Image.width, m_Image.height, 4, m_Image.pixels.data(), m_Image.width * 4);
	return png;
}

void TileExportDialog::Save()
{
	const HRESULT initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(initialized) && initialized != RPC_E_CHANGED_MODE)
	{
		MessageBoxW(m_Hwnd, L"Could not open the save dialog.", Title, MB_OK | MB_ICONERROR);
		return;
	}

	bool saved = false;

	// Release the dialog and shell item before balancing COM initialization
	{
		Microsoft::WRL::ComPtr<IFileSaveDialog> dialog;
		HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&dialog));
		if (SUCCEEDED(hr))
		{
			const COMDLG_FILTERSPEC filters[] = { { L"PNG image", L"*.png" } };
			const std::wstring filename = (m_Source.rom_name.empty() ? std::wstring(L"Tile data") : m_Source.rom_name + L" tiles") + L".png";
			hr = dialog->SetFileTypes(1, filters);
			if (SUCCEEDED(hr)) hr = dialog->SetDefaultExtension(L"png");
			if (SUCCEEDED(hr)) hr = dialog->SetFileName(filename.c_str());
			if (SUCCEEDED(hr)) hr = dialog->SetTitle(L"Save Tile Data");
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
				const std::vector<uint8_t> png = EncodePng();

				std::ofstream output(std::filesystem::path(path), std::ios::binary | std::ios::trunc);
				output.write(reinterpret_cast<const char*>(png.data()), static_cast<std::streamsize>(png.size()));
				output.close();

				if (png.empty() || !output)
				{
					MessageBoxW(m_Hwnd, L"Could not save the image.", Title, MB_OK | MB_ICONERROR);
				}
				else
				{
					saved = true;
				}
			}
			CoTaskMemFree(path);
		}

		if (FAILED(hr) && hr != HRESULT_FROM_WIN32(ERROR_CANCELLED))
		{
			MessageBoxW(m_Hwnd, L"Could not choose a file to save to.", Title, MB_OK | MB_ICONERROR);
		}
	}

	if (SUCCEEDED(initialized))
	{
		CoUninitialize();
	}

	if (saved)
	{
		Close();
	}
}

void TileExportDialog::CopyToClipboard()
{
	if (m_Image.pixels.empty() || !OpenClipboard(m_Hwnd))
	{
		return;
	}

	EmptyClipboard();

	// A bottom-up 32-bit DIB for most apps
	{
		const size_t row_bytes = static_cast<size_t>(m_Image.width) * 4;
		const size_t size = sizeof(BITMAPINFOHEADER) + row_bytes * m_Image.height;
		HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, size);
		if (memory != NULL)
		{
			uint8_t* data = static_cast<uint8_t*>(GlobalLock(memory));

			BITMAPINFOHEADER header = {};
			header.biSize = sizeof(BITMAPINFOHEADER);
			header.biWidth = m_Image.width;
			header.biHeight = m_Image.height;
			header.biPlanes = 1;
			header.biBitCount = 32;
			header.biCompression = BI_RGB;
			std::memcpy(data, &header, sizeof(header));

			uint32_t* pixels = reinterpret_cast<uint32_t*>(data + sizeof(header));
			for (int y = 0; y < m_Image.height; ++y)
			{
				const uint32_t* source = m_Image.pixels.data() + static_cast<size_t>(m_Image.height - 1 - y) * m_Image.width;
				for (int x = 0; x < m_Image.width; ++x)
				{
					pixels[static_cast<size_t>(y) * m_Image.width + x] = ToDib(source[x]) | (source[x] & 0xFF000000);
				}
			}

			GlobalUnlock(memory);
			if (SetClipboardData(CF_DIB, memory) == NULL)
			{
				GlobalFree(memory);
			}
		}
	}

	// And the PNG, which keeps transparency, for apps that read it
	{
		const std::vector<uint8_t> png = EncodePng();
		const UINT format = RegisterClipboardFormatW(L"PNG");
		HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, png.size());
		if (memory != NULL && format != 0 && !png.empty())
		{
			std::memcpy(GlobalLock(memory), png.data(), png.size());
			GlobalUnlock(memory);
			if (SetClipboardData(format, memory) == NULL)
			{
				GlobalFree(memory);
			}
		}
		else if (memory != NULL)
		{
			GlobalFree(memory);
		}
	}

	CloseClipboard();

	// Say so on the button for a moment
	SetWindowText(m_ButtonCopy, L"Copied");
	SetTimer(m_Hwnd, m_CopiedTimerId, 1500, NULL);
}

void TileExportDialog::PaintBackground(HDC hdc)
{
	RECT client;
	GetClientRect(m_Hwnd, &client);

	RECT content = client;
	content.bottom -= m_FooterHeight;
	FillRect(hdc, &content, GetSysColorBrush(COLOR_WINDOW));

	RECT divider = client;
	divider.top = content.bottom;
	divider.bottom = divider.top + 1;
	SetDCBrushColor(hdc, DividerColour);
	FillRect(hdc, &divider, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));

	RECT footer = client;
	footer.top = divider.bottom;
	FillRect(hdc, &footer, GetSysColorBrush(COLOR_BTNFACE));
}

LRESULT TileExportDialog::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_CLOSE:
			Close();
			return 0;

		case WM_DESTROY:
			KillTimer(hwnd, m_CopiedTimerId);
			if (m_Owner != NULL)
			{
				EnableWindow(m_Owner, TRUE);
			}

			m_Hwnd = NULL;
			return 0;

		case WM_TIMER:
			if (wParam == m_CopiedTimerId)
			{
				KillTimer(hwnd, m_CopiedTimerId);
				SetWindowText(m_ButtonCopy, L"Copy to clipboard");
				return 0;
			}

			break;

		case WM_CTLCOLORSTATIC:
		{
			HDC hdc = reinterpret_cast<HDC>(wParam);
			HWND control = reinterpret_cast<HWND>(lParam);
			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, control == m_LabelSize ? SecondaryTextColour : GetSysColor(COLOR_WINDOWTEXT));
			return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_WINDOW));
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
			const int code = HIWORD(wParam);

			if (id == IDCANCEL)
			{
				Close();
				return 0;
			}

			if (id == IDOK)
			{
				Save();
				return 0;
			}

			if (id == m_ControlCopyId && code == BN_CLICKED)
			{
				CopyToClipboard();
				return 0;
			}

			const bool layout = id >= m_ControlLayoutId && id < m_ControlLayoutId + 4;
			const bool scale = id >= m_ControlScaleId && id < m_ControlScaleId + 4;
			const bool option = id == m_ControlGridId || id == m_ControlTransparentId;
			if ((layout || scale || option) && code == BN_CLICKED)
			{
				Rebuild();
				return 0;
			}

			if (id == m_ControlPaletteId && code == CBN_SELCHANGE)
			{
				Rebuild();
				return 0;
			}

			break;
		}
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}
