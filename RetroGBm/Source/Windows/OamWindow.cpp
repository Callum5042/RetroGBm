#include "Windows/OamWindow.h"
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
	const wchar_t* ClassName = L"RetroGBmOam";

	static OamWindow* GetWindow(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		OamWindow* window = nullptr;
		if (uMsg == WM_NCCREATE)
		{
			CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
			window = reinterpret_cast<OamWindow*>(pCreate->lpCreateParams);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
		}
		else
		{
			window = reinterpret_cast<OamWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		}

		return window;
	}

	static LRESULT CALLBACK OamWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		OamWindow* window = GetWindow(hwnd, msg, wParam, lParam);
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

	// Height of text wrapped to a width
	int MeasureWrapped(HFONT font, const std::wstring& text, int width)
	{
		HDC hdc = GetDC(NULL);
		HGDIOBJ previous = SelectObject(hdc, font);

		RECT rect = { 0, 0, width, 0 };
		DrawTextW(hdc, text.c_str(), -1, &rect, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);

		SelectObject(hdc, previous);
		ReleaseDC(NULL, hdc);
		return rect.bottom;
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

	void FillChecker(HDC hdc, int x, int y, int width, int height, int square)
	{
		for (int py = 0; py < height; py += square)
		{
			for (int px = 0; px < width; px += square)
			{
				const bool dark = ((px / square) + (py / square)) % 2 == 1;
				FillColour(hdc, x + px, y + py, (std::min)(square, width - px), (std::min)(square, height - py), dark ? RGB(230, 230, 230) : RGB(255, 255, 255));
			}
		}
	}

	void FillTriangle(HDC hdc, int x, int y, int size, COLORREF colour)
	{
		// Pointing up, with its top at x, y
		const POINT points[] = { { x, y }, { x + size / 2 + 1, y + size }, { x - size / 2 - 1, y + size } };
		SetDCBrushColor(hdc, colour);
		SetDCPenColor(hdc, colour);
		HGDIOBJ previous_brush = SelectObject(hdc, GetStockObject(DC_BRUSH));
		HGDIOBJ previous_pen = SelectObject(hdc, GetStockObject(DC_PEN));
		Polygon(hdc, points, 3);
		SelectObject(hdc, previous_brush);
		SelectObject(hdc, previous_pen);
	}

	// Display colours are 0xAABBGGRR, the views want 0x00RRGGBB
	uint32_t ToViewColour(uint32_t colour)
	{
		return ((colour & 0xFF) << 16) | (colour & 0xFF00) | ((colour >> 16) & 0xFF);
	}

	// "104-111, 120" from lines in order
	template <typename T>
	std::wstring LineRanges(const std::vector<T>& lines)
	{
		std::wstring text;
		size_t i = 0;
		while (i < lines.size())
		{
			size_t end = i;
			while (end + 1 < lines.size() && lines[end + 1] == lines[end] + 1)
			{
				++end;
			}

			if (!text.empty())
			{
				text += L", ";
			}

			text += end == i ? std::to_wstring(lines[i]) : std::format(L"{}\x2013{}", lines[i], lines[end]);
			i = end + 1;
		}

		return text;
	}

	// Inspector preview: the object in pixels of 8px, 64 x 128 at most, with the palette beside it
	const int PreviewPixelSize = 8;
	const int PreviewWidth = 8 * PreviewPixelSize;
	const int PreviewHeight = 16 * PreviewPixelSize;

	const wchar_t* NoObjectHint = L"Hover over an object to see its address, or click one to pin it here.";
	const wchar_t* ObjectHint = L"Click an object to pin it here. The arrow keys move the selection, and double-click or Enter opens its tile in Tile Data.";

	const COLORREF DividerColour = RGB(223, 223, 223);
	const COLORREF InspectorColour = RGB(250, 250, 250);
	const COLORREF SecondaryTextColour = RGB(92, 92, 92);
	const COLORREF WarningColour = RGB(197, 90, 0);
	const COLORREF WarningFillColour = RGB(255, 246, 234);
	const COLORREF WarningBorderColour = RGB(240, 199, 143);
	const COLORREF WarningTitleColour = RGB(122, 56, 0);
	const COLORREF WarningTextColour = RGB(74, 42, 16);
}

OamWindow::OamWindow(Application* application) : m_Application(application)
{
}

OamWindow::~OamWindow()
{
	this->Destroy();
}

void OamWindow::Create()
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

		LOGFONT bold = metrics.lfMessageFont;
		bold.lfWeight = FW_SEMIBOLD;
		m_BoldFont = CreateFontIndirect(&bold);

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
	this->RenderObjects();
	this->UpdateStatus();

	ShowWindow(m_Hwnd, SW_SHOWNORMAL);
	SetFocus(m_GridView.GetHwnd());
}

void OamWindow::Destroy()
{
	// WM_DESTROY clears m_Hwnd
	if (m_Hwnd != NULL)
	{
		DestroyWindow(m_Hwnd);
	}

	UnregisterClass(ClassName, GetModuleHandle(NULL));
	ReleaseResources();
}

void OamWindow::ReleaseResources()
{
	m_GridView.Destroy();
	m_ScreenView.Destroy();

	for (HFONT* font : { &m_Font, &m_BoldFont, &m_HeadingFont, &m_MonoFont, &m_MonoSmallFont })
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

void OamWindow::Focus()
{
	if (IsIconic(m_Hwnd))
	{
		ShowWindow(m_Hwnd, SW_RESTORE);
	}

	SetForegroundWindow(m_Hwnd);
}

void OamWindow::Update()
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
		this->RenderObjects();
	}

	if (Button_GetCheck(m_ButtonPause) != BST_CHECKED)
	{
		this->CaptureSnapshot();
		this->RenderObjects();
	}

	this->UpdateTitle();
	this->UpdateStatus();
}

void OamWindow::UpdateTitle()
{
	if (m_TitleFilename == m_Application->CurrentFilename)
	{
		return;
	}

	m_TitleFilename = m_Application->CurrentFilename;

	// Show the ROM file name without its extension, e.g. "OAM - Pokemon Red (UE)".
	// CurrentFilename is in the ANSI code page, which path converts back.
	const std::wstring rom_name = std::filesystem::path(m_TitleFilename).stem().wstring();
	const std::wstring title = rom_name.empty() ? L"OAM" : L"OAM \x2014 " + rom_name;
	SetWindowText(m_Hwnd, title.c_str());
}

void OamWindow::CaptureSnapshot()
{
	Emulator* emulator = m_Application->GetEmulator();
	if (emulator == nullptr || !emulator->IsRunning())
	{
		return;
	}

	Ppu* ppu = emulator->GetPpu();
	Display* display = emulator->GetDisplay();

	m_Snapshot.oam = ppu->GetOam();

	const int banks = m_IsColour ? 2 : 1;
	for (int bank = 0; bank < 2; ++bank)
	{
		if (bank >= banks)
		{
			m_Snapshot.tiles[bank].clear();
			continue;
		}

		m_Snapshot.tiles[bank].resize(TileData::BytesPerBank);
		for (int i = 0; i < TileData::BytesPerBank; ++i)
		{
			m_Snapshot.tiles[bank][i] = ppu->ReadVideoRam(static_cast<uint16_t>(0x8000 + i), static_cast<uint8_t>(bank));
		}
	}

	m_Snapshot.lcdc = display->Read(0xFF40);
	m_Snapshot.coordinate_priority = display->CoordinatePriority();

	// The last finished frame, which already has the objects in it
	if (IsScreenView() && ShowBackground())
	{
		const uint32_t* frame = static_cast<const uint32_t*>(display->GetVideoBuffer());
		m_Snapshot.frame.resize(static_cast<size_t>(display->GetVideoBufferSize()));
		for (size_t i = 0; i < m_Snapshot.frame.size(); ++i)
		{
			m_Snapshot.frame[i] = ToViewColour(frame[i]);
		}
	}
	else
	{
		m_Snapshot.frame.clear();
	}

	// DMG has two palettes, OBP0 and OBP1, coloured the same way the screen colours them
	const int palettes = m_IsColour ? 8 : 2;
	for (int palette = 0; palette < 8; ++palette)
	{
		for (uint8_t index = 0; index < 4; ++index)
		{
			m_Snapshot.colours[palette][index] = palette < palettes
				? ToViewColour(display->GetColourFromObjectPalette(static_cast<uint8_t>(palette), index))
				: 0;
		}
	}

	m_HasSnapshot = true;
}

int OamWindow::ObjectHeight() const
{
	switch (static_cast<SizeChoice>(ComboBox_GetCurSel(m_ComboSize)))
	{
		case SizeChoice::Small: return 8;
		case SizeChoice::Tall: return 16;
		default: return Oam::HeightFromLcdc(m_Snapshot.lcdc);
	}
}

int OamWindow::PaletteOf(const Oam::Object& object) const
{
	const Oam::Attributes attributes = Oam::DecodeAttributes(object.flags);
	return m_IsColour ? attributes.cgb_palette : attributes.dmg_palette;
}

std::wstring OamWindow::PaletteName(const Oam::Object& object) const
{
	return m_IsColour ? std::format(L"OBJ {}", PaletteOf(object)) : std::format(L"OBP{}", PaletteOf(object));
}

std::vector<uint32_t> OamWindow::ObjectPixels(const Oam::Object& object, int height) const
{
	const uint8_t* bank0 = m_Snapshot.tiles[0].data();
	const uint8_t* bank1 = m_Snapshot.tiles[1].size() == TileData::BytesPerBank ? m_Snapshot.tiles[1].data() : nullptr;
	const std::vector<uint8_t> image = Oam::DecodeImage(object, height, bank0, bank1);
	const std::array<uint32_t, 4>& colours = m_Snapshot.colours[PaletteOf(object)];

	std::vector<uint32_t> pixels(image.size());
	for (size_t i = 0; i < image.size(); ++i)
	{
		// Colour 0 is transparent for objects
		pixels[i] = image[i] == 0 ? 0 : (0xFF000000 | colours[image[i]]);
	}

	return pixels;
}

void OamWindow::RenderObjects()
{
	if (!m_HasSnapshot || m_Snapshot.tiles[0].size() != TileData::BytesPerBank)
	{
		m_GridView.SetEmptyText(L"Load a game to see its objects.");
		m_GridView.SetCells({});
		m_ScreenView.SetEmptyText(L"Load a game to see its objects.");
		m_ScreenView.SetEmpty(true);
		m_LineScan = {};
		UpdateInspector();
		EnableWindow(m_ButtonExport, FALSE);
		return;
	}

	const int height = ObjectHeight();
	const bool show_hidden = ShowHidden();
	m_LineScan = Oam::ScanLines(m_Snapshot.oam.data(), height);

	// Only the view on show is kept up to date; switching views renders the other
	if (IsScreenView())
	{
		RenderScreen();
		UpdateInspector();
		return;
	}

	std::vector<OamGridView::Cell> cells;
	cells.reserve(Oam::Count);
	for (int index = 0; index < Oam::Count; ++index)
	{
		const Oam::Object object = Oam::Decode(m_Snapshot.oam.data(), index);
		if (!show_hidden && !Oam::IsVisible(object, height))
		{
			continue;
		}

		OamGridView::Cell cell;
		cell.index = index;
		cell.height = height;
		cell.pixels = ObjectPixels(object, height);

		// "$0E · P4" on CGB, "$0E · OBP1" on DMG
		const std::wstring palette = m_IsColour ? std::format(L"P{}", PaletteOf(object)) : PaletteName(object);
		cell.caption = std::format(L"${:02X} \x00B7 {}", Oam::FirstTile(object.tile, height), palette);
		cell.hidden = !Oam::IsVisible(object, height);
		cell.dropped = !m_LineScan.dropped[index].empty();
		cells.push_back(std::move(cell));
	}

	// With hidden objects left out there may be nothing to show
	m_GridView.SetEmptyText(L"No objects are on screen. Tick Show hidden to see all 40.");
	m_GridView.SetCells(cells);
	UpdateInspector();

	const BOOL can_export = cells.empty() ? FALSE : TRUE;
	if (IsWindowEnabled(m_ButtonExport) != can_export)
	{
		EnableWindow(m_ButtonExport, can_export);
	}
}

void OamWindow::RenderScreen()
{
	const int height = ObjectHeight();

	// Visible objects in OAM order, and the ones that use line slots without being seen
	std::vector<Oam::Object> objects;
	std::vector<int> visible;
	std::vector<int> unseen;
	for (int index = 0; index < Oam::Count; ++index)
	{
		objects.push_back(Oam::Decode(m_Snapshot.oam.data(), index));
		if (Oam::IsVisible(objects.back(), height))
		{
			visible.push_back(index);
		}
		else if (Oam::IsOnLines(objects.back(), height))
		{
			unseen.push_back(index);
		}
	}

	// Back to front. Where objects overlap, the one further left wins, then the one earlier in OAM,
	// unless CGB is set to go by OAM order alone.
	const bool by_x = m_Snapshot.coordinate_priority;
	std::sort(visible.begin(), visible.end(), [&](int a, int b)
	{
		if (by_x && objects[a].x != objects[b].x)
		{
			return objects[a].x > objects[b].x;
		}

		return a > b;
	});

	OamScreenView::Scene scene;
	scene.lines = m_LineScan.objects;
	if (ShowBackground())
	{
		scene.background = m_Snapshot.frame;
	}
	for (int index : visible)
	{
		OamScreenView::Object object;
		object.index = index;
		object.x = objects[index].ScreenX();
		object.y = objects[index].ScreenY();
		object.height = height;
		object.pixels = ObjectPixels(objects[index], height);
		object.dropped = m_LineScan.dropped[index];
		scene.objects.push_back(std::move(object));
	}

	if ((m_Snapshot.lcdc & 0x02) == 0)
	{
		scene.note = L"Objects are turned off (LCDC.1), so the game isn't drawing any of these.";
	}
	else if (!unseen.empty())
	{
		std::wstring list;
		for (int index : unseen)
		{
			list += (list.empty() ? L"" : L", ") + std::format(L"{:02}", index);
		}

		scene.note = std::format(L"Not shown: {} {}, off the left or right edge but still using a slot on {} lines.",
			unseen.size() == 1 ? L"object" : L"objects", list, unseen.size() == 1 ? L"its" : L"their");
	}

	m_ScreenView.SetEmpty(false);
	m_ScreenView.SetScene(scene);
}

bool OamWindow::IsScreenView() const
{
	return Button_GetCheck(m_ButtonScreen) == BST_CHECKED;
}

void OamWindow::ShowView()
{
	const bool screen = IsScreenView();
	ShowWindow(m_GridView.GetHwnd(), screen ? SW_HIDE : SW_SHOWNA);
	ShowWindow(m_ScreenView.GetHwnd(), screen ? SW_SHOWNA : SW_HIDE);

	// Zoom, Show hidden and Export are for the grid, the rest for the screen
	for (HWND control : { m_LabelZoom, m_ComboZoom, m_CheckShowHidden, m_ButtonExport })
	{
		ShowWindow(control, screen ? SW_HIDE : SW_SHOWNA);
	}

	for (HWND control : { m_CheckBackground, m_CheckOutlines, m_CheckLineLimit })
	{
		ShowWindow(control, screen ? SW_SHOWNA : SW_HIDE);
	}

	// The frame is only taken while the Screen view needs it. A paused snapshot is kept as it is.
	if (Button_GetCheck(m_ButtonPause) != BST_CHECKED)
	{
		CaptureSnapshot();
	}

	RenderObjects();
	Layout();
}

bool OamWindow::ShowBackground() const
{
	return Button_GetCheck(m_CheckBackground) == BST_CHECKED;
}

void OamWindow::SelectionChanged(int index)
{
	m_Selection = index;
	m_GridView.SetSelection(index);
	m_ScreenView.SetSelection(index);
	UpdateInspector();
}

bool OamWindow::ShowHidden() const
{
	return Button_GetCheck(m_CheckShowHidden) == BST_CHECKED;
}

void OamWindow::ExportPng()
{
	if (!m_HasSnapshot || m_Snapshot.tiles[0].size() != TileData::BytesPerBank)
	{
		return;
	}

	// Take the image now, so it matches what was on screen when Export was clicked.
	// Objects are laid out 8 to a row in OAM order, one zoomed pixel apart.
	const int zoom = static_cast<int>(ComboBox_GetItemData(m_ComboZoom, ComboBox_GetCurSel(m_ComboZoom)));
	const int height = ObjectHeight();
	const bool show_hidden = ShowHidden();

	std::vector<std::vector<uint32_t>> objects;
	for (int index = 0; index < Oam::Count; ++index)
	{
		const Oam::Object object = Oam::Decode(m_Snapshot.oam.data(), index);
		if (show_hidden || Oam::IsVisible(object, height))
		{
			objects.push_back(ObjectPixels(object, height));
		}
	}

	if (objects.empty())
	{
		return;
	}

	const int columns = (std::min)(8, static_cast<int>(objects.size()));
	const int rows = (static_cast<int>(objects.size()) + columns - 1) / columns;
	const int cell_width = 8 * zoom;
	const int cell_height = height * zoom;
	const int width = columns * cell_width + (columns - 1) * zoom;
	const int image_height = rows * cell_height + (rows - 1) * zoom;

	// RGBA bytes, transparent where no object pixel is drawn
	std::vector<uint8_t> rgba(static_cast<size_t>(width) * image_height * 4);
	for (size_t i = 0; i < objects.size(); ++i)
	{
		const int origin_x = static_cast<int>(i % columns) * (cell_width + zoom);
		const int origin_y = static_cast<int>(i / columns) * (cell_height + zoom);
		for (int y = 0; y < cell_height; ++y)
		{
			for (int x = 0; x < cell_width; ++x)
			{
				const uint32_t colour = objects[i][static_cast<size_t>(y / zoom) * 8 + x / zoom];
				uint8_t* pixel = rgba.data() + (static_cast<size_t>(origin_y + y) * width + origin_x + x) * 4;
				pixel[0] = static_cast<uint8_t>((colour >> 16) & 0xFF);
				pixel[1] = static_cast<uint8_t>((colour >> 8) & 0xFF);
				pixel[2] = static_cast<uint8_t>(colour & 0xFF);
				pixel[3] = static_cast<uint8_t>((colour >> 24) & 0xFF);
			}
		}
	}

	const std::wstring title = L"Export Objects";
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
			// "Pokemon Red objects.png"
			const std::wstring rom_name = std::filesystem::path(m_Application->CurrentFilename).stem().wstring();
			const std::wstring filename = (rom_name.empty() ? L"Objects" : rom_name + L" objects") + L".png";

			const COMDLG_FILTERSPEC filters[] = { { L"PNG image", L"*.png" } };
			hr = dialog->SetFileTypes(1, filters);
			if (SUCCEEDED(hr)) hr = dialog->SetDefaultExtension(L"png");
			if (SUCCEEDED(hr)) hr = dialog->SetFileName(filename.c_str());
			if (SUCCEEDED(hr)) hr = dialog->SetTitle(L"Save Objects");
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
				stbi_write_png_to_func(write, &png, width, image_height, 4, rgba.data(), width * 4);

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

std::wstring OamWindow::PositionText(const Oam::Object& object, int index) const
{
	const int height = ObjectHeight();
	if (!Oam::IsOnLines(object, height))
	{
		return std::format(L"Hidden \x00B7 Y = ${:02X} is off the top or bottom", object.y);
	}

	if (!Oam::IsVisible(object, height))
	{
		return std::format(L"Hidden \x00B7 X = ${:02X}, but it still uses a slot on its lines", object.x);
	}

	std::wstring text = std::format(L"On screen at {}, {}", object.ScreenX(), object.ScreenY());
	if (!m_LineScan.dropped[index].empty())
	{
		text += L" \x00B7 partly dropped";
	}

	return text;
}

void OamWindow::UpdateInspector()
{
	const int index = m_Selection;
	const bool has_object = index >= 0 && m_HasSnapshot && m_Snapshot.tiles[0].size() == TileData::BytesPerBank;
	const bool has_warning = has_object && !m_LineScan.dropped[index].empty();

	// Show the details only while an object is selected, and the warning only when it's dropped
	if (has_object != m_InspectorHasObject || has_warning != m_InspectorHasWarning)
	{
		m_InspectorHasObject = has_object;
		m_InspectorHasWarning = has_warning;

		std::vector<HWND> details = { m_LabelSubtitle, m_Preview, m_ButtonOpenTile };
		details.insert(details.end(), m_PropertyLabels.begin(), m_PropertyLabels.end());
		details.insert(details.end(), m_PropertyValues.begin(), m_PropertyValues.end());
		for (HWND control : details)
		{
			ShowWindow(control, has_object ? SW_SHOWNA : SW_HIDE);
		}

		ShowWindow(m_Warning, has_warning ? SW_SHOWNA : SW_HIDE);
		SetTextIfChanged(m_LabelHint, has_object ? ObjectHint : NoObjectHint);
		Layout();
	}

	if (!has_object)
	{
		SetTextIfChanged(m_LabelHeading, L"No object selected");
		return;
	}

	const Oam::Object object = Oam::Decode(m_Snapshot.oam.data(), index);
	const Oam::Attributes attributes = Oam::DecodeAttributes(object.flags);
	const int height = ObjectHeight();
	const uint8_t first = Oam::FirstTile(object.tile, height);
	const uint16_t data = Oam::TileAddress(object.tile, height);
	const uint16_t address = Oam::ObjectAddress(index);

	SetTextIfChanged(m_LabelHeading, std::format(L"Object {:02}", index));
	SetTextIfChanged(m_LabelSubtitle, PositionText(object, index));

	std::wstring flip = L"None";
	if (attributes.flip_x && attributes.flip_y) flip = L"X and Y";
	else if (attributes.flip_x) flip = L"X";
	else if (attributes.flip_y) flip = L"Y";

	const int data_end = data + TileData::BytesPerTile * (height / 8) - 1;
	SetTextIfChanged(m_PropertyValues[0], std::format(L"${:04X}\x2013${:04X}", address, address + Oam::BytesPerObject - 1));
	SetTextIfChanged(m_PropertyValues[1], std::format(L"X ${:02X} \x00B7 Y ${:02X}", object.x, object.y));
	SetTextIfChanged(m_PropertyValues[2], height == 16 ? std::format(L"${:02X} + ${:02X}", first, first + 1) : std::format(L"${:02X}", first));
	SetTextIfChanged(m_PropertyValues[3], m_IsColour
		? std::format(L"${:04X}\x2013${:04X} \x00B7 bank {}", data, data_end, attributes.bank)
		: std::format(L"${:04X}\x2013${:04X}", data, data_end));
	SetTextIfChanged(m_PropertyValues[4], std::format(L"${:02X}", object.flags));
	SetTextIfChanged(m_PropertyValues[5], PaletteName(object));
	SetTextIfChanged(m_PropertyValues[6], flip);
	SetTextIfChanged(m_PropertyValues[7], attributes.priority ? L"Behind BG colours 1\x2013" L"3" : L"Over background");

	// Line limit
	if (has_warning)
	{
		const std::vector<uint8_t>& dropped = m_LineScan.dropped[index];
		int most = 0;
		for (uint8_t line : dropped)
		{
			most = (std::max)(most, static_cast<int>(m_LineScan.objects[line]));
		}

		const std::wstring title = std::format(L"Not drawn on {} {}", dropped.size() == 1 ? L"line" : L"lines", LineRanges(dropped));
		const std::wstring text = std::format(L"Up to {} objects share these lines. The PPU draws only the first {} it finds in OAM, so this one is skipped there.",
			most, Oam::ObjectsPerLine);

		if (title != m_WarningTitle || text != m_WarningText)
		{
			m_WarningTitle = title;
			m_WarningText = text;
			InvalidateRect(m_Warning, NULL, FALSE);

			// Title, then the explanation wrapped beside the triangle
			const int height = 30 + MeasureWrapped(m_Font, text, m_InspectorWidth - 33 - 44) + 10;
			if (height != m_WarningHeight)
			{
				m_WarningHeight = height;
				Layout();
			}
		}
	}

	// Preview
	const std::vector<uint32_t> pixels = ObjectPixels(object, height);
	const std::array<uint32_t, 4>& colours = m_Snapshot.colours[PaletteOf(object)];
	const std::wstring palette_name = PaletteName(object);
	if (pixels != m_PreviewPixels || colours != m_PreviewColours || palette_name != m_PreviewPaletteName)
	{
		m_PreviewPixels = pixels;
		m_PreviewHeight = height;
		m_PreviewColours = colours;
		m_PreviewPaletteName = palette_name;
		InvalidateRect(m_Preview, NULL, FALSE);
	}
}

void OamWindow::DrawPreview(const DRAWITEMSTRUCT* item)
{
	const RECT& rect = item->rcItem;
	const int width = rect.right - rect.left;
	const int height = rect.bottom - rect.top;

	// Draw off screen so the preview doesn't flicker when it changes every frame
	HDC hdc = CreateCompatibleDC(item->hDC);
	HBITMAP bitmap = CreateCompatibleBitmap(item->hDC, width, height);
	HGDIOBJ previous_bitmap = SelectObject(hdc, bitmap);

	FillColour(hdc, 0, 0, width, height, InspectorColour);

	// The object on a checkerboard, so transparent pixels show as such
	const int image_height = m_PreviewHeight * PreviewPixelSize;
	FillChecker(hdc, 0, 0, PreviewWidth, image_height, PreviewPixelSize);

	if (m_PreviewPixels.size() == static_cast<size_t>(8 * m_PreviewHeight))
	{
		for (int y = 0; y < m_PreviewHeight; ++y)
		{
			for (int x = 0; x < 8; ++x)
			{
				const uint32_t pixel = m_PreviewPixels[static_cast<size_t>(y) * 8 + x];
				if ((pixel & 0xFF000000) != 0)
				{
					FillColour(hdc, x * PreviewPixelSize, y * PreviewPixelSize, PreviewPixelSize, PreviewPixelSize, ToColorRef(pixel));
				}
			}
		}
	}

	RECT outline = { 0, 0, PreviewWidth, image_height };
	SetDCBrushColor(hdc, RGB(200, 200, 200));
	FrameRect(hdc, &outline, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));

	// Palette swatches, colour 0 shown as transparent
	const int swatch_x = PreviewWidth + 16;
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
		const int y = 24 + i * 26;
		if (i == 0)
		{
			FillChecker(hdc, swatch_x, y, 20, 20, 5);
		}
		else
		{
			FillColour(hdc, swatch_x, y, 20, 20, ToColorRef(m_PreviewColours[i]));
		}

		RECT swatch = { swatch_x, y, swatch_x + 20, y + 20 };
		SetDCBrushColor(hdc, RGB(200, 200, 200));
		FrameRect(hdc, &swatch, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));

		const std::wstring text = i == 0 ? L"0  transparent" : std::format(L"{}  #{:06X}", i, m_PreviewColours[i] & 0xFFFFFF);
		RECT text_rect = { swatch_x + 26, y, width, y + 20 };
		DrawTextW(hdc, text.c_str(), -1, &text_rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	}

	BitBlt(item->hDC, rect.left, rect.top, width, height, hdc, 0, 0, SRCCOPY);

	SelectObject(hdc, previous_bitmap);
	DeleteObject(bitmap);
	DeleteDC(hdc);
}

void OamWindow::DrawWarning(const DRAWITEMSTRUCT* item)
{
	RECT rect = item->rcItem;
	HDC hdc = item->hDC;

	FillColour(hdc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, WarningFillColour);
	SetDCBrushColor(hdc, WarningBorderColour);
	FrameRect(hdc, &rect, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));

	FillTriangle(hdc, rect.left + 18, rect.top + 12, 12, WarningColour);

	SetBkMode(hdc, TRANSPARENT);
	const int text_left = rect.left + 34;

	SelectObject(hdc, m_BoldFont);
	SetTextColor(hdc, WarningTitleColour);
	RECT title_rect = { text_left, rect.top + 10, rect.right - 10, rect.top + 28 };
	DrawTextW(hdc, m_WarningTitle.c_str(), -1, &title_rect, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

	SelectObject(hdc, m_Font);
	SetTextColor(hdc, WarningTextColour);
	RECT text_rect = { text_left, rect.top + 30, rect.right - 10, rect.bottom - 8 };
	DrawTextW(hdc, m_WarningText.c_str(), -1, &text_rect, DT_LEFT | DT_TOP | DT_WORDBREAK | DT_NOPREFIX);
}

void OamWindow::OpenSelectedTile()
{
	const int index = m_Selection;
	if (index < 0 || !m_HasSnapshot)
	{
		return;
	}

	const Oam::Object object = Oam::Decode(m_Snapshot.oam.data(), index);
	const int bank = m_IsColour ? Oam::DecodeAttributes(object.flags).bank : 0;
	m_Application->ShowTileInTileWindow(bank, Oam::FirstTile(object.tile, ObjectHeight()));
}

void OamWindow::UpdateHoverStatus(int index)
{
	std::wstring text;
	if (index >= 0 && m_HasSnapshot)
	{
		const Oam::Object object = Oam::Decode(m_Snapshot.oam.data(), index);
		const std::wstring position = Oam::IsVisible(object, ObjectHeight())
			? std::format(L"{}, {}", object.ScreenX(), object.ScreenY())
			: L"hidden";

		text = std::format(L"Object {:02} \x00B7 ${:04X} \x00B7 tile ${:02X} \x00B7 {}",
			index, Oam::ObjectAddress(index), object.tile, position);
	}

	SendMessageW(m_StatusBar, SB_SETTEXTW, 1, reinterpret_cast<LPARAM>(text.c_str()));
}

bool OamWindow::HandleDialogMessage(MSG* msg)
{
	return m_Hwnd != NULL && IsDialogMessage(m_Hwnd, msg);
}

void OamWindow::WindowCreate(int width, int height)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);

	WNDCLASS wc = {};
	wc.style = CS_VREDRAW | CS_HREDRAW;
	wc.lpfnWndProc = OamWndProc;
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

	m_Hwnd = CreateWindowEx(exStyle, ClassName, L"OAM", dwStyle, x, y, window_width, window_height, owner, NULL, hInstance, this);
	if (m_Hwnd == NULL)
	{
		throw std::exception("CreateWindow Failed");
	}
}

void OamWindow::CreateControls()
{
	auto create = [this](const wchar_t* type, const wchar_t* text, DWORD style, int id, HFONT font)
	{
		HWND control = CreateWindowW(type, text, WS_CHILD | WS_VISIBLE | style, 0, 0, 0, 0, m_Hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), nullptr, nullptr);
		SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
		return control;
	};

	// View: the grid of objects, or where they are on screen
	m_ButtonObjects = create(L"BUTTON", L"Objects", WS_TABSTOP | WS_GROUP | BS_AUTORADIOBUTTON | BS_PUSHLIKE, m_ControlObjectsId, m_Font);
	m_ButtonScreen = create(L"BUTTON", L"Screen", BS_AUTORADIOBUTTON | BS_PUSHLIKE, m_ControlScreenId, m_Font);
	Button_SetCheck(m_ButtonObjects, BST_CHECKED);

	// Size: LCDC.2 by default, as the game changes it
	m_LabelSize = create(L"STATIC", L"Size", SS_CENTERIMAGE, 0, m_Font);
	m_ComboSize = create(WC_COMBOBOX, L"", WS_TABSTOP | WS_GROUP | CBS_DROPDOWNLIST, m_ControlSizeId, m_Font);
	for (const wchar_t* text : { L"Auto (LCDC.2)", L"8\x00D7" L"8", L"8\x00D7" L"16" })
	{
		ComboBox_AddString(m_ComboSize, text);
	}
	ComboBox_SetCurSel(m_ComboSize, 0);

	// Zoom
	m_LabelZoom = create(L"STATIC", L"Zoom", SS_CENTERIMAGE, 0, m_Font);
	m_ComboZoom = create(WC_COMBOBOX, L"", WS_TABSTOP | CBS_DROPDOWNLIST, m_ControlZoomId, m_Font);
	for (int zoom : { 2, 3, 4, 6 })
	{
		const std::wstring text = std::to_wstring(zoom) + L"\x00D7";
		const int index = ComboBox_AddString(m_ComboZoom, text.c_str());
		ComboBox_SetItemData(m_ComboZoom, index, zoom);
	}
	ComboBox_SetCurSel(m_ComboZoom, 2);

	// Hidden objects: off screen, or at X = 0
	m_CheckShowHidden = create(L"BUTTON", L"Show hidden", WS_TABSTOP | BS_AUTOCHECKBOX, m_ControlShowHiddenId, m_Font);
	Button_SetCheck(m_CheckShowHidden, BST_CHECKED);

	// Screen options, hidden until the Screen view is picked
	m_CheckBackground = create(L"BUTTON", L"Background", WS_TABSTOP | BS_AUTOCHECKBOX, m_ControlBackgroundId, m_Font);
	m_CheckOutlines = create(L"BUTTON", L"Outlines", WS_TABSTOP | BS_AUTOCHECKBOX, m_ControlOutlinesId, m_Font);
	m_CheckLineLimit = create(L"BUTTON", L"Line limit", WS_TABSTOP | BS_AUTOCHECKBOX, m_ControlLineLimitId, m_Font);
	Button_SetCheck(m_CheckOutlines, BST_CHECKED);
	Button_SetCheck(m_CheckLineLimit, BST_CHECKED);
	for (HWND control : { m_CheckBackground, m_CheckOutlines, m_CheckLineLimit })
	{
		ShowWindow(control, SW_HIDE);
	}

	// Pause updates and export, on the right
	m_ButtonPause = create(L"BUTTON", L"Pause updates", WS_TABSTOP | BS_AUTOCHECKBOX | BS_PUSHLIKE, m_ControlPauseId, m_Font);
	m_ButtonExport = create(L"BUTTON", L"Export PNG\x2026", WS_TABSTOP | BS_PUSHBUTTON | WS_DISABLED, m_ControlExportId, m_Font);

	// Objects
	m_GridView.Create(m_Hwnd, m_ControlGridId);
	m_GridView.SetZoom(static_cast<int>(ComboBox_GetItemData(m_ComboZoom, ComboBox_GetCurSel(m_ComboZoom))));
	m_GridView.HoverChanged = [this]() { UpdateHoverStatus(m_GridView.GetHover()); };
	m_GridView.SelectionChanged = [this]() { SelectionChanged(m_GridView.GetSelection()); };
	m_GridView.ActivateRequested = [this]() { OpenSelectedTile(); };

	// Screen, hidden until picked
	m_ScreenView.Create(m_Hwnd, m_ControlScreenViewId);
	m_ScreenView.HoverChanged = [this]() { UpdateHoverStatus(m_ScreenView.GetHover()); };
	m_ScreenView.SelectionChanged = [this]() { SelectionChanged(m_ScreenView.GetSelection()); };
	m_ScreenView.ActivateRequested = [this]() { OpenSelectedTile(); };

	// Inspector, with the details hidden until an object is selected
	m_LabelHeading = create(L"STATIC", L"No object selected", SS_LEFT | SS_ENDELLIPSIS, 0, m_HeadingFont);
	m_LabelSubtitle = create(L"STATIC", L"", SS_LEFT | SS_ENDELLIPSIS, 0, m_Font);
	m_Preview = create(L"STATIC", L"", SS_OWNERDRAW, m_ControlPreviewId, m_Font);

	const wchar_t* property_names[] = { L"OAM address", L"Position", L"Tile", L"Tile data", L"Attributes", L"Palette", L"Flip", L"Priority" };
	for (size_t i = 0; i < m_PropertyLabels.size(); ++i)
	{
		m_PropertyLabels[i] = create(L"STATIC", property_names[i], SS_LEFT, 0, m_Font);
		m_PropertyValues[i] = create(L"STATIC", L"", SS_LEFT | SS_ENDELLIPSIS, 0, m_MonoFont);
	}

	m_Warning = create(L"STATIC", L"", SS_OWNERDRAW, m_ControlWarningId, m_Font);
	m_ButtonOpenTile = create(L"BUTTON", L"Open tile in Tile Data", WS_TABSTOP | BS_PUSHBUTTON, m_ControlOpenTileId, m_Font);
	m_LabelHint = create(L"STATIC", NoObjectHint, SS_LEFT, 0, m_Font);

	std::vector<HWND> details = { m_LabelSubtitle, m_Preview, m_Warning, m_ButtonOpenTile };
	details.insert(details.end(), m_PropertyLabels.begin(), m_PropertyLabels.end());
	details.insert(details.end(), m_PropertyValues.begin(), m_PropertyValues.end());
	for (HWND control : details)
	{
		ShowWindow(control, SW_HIDE);
	}
	m_InspectorHasObject = false;
	m_InspectorHasWarning = false;

	// Status bar
	m_StatusBar = CreateWindowExW(0, STATUSCLASSNAME, L"", WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
		0, 0, 0, 0, m_Hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(m_ControlStatusBarId)), nullptr, nullptr);
	SendMessageW(m_StatusBar, WM_SETFONT, reinterpret_cast<WPARAM>(m_Font), TRUE);
}

void OamWindow::UpdateStatus()
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
		// "CGB · LCDC $E7 · objects on · 8×16 · 20 of 40 on screen · lines 104–111 over the limit · Live, updates every frame"
		const uint8_t lcdc = m_Snapshot.lcdc;
		const int height = ObjectHeight();

		int visible = 0;
		for (int index = 0; index < Oam::Count; ++index)
		{
			if (Oam::IsVisible(Oam::Decode(m_Snapshot.oam.data(), index), height))
			{
				++visible;
			}
		}

		std::vector<int> over;
		for (int line = 0; line < Oam::ScreenHeight; ++line)
		{
			if (m_LineScan.IsOverLimit(line))
			{
				over.push_back(line);
			}
		}

		text = std::format(L"{} \x00B7 LCDC ${:02X} \x00B7 objects {} \x00B7 8\x00D7{} \x00B7 {} of {} on screen",
			m_IsColour ? L"CGB" : L"DMG", lcdc, (lcdc & 0x02) ? L"on" : L"off", height, visible, Oam::Count);

		if (!over.empty())
		{
			text += std::format(L" \x00B7 {} {} over the limit", over.size() == 1 ? L"line" : L"lines", LineRanges(over));
		}

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

void OamWindow::Layout()
{
	RECT client;
	GetClientRect(m_Hwnd, &client);
	const int width = client.right;

	// Status bar sizes itself to the bottom of the window
	SendMessageW(m_StatusBar, WM_SIZE, 0, 0);
	RECT status_rect;
	GetWindowRect(m_StatusBar, &status_rect);
	const int status_height = status_rect.bottom - status_rect.top;

	int parts[] = { width - 300 > 0 ? width - 300 : 0, -1 };
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

	// View buttons side by side
	for (HWND button : { m_ButtonObjects, m_ButtonScreen })
	{
		const int w = MeasureText(m_Font, GetText(button)) + 28;
		place(button, x, button_y, w, button_height);
		x += w;
	}
	x += group_gap;

	label(m_LabelSize);
	place(m_ComboSize, x, combo_y, 120, 200);
	x += 120 + group_gap;

	if (!IsScreenView())
	{
		label(m_LabelZoom);
		place(m_ComboZoom, x, combo_y, 56, 200);
		x += 56 + group_gap;

		place(m_CheckShowHidden, x, button_y, MeasureText(m_Font, GetText(m_CheckShowHidden)) + 24, button_height);
	}
	else
	{
		for (HWND check : { m_CheckBackground, m_CheckOutlines, m_CheckLineLimit })
		{
			const int w = MeasureText(m_Font, GetText(check)) + 24;
			place(check, x, button_y, w, button_height);
			x += w + 8;
		}
	}

	// Export is only for the grid, so Pause updates takes its place in the Screen view
	const int export_width = IsScreenView() ? -8 : MeasureText(m_Font, GetText(m_ButtonExport)) + 28;
	const int pause_width = MeasureText(m_Font, GetText(m_ButtonPause)) + 28;
	if (!IsScreenView())
	{
		place(m_ButtonExport, width - 12 - export_width, button_y, export_width, button_height);
	}
	place(m_ButtonPause, width - 12 - export_width - 8 - pause_width, button_y, pause_width, button_height);

	// Inspector
	const int inspector_x = width - m_InspectorWidth + 1 + 16;
	const int inspector_width = m_InspectorWidth - 1 - 32;
	const int body_top = m_CommandBarHeight + 1;
	int y = body_top + 16;
	place(m_LabelHeading, inspector_x, y, inspector_width, 28);
	y += 30;

	if (m_InspectorHasObject)
	{
		place(m_LabelSubtitle, inspector_x, y, inspector_width, 20);
		y += 28;

		place(m_Preview, inspector_x, y, inspector_width, PreviewHeight);
		y += PreviewHeight + 16;

		const int property_label_width = 92;
		for (size_t i = 0; i < m_PropertyLabels.size(); ++i)
		{
			place(m_PropertyLabels[i], inspector_x, y, property_label_width, 20);
			place(m_PropertyValues[i], inspector_x + property_label_width + 4, y, inspector_width - property_label_width - 4, 20);
			y += 22;
		}
		y += 10;

		if (m_InspectorHasWarning)
		{
			place(m_Warning, inspector_x, y, inspector_width, m_WarningHeight);
			y += m_WarningHeight + 12;
		}

		const int open_width = MeasureText(m_Font, GetText(m_ButtonOpenTile)) + 28;
		place(m_ButtonOpenTile, inspector_x, y, open_width, button_height);
		y += button_height + 16;
	}

	place(m_LabelHint, inspector_x, y, inspector_width, 64);

	// Objects fill the rest, left of the inspector
	const int body_height = client.bottom - status_height - body_top;
	place(m_GridView.GetHwnd(), 0, body_top, width - m_InspectorWidth, body_height > 0 ? body_height : 0);
	place(m_ScreenView.GetHwnd(), 0, body_top, width - m_InspectorWidth, body_height > 0 ? body_height : 0);

	EndDeferWindowPos(defer);

	InvalidateRect(m_Hwnd, NULL, TRUE);
}

void OamWindow::PaintBackground(HDC hdc)
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

	// 1px divider and the inspector, right of the objects
	RECT inspector_divider = client;
	inspector_divider.top = divider.bottom;
	inspector_divider.left = client.right - m_InspectorWidth;
	inspector_divider.right = inspector_divider.left + 1;
	SetDCBrushColor(hdc, DividerColour);
	FillRect(hdc, &inspector_divider, dc_brush);

	RECT inspector = inspector_divider;
	inspector.left = inspector_divider.right;
	inspector.right = client.right;
	FillRect(hdc, &inspector, m_InspectorBrush);
}

LRESULT OamWindow::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
				m_Application->GetMainWindow()->ToggleOamWindowMenuItem(false);
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
			RECT minimum = { 0, 0, 760, 560 };
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
				SetFocus(m_LastFocus != NULL ? m_LastFocus : (IsScreenView() ? m_ScreenView.GetHwnd() : m_GridView.GetHwnd()));
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

			const bool checkbox = control == m_CheckShowHidden || control == m_CheckBackground || control == m_CheckOutlines || control == m_CheckLineLimit;
			SetTextColor(hdc, checkbox ? GetSysColor(COLOR_WINDOWTEXT) : SecondaryTextColour);
			return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_BTNFACE));
		}

		case WM_DRAWITEM:
			if (static_cast<int>(wParam) == m_ControlPreviewId)
			{
				DrawPreview(reinterpret_cast<const DRAWITEMSTRUCT*>(lParam));
				return TRUE;
			}

			if (static_cast<int>(wParam) == m_ControlWarningId)
			{
				DrawWarning(reinterpret_cast<const DRAWITEMSTRUCT*>(lParam));
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

			if (id == m_ControlSizeId && code == CBN_SELCHANGE)
			{
				RenderObjects();
				UpdateStatus();
				return 0;
			}

			if (id == m_ControlOpenTileId && code == BN_CLICKED)
			{
				OpenSelectedTile();
				return 0;
			}

			if ((id == m_ControlObjectsId || id == m_ControlScreenId) && code == BN_CLICKED)
			{
				ShowView();
				return 0;
			}

			if (id == m_ControlBackgroundId && code == BN_CLICKED)
			{
				// While paused there is only a frame if Background was on when the snapshot was taken
				if (Button_GetCheck(m_ButtonPause) != BST_CHECKED)
				{
					CaptureSnapshot();
				}

				RenderObjects();
				return 0;
			}

			if (id == m_ControlOutlinesId && code == BN_CLICKED)
			{
				m_ScreenView.SetShowOutlines(Button_GetCheck(m_CheckOutlines) == BST_CHECKED);
				return 0;
			}

			if (id == m_ControlLineLimitId && code == BN_CLICKED)
			{
				m_ScreenView.SetShowLineLimit(Button_GetCheck(m_CheckLineLimit) == BST_CHECKED);
				return 0;
			}

			if (id == m_ControlZoomId && code == CBN_SELCHANGE)
			{
				m_GridView.SetZoom(static_cast<int>(ComboBox_GetItemData(m_ComboZoom, ComboBox_GetCurSel(m_ComboZoom))));
				return 0;
			}

			if (id == m_ControlShowHiddenId && code == BN_CLICKED)
			{
				RenderObjects();
				return 0;
			}

			if (id == m_ControlExportId && code == BN_CLICKED)
			{
				ExportPng();
				return 0;
			}

			break;
		}
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}
