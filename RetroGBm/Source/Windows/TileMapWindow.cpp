#include "Windows/TileMapWindow.h"
#include "Windows/MainWindow.h"
#include "Application.h"
#include "../resource.h"

#include <CommCtrl.h>
#include <windowsx.h>
#pragma comment(lib, "comctl32.lib")

#include <RetroGBm/Emulator.h>
#include <RetroGBm/Display.h>
#include <RetroGBm/Ppu.h>

#include <filesystem>
#include <format>
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
	NONCLIENTMETRICS metrics = { sizeof(NONCLIENTMETRICS) };
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0);
	m_Font = CreateFontIndirect(&metrics.lfMessageFont);

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

	if (m_Font != NULL)
	{
		DeleteObject(m_Font);
		m_Font = NULL;
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
		return;
	}

	const uint16_t address = MapAddress();
	const bool background = address == TileMap::BackgroundMap(m_Snapshot.lcdc);
	const bool window = address == TileMap::WindowMap(m_Snapshot.lcdc) && (m_Snapshot.lcdc & 0x20) != 0;
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

	// Grid
	m_CheckGrid = create(L"BUTTON", L"Grid", WS_TABSTOP | BS_AUTOCHECKBOX, m_ControlGridId, m_Font);

	// Pause updates, on the right
	m_ButtonPause = create(L"BUTTON", L"Pause updates", WS_TABSTOP | BS_AUTOCHECKBOX | BS_PUSHLIKE, m_ControlPauseId, m_Font);

	// Map
	m_MapView.Create(m_Hwnd, m_ControlMapViewId);
	m_MapView.SetZoom(static_cast<int>(ComboBox_GetItemData(m_ComboZoom, ComboBox_GetCurSel(m_ComboZoom))));
	m_MapView.SetGrid(false);

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

	place(m_CheckGrid, x, button_y, MeasureText(m_Font, GetText(m_CheckGrid)) + 24, button_height);

	const int pause_width = MeasureText(m_Font, GetText(m_ButtonPause)) + 28;
	place(m_ButtonPause, width - 12 - pause_width, button_y, pause_width, button_height);

	// Map fills the rest, left of the inspector
	const int body_top = m_CommandBarHeight + 1;
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

			SetTextColor(hdc, control == m_CheckGrid ? GetSysColor(COLOR_WINDOWTEXT) : SecondaryTextColour);
			return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_BTNFACE));
		}

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

			break;
		}
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}
