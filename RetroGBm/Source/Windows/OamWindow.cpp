#include "Windows/OamWindow.h"
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

	std::wstring GetText(HWND control)
	{
		std::wstring text(static_cast<size_t>(GetWindowTextLength(control)), L'\0');
		GetWindowText(control, text.data(), static_cast<int>(text.size()) + 1);
		return text;
	}

	// Display colours are 0xAABBGGRR, the views want 0x00RRGGBB
	uint32_t ToViewColour(uint32_t colour)
	{
		return ((colour & 0xFF) << 16) | (colour & 0xFF00) | ((colour >> 16) & 0xFF);
	}

	// "104-111, 120" from lines in order
	std::wstring LineRanges(const std::vector<int>& lines)
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

	const COLORREF DividerColour = RGB(223, 223, 223);
	const COLORREF SecondaryTextColour = RGB(92, 92, 92);
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
	}

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

	for (HFONT* font : { &m_Font })
	{
		if (*font != NULL)
		{
			DeleteObject(*font);
			*font = NULL;
		}
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

void OamWindow::RenderObjects()
{
	if (!m_HasSnapshot || m_Snapshot.tiles[0].size() != TileData::BytesPerBank)
	{
		m_GridView.SetEmptyText(L"Load a game to see its objects.");
		m_GridView.SetCells({});
		m_LineScan = {};
		return;
	}

	const int height = ObjectHeight();
	const uint8_t* bank0 = m_Snapshot.tiles[0].data();
	const uint8_t* bank1 = m_Snapshot.tiles[1].size() == TileData::BytesPerBank ? m_Snapshot.tiles[1].data() : nullptr;
	m_LineScan = Oam::ScanLines(m_Snapshot.oam.data(), height);

	std::vector<OamGridView::Cell> cells;
	cells.reserve(Oam::Count);
	for (int index = 0; index < Oam::Count; ++index)
	{
		const Oam::Object object = Oam::Decode(m_Snapshot.oam.data(), index);
		const std::vector<uint8_t> image = Oam::DecodeImage(object, height, bank0, bank1);
		const std::array<uint32_t, 4>& colours = m_Snapshot.colours[PaletteOf(object)];

		OamGridView::Cell cell;
		cell.index = index;
		cell.height = height;
		cell.pixels.resize(image.size());
		for (size_t i = 0; i < image.size(); ++i)
		{
			// Colour 0 is transparent for objects
			cell.pixels[i] = image[i] == 0 ? 0 : (0xFF000000 | colours[image[i]]);
		}

		// "$0E · P4" on CGB, "$0E · OBP1" on DMG
		const std::wstring palette = m_IsColour ? std::format(L"P{}", PaletteOf(object)) : PaletteName(object);
		cell.caption = std::format(L"${:02X} \x00B7 {}", Oam::FirstTile(object.tile, height), palette);
		cell.hidden = !Oam::IsVisible(object, height);
		cell.dropped = !m_LineScan.dropped[index].empty();
		cells.push_back(std::move(cell));
	}

	m_GridView.SetCells(cells);
}

void OamWindow::UpdateHoverStatus()
{
	const int index = m_GridView.GetHover();

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

	// Size: LCDC.2 by default, as the game changes it
	m_LabelSize = create(L"STATIC", L"Size", SS_CENTERIMAGE, 0, m_Font);
	m_ComboSize = create(WC_COMBOBOX, L"", WS_TABSTOP | WS_GROUP | CBS_DROPDOWNLIST, m_ControlSizeId, m_Font);
	for (const wchar_t* text : { L"Auto (LCDC.2)", L"8\x00D7" L"8", L"8\x00D7" L"16" })
	{
		ComboBox_AddString(m_ComboSize, text);
	}
	ComboBox_SetCurSel(m_ComboSize, 0);

	// Pause updates, on the right
	m_ButtonPause = create(L"BUTTON", L"Pause updates", WS_TABSTOP | BS_AUTOCHECKBOX | BS_PUSHLIKE, m_ControlPauseId, m_Font);

	// Objects
	m_GridView.Create(m_Hwnd, m_ControlGridId);
	m_GridView.HoverChanged = [this]() { UpdateHoverStatus(); };

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

	HDWP defer = BeginDeferWindowPos(8);
	auto place = [&defer](HWND control, int x, int y, int w, int h)
	{
		defer = DeferWindowPos(defer, control, NULL, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
	};

	// Command bar
	const int button_height = 26;
	const int button_y = (m_CommandBarHeight - button_height) / 2;
	const int combo_y = (m_CommandBarHeight - 23) / 2;

	int x = 12;
	const int label_width = MeasureText(m_Font, GetText(m_LabelSize));
	place(m_LabelSize, x, button_y, label_width, button_height);
	x += label_width + 8;
	place(m_ComboSize, x, combo_y, 120, 200);

	const int pause_width = MeasureText(m_Font, GetText(m_ButtonPause)) + 28;
	place(m_ButtonPause, width - 12 - pause_width, button_y, pause_width, button_height);

	// Objects fill the rest
	const int body_top = m_CommandBarHeight + 1;
	const int body_height = client.bottom - status_height - body_top;
	place(m_GridView.GetHwnd(), 0, body_top, width, body_height > 0 ? body_height : 0);

	EndDeferWindowPos(defer);

	InvalidateRect(m_Hwnd, NULL, TRUE);
}

void OamWindow::PaintBackground(HDC hdc)
{
	RECT client;
	GetClientRect(m_Hwnd, &client);

	// Grey command bar
	RECT command_bar = client;
	command_bar.bottom = m_CommandBarHeight;
	FillRect(hdc, &command_bar, GetSysColorBrush(COLOR_BTNFACE));

	// 1px divider under the command bar
	RECT divider = client;
	divider.top = m_CommandBarHeight;
	divider.bottom = divider.top + 1;
	SetDCBrushColor(hdc, DividerColour);
	FillRect(hdc, &divider, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
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
			RECT minimum = { 0, 0, 640, 420 };
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
				SetFocus(m_LastFocus != NULL ? m_LastFocus : m_GridView.GetHwnd());
			}

			return 0;
		}

		case WM_CTLCOLORSTATIC:
		{
			HDC hdc = reinterpret_cast<HDC>(wParam);
			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, SecondaryTextColour);
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

			if (id == m_ControlSizeId && code == CBN_SELCHANGE)
			{
				RenderObjects();
				UpdateStatus();
				return 0;
			}

			break;
		}
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}
