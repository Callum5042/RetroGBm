#include "Windows/TileWindow.h"
#include "Windows/MainWindow.h"
#include "Application.h"
#include "../resource.h"

#include <CommCtrl.h>
#include <windowsx.h>
#pragma comment(lib, "comctl32.lib")

#include <RetroGBm/Emulator.h>
#include <RetroGBm/Display.h>

#include <filesystem>
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

	// Palette list entries store what they select as item data
	enum class PaletteKind : int { Background = 0, Object = 1, Greyscale = 2 };

	LPARAM PaletteItemData(PaletteKind kind, int index)
	{
		return (static_cast<int>(kind) << 8) | index;
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
		ReleaseDC(NULL, hdc);
	}

	m_InspectorBrush = CreateSolidBrush(InspectorColour);

	INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_STANDARD_CLASSES | ICC_BAR_CLASSES };
	InitCommonControlsEx(&icex);

	this->WindowCreate(m_ClientWidth, m_ClientHeight);

	// Show the ROM file name without its extension in the title, e.g. "Tile Data - Pokemon Red (UE)"
	{
		const std::wstring rom_name = std::filesystem::path(m_Application->CurrentFilename).stem().wstring();
		if (!rom_name.empty())
		{
			SetWindowText(m_Hwnd, (L"Tile Data \x2014 " + rom_name).c_str());
		}
	}

	this->CreateControls();

	Emulator* emulator = m_Application->GetEmulator();
	m_IsColour = emulator != nullptr && emulator->IsRunning() && emulator->GetDisplay()->IsColour();
	this->ApplyMode();

	this->Layout();
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

	for (HFONT* font : { &m_Font, &m_HeadingFont })
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
	}

	this->UpdateStatus();
}

bool TileWindow::HandleDialogMessage(MSG* msg)
{
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

	// Inspector
	m_LabelTileHeading = create(L"STATIC", L"No tile selected", SS_LEFT | SS_ENDELLIPSIS, 0, m_HeadingFont);
	m_LabelTileHint = create(L"STATIC", L"Hover over a tile to see its address, or click one to pin it here.", SS_LEFT, 0, m_Font);

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

	int parts[] = { width - 280 > 0 ? width - 280 : 0, -1 };
	SendMessageW(m_StatusBar, SB_SETPARTS, 2, reinterpret_cast<LPARAM>(parts));

	HDWP defer = BeginDeferWindowPos(16);
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
	place(m_LabelTileHeading, inspector_x, body_top + 16, inspector_width, 28);
	place(m_LabelTileHint, inspector_x, body_top + 16 + 32, inspector_width, 48);

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

			if (control == m_LabelTileHeading || control == m_LabelTileHint)
			{
				SetTextColor(hdc, control == m_LabelTileHint ? SecondaryTextColour : GetSysColor(COLOR_WINDOWTEXT));
				return reinterpret_cast<LRESULT>(m_InspectorBrush);
			}

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

			if (id == m_ControlPauseId && HIWORD(wParam) == BN_CLICKED)
			{
				UpdateStatus();
				return 0;
			}

			break;
		}
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}
