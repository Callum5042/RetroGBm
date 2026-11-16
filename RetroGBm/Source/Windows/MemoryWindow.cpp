#include "Windows/MemoryWindow.h"
#include "Windows/MainWindow.h"
#include "Application.h"
#include "../resource.h"

#include <CommCtrl.h>
#include <windowsx.h>
#pragma comment(lib, "comctl32.lib")

#include <RetroGBm/Emulator.h>
#include <RetroGBm/Display.h>
#include <RetroGBm/Ppu.h>
#include <RetroGBm/Ram.h>
#include <RetroGBm/Cartridge/BaseCartridge.h>

#include <algorithm>
#include <filesystem>
#include <format>
#include <string>

namespace
{
	const wchar_t* ClassName = L"RetroGBmMemory";

	static MemoryWindow* GetWindow(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		MemoryWindow* window = nullptr;
		if (uMsg == WM_NCCREATE)
		{
			CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
			window = reinterpret_cast<MemoryWindow*>(pCreate->lpCreateParams);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
		}
		else
		{
			window = reinterpret_cast<MemoryWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		}

		return window;
	}

	static LRESULT CALLBACK MemoryWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		MemoryWindow* window = GetWindow(hwnd, msg, wParam, lParam);
		if (window == nullptr)
		{
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}

		return window->HandleMessage(hwnd, msg, wParam, lParam);
	}

	const COLORREF DividerColour = RGB(223, 223, 223);
	const COLORREF InspectorColour = RGB(250, 250, 250);

	struct BlockInfo
	{
		const wchar_t* name;
		MemoryHexView::Addressing addressing;

		// Where the CPU sees it, and its strip in the block list
		const wchar_t* range;
		COLORREF colour;
	};

	// Same order as MemoryWindow::Block
	const BlockInfo Blocks[] =
	{
		{ L"ROM", { 0x0000, 0x4000, true }, L"0000-7FFF", RGB(138, 109, 59) },
		{ L"Video RAM", { 0x8000, 0x2000, false }, L"8000-9FFF", RGB(46, 125, 50) },
		{ L"Cartridge RAM", { 0xA000, 0x2000, false }, L"A000-BFFF", RGB(122, 122, 122) },
		{ L"Work RAM", { 0xC000, 0x1000, true }, L"C000-DFFF", RGB(0, 95, 184) },
		{ L"OAM", { 0xFE00, 0, false }, L"FE00-FE9F", RGB(194, 24, 91) },
		{ L"High RAM", { 0xFF80, 0, false }, L"FF80-FFFE", RGB(111, 63, 181) },
	};

	const int BlockItemHeight = 30;
	const COLORREF BlockSelectionColour = RGB(204, 228, 247);
	const COLORREF BlockFocusColour = RGB(109, 166, 224);
	const COLORREF BlockRangeColour = RGB(95, 95, 95);

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

	const COLORREF InspectorLabelColour = RGB(92, 92, 92);
	const COLORREF InspectorTextColour = RGB(26, 26, 26);

	const wchar_t* InspectorHeadings[] = { L"Selection", L"8-bit value" };

	struct FieldInfo
	{
		const wchar_t* label;
		int section;
	};

	// Same order as MemoryWindow::Field
	const FieldInfo Fields[] =
	{
		{ L"Start", 0 },
		{ L"End", 0 },
		{ L"Length", 0 },
		{ L"Hex", 1 },
		{ L"Unsigned", 1 },
		{ L"Signed", 1 },
		{ L"Binary", 1 },
		{ L"ASCII", 1 },
	};
}

MemoryWindow::MemoryWindow(Application* application) : m_Application(application)
{
}

MemoryWindow::~MemoryWindow()
{
	this->Destroy();
}

void MemoryWindow::Create()
{
	if (IsOpen())
	{
		return;
	}

	// Use the system message font (Segoe UI 9pt on Windows 10/11)
	NONCLIENTMETRICS metrics = { sizeof(NONCLIENTMETRICS) };
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0);
	m_Font = CreateFontIndirect(&metrics.lfMessageFont);

	LOGFONT bold = metrics.lfMessageFont;
	bold.lfWeight = FW_SEMIBOLD;
	m_BoldFont = CreateFontIndirect(&bold);

	// Consolas for the inspector's values, the same as the hex view
	{
		HDC hdc = GetDC(NULL);
		LOGFONT mono = {};
		mono.lfHeight = -MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72);
		mono.lfWeight = FW_NORMAL;
		mono.lfQuality = CLEARTYPE_QUALITY;
		mono.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
		wcscpy_s(mono.lfFaceName, L"Consolas");
		m_MonoFont = CreateFontIndirect(&mono);
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
	this->Layout();
	this->UpdateStatus();
	this->SelectBlock(m_Block);

	ShowWindow(m_Hwnd, SW_SHOWNORMAL);
}

void MemoryWindow::Destroy()
{
	// WM_DESTROY clears m_Hwnd
	if (m_Hwnd != NULL)
	{
		DestroyWindow(m_Hwnd);
	}

	UnregisterClass(ClassName, GetModuleHandle(NULL));
	ReleaseResources();
}

void MemoryWindow::ReleaseResources()
{
	for (HFONT* font : { &m_Font, &m_BoldFont, &m_MonoFont })
	{
		if (*font != NULL)
		{
			DeleteObject(*font);
			*font = NULL;
		}
	}

	// The inspector's controls went with the window
	for (std::wstring& text : m_InspectorText)
	{
		text.clear();
	}

	if (m_InspectorBrush != NULL)
	{
		DeleteObject(m_InspectorBrush);
		m_InspectorBrush = NULL;
	}

	// Its window has gone with this one, this frees its fonts and back buffer
	m_HexView.Destroy();
}

void MemoryWindow::Focus()
{
	if (IsIconic(m_Hwnd))
	{
		ShowWindow(m_Hwnd, SW_RESTORE);
	}

	SetForegroundWindow(m_Hwnd);
}

void MemoryWindow::Update()
{
	if (!IsOpen())
	{
		return;
	}

	this->UpdateTitle();
	this->UpdateStatus();
	this->UpdateEmptyText();
	m_HexView.Refresh();

	// The selected bytes can change under the selection
	this->UpdateInspector();
}

void MemoryWindow::SelectBlock(Block block)
{
	m_Block = block;
	SendMessageW(m_BlockList, LB_SETCURSEL, static_cast<WPARAM>(block), 0);

	m_HexView.SetBlock(Blocks[static_cast<int>(block)].addressing, [this, block]() { return ReadBlock(block); });
	UpdateEmptyText();
	UpdateInspector();
}

void MemoryWindow::SetInspectorValue(Field field, const std::wstring& text)
{
	// Only set when changed, as this is called every frame
	const int index = static_cast<int>(field);
	if (m_InspectorText[index] != text)
	{
		m_InspectorText[index] = text;
		SetWindowTextW(m_InspectorValues[index], text.c_str());
	}
}

void MemoryWindow::UpdateInspector()
{
	const std::span<const uint8_t> data = ReadBlock(m_Block);
	if (data.empty() || !m_HexView.HasSelection())
	{
		for (int i = 0; i < m_FieldCount; ++i)
		{
			SetInspectorValue(static_cast<Field>(i), L"");
		}

		return;
	}

	// Selection
	const size_t start = (std::min)(m_HexView.GetSelectionStart(), data.size() - 1);
	const size_t end = (std::min)(m_HexView.GetSelectionEnd(), data.size() - 1);
	const size_t length = end - start + 1;

	SetInspectorValue(Field::Start, m_HexView.FormatAddress(start));
	SetInspectorValue(Field::End, m_HexView.FormatAddress(end));
	SetInspectorValue(Field::Length, length == 1 ? L"1 byte" : std::format(L"{} bytes", length));

	// The byte at the start of the selection
	const uint8_t value = data[start];
	const bool printable = value >= 0x20 && value < 0x7F;

	SetInspectorValue(Field::Hex8, std::format(L"${:02X}", value));
	SetInspectorValue(Field::Unsigned8, std::format(L"{}", value));
	SetInspectorValue(Field::Signed8, std::format(L"{}", static_cast<int8_t>(value)));
	SetInspectorValue(Field::Binary8, std::format(L"{:04b} {:04b}", value >> 4, value & 0x0F));
	SetInspectorValue(Field::Ascii8, printable ? std::format(L"'{}'", static_cast<wchar_t>(value)) : L"Not printable");
}

std::span<const uint8_t> MemoryWindow::ReadBlock(Block block) const
{
	Emulator* emulator = m_Application->GetEmulator();
	if (emulator == nullptr || !emulator->IsRunning() || emulator->GetCartridge() == nullptr)
	{
		return {};
	}

	// Only the first VRAM and WRAM banks exist on a DMG
	const bool colour = emulator->GetDisplay()->IsColour();

	switch (block)
	{
		case Block::Rom:
			return emulator->GetCartridge()->GetCartridgeData().data;

		case Block::VideoRam:
			return std::span<const uint8_t>(emulator->GetPpu()->GetVideoRam()).first(colour ? 0x4000 : 0x2000);

		case Block::CartridgeRam:
			return emulator->GetCartridge()->GetExternalRam();

		case Block::WorkRam:
		{
			const std::vector<uint8_t>& work_ram = emulator->GetRam()->GetWorkRam();
			return std::span<const uint8_t>(work_ram).first((std::min)(work_ram.size(), size_t(colour ? 0x8000 : 0x2000)));
		}

		case Block::Oam:
			return emulator->GetPpu()->GetOam();

		case Block::HighRam:
		{
			// $FF80-$FFFE
			const std::vector<uint8_t>& high_ram = emulator->GetRam()->GetHighRam();
			return std::span<const uint8_t>(high_ram).first((std::min)(high_ram.size(), size_t(0x7F)));
		}
	}

	return {};
}

void MemoryWindow::UpdateEmptyText()
{
	Emulator* emulator = m_Application->GetEmulator();
	const bool running = emulator != nullptr && emulator->IsRunning();

	if (!running)
	{
		m_HexView.SetEmptyText(L"No game loaded");
	}
	else if (m_Block == Block::CartridgeRam)
	{
		m_HexView.SetEmptyText(L"This cartridge has no RAM");
	}
	else
	{
		m_HexView.SetEmptyText(L"");
	}
}

void MemoryWindow::UpdateTitle()
{
	if (m_TitleFilename == m_Application->CurrentFilename)
	{
		return;
	}

	m_TitleFilename = m_Application->CurrentFilename;

	// Show the ROM file name without its extension, e.g. "Memory - Pokemon Red (UE)".
	// CurrentFilename is in the ANSI code page, which path converts back.
	const std::wstring rom_name = std::filesystem::path(m_TitleFilename).stem().wstring();
	const std::wstring title = rom_name.empty() ? L"Memory" : L"Memory - " + rom_name;
	SetWindowText(m_Hwnd, title.c_str());
}

void MemoryWindow::UpdateStatus()
{
	Emulator* emulator = m_Application->GetEmulator();
	const bool running = emulator != nullptr && emulator->IsRunning();
	const std::wstring text = running ? L"Live, updates every frame" : L"No game loaded";

	// Only set when changed, as this is called every frame
	wchar_t current[128] = {};
	SendMessageW(m_StatusBar, SB_GETTEXTW, 0, reinterpret_cast<LPARAM>(current));
	if (text != current)
	{
		SendMessageW(m_StatusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(text.c_str()));
	}
}

bool MemoryWindow::HandleDialogMessage(MSG* msg)
{
	return m_Hwnd != NULL && IsDialogMessage(m_Hwnd, msg);
}

void MemoryWindow::WindowCreate(int width, int height)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);

	WNDCLASS wc = {};
	wc.style = CS_VREDRAW | CS_HREDRAW;
	wc.lpfnWndProc = MemoryWndProc;
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

	m_Hwnd = CreateWindowEx(exStyle, ClassName, L"Memory", dwStyle, x, y, window_width, window_height, owner, NULL, hInstance, this);
	if (m_Hwnd == NULL)
	{
		throw std::exception("CreateWindow Failed");
	}
}

void MemoryWindow::CreateControls()
{
	m_StatusBar = CreateWindowExW(0, STATUSCLASSNAME, L"", WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 
		0, 0, 0, 0, m_Hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(m_ControlStatusBarId)), nullptr, nullptr);
	SendMessageW(m_StatusBar, WM_SETFONT, reinterpret_cast<WPARAM>(m_Font), TRUE);

	CreateCommandBar();

	// Block list on the left, under its heading
	m_LabelBlocks = CreateWindowExW(0, WC_STATICW, L"Memory blocks", WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX,
		0, 0, 0, 0, m_Hwnd, nullptr, nullptr, nullptr);
	SendMessageW(m_LabelBlocks, WM_SETFONT, reinterpret_cast<WPARAM>(m_Font), TRUE);

	// The names are kept as strings too, for screen readers
	m_BlockList = CreateWindowExW(0, WC_LISTBOXW, L"Memory blocks", WS_CHILD | WS_VISIBLE | WS_TABSTOP | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS,
		0, 0, 0, 0, m_Hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(m_ControlBlockListId)), nullptr, nullptr);
	SendMessageW(m_BlockList, WM_SETFONT, reinterpret_cast<WPARAM>(m_Font), TRUE);

	for (const BlockInfo& block : Blocks)
	{
		SendMessageW(m_BlockList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(block.name));
	}

	m_HexView.Create(m_Hwnd, m_ControlHexViewId);
	m_HexView.SelectionChanged = [this]() { UpdateInspector(); };

	CreateInspector();
}

void MemoryWindow::CreateCommandBar()
{
	auto create = [this](const wchar_t* type, const wchar_t* text, DWORD style, int id, HFONT font)
	{
		HWND control = CreateWindowW(type, text, WS_CHILD | WS_VISIBLE | style, 0, 0, 0, 0, m_Hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), nullptr, nullptr);
		SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
		return control;
	};

	// Disabled until each feature is built
	const DWORD off = WS_DISABLED;

	// Go to an address, and find bytes or text
	m_LabelGoTo = create(WC_STATICW, L"Go to", SS_CENTERIMAGE, 0, m_Font);
	m_EditGoTo = create(WC_EDITW, L"", WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL | off, m_ControlGoToId, m_MonoFont);
	Edit_SetCueBannerText(m_EditGoTo, L"$C040");

	m_LabelFind = create(WC_STATICW, L"Find", SS_CENTERIMAGE, 0, m_Font);
	m_EditFind = create(WC_EDITW, L"", WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL | off, m_ControlFindId, m_Font);
	Edit_SetCueBannerText(m_EditFind, L"Hex bytes or text");

	// Hex or text: push-like radio buttons act as a segmented control
	m_ButtonHex = create(WC_BUTTONW, L"Hex", WS_TABSTOP | WS_GROUP | BS_AUTORADIOBUTTON | BS_PUSHLIKE | off, m_ControlHexId, m_Font);
	m_ButtonText = create(WC_BUTTONW, L"Text", BS_AUTORADIOBUTTON | BS_PUSHLIKE | off, m_ControlTextId, m_Font);
	Button_SetCheck(m_ButtonHex, BST_CHECKED);

	// Bytes per row
	m_LabelColumns = create(WC_STATICW, L"Columns", SS_CENTERIMAGE | WS_GROUP, 0, m_Font);
	m_ComboColumns = create(WC_COMBOBOXW, L"", WS_TABSTOP | CBS_DROPDOWNLIST | off, m_ControlColumnsId, m_Font);
	for (const wchar_t* columns : { L"8", L"16", L"32" })
	{
		ComboBox_AddString(m_ComboColumns, columns);
	}
	ComboBox_SetCurSel(m_ComboColumns, 1);

	m_CheckHighlight = create(WC_BUTTONW, L"Highlight changes", WS_TABSTOP | BS_AUTOCHECKBOX | off, m_ControlHighlightId, m_Font);

	// Pause updates and export, on the right
	m_ButtonPause = create(WC_BUTTONW, L"Pause updates", WS_TABSTOP | BS_AUTOCHECKBOX | BS_PUSHLIKE | off, m_ControlPauseId, m_Font);
	m_ButtonExport = create(WC_BUTTONW, L"Export range\x2026", WS_TABSTOP | BS_PUSHBUTTON | off, m_ControlExportId, m_Font);
}

void MemoryWindow::LayoutCommandBar(int width)
{
	const int button_height = 26;
	const int button_y = (m_CommandBarHeight - button_height) / 2;
	const int field_height = 23;
	const int field_y = (m_CommandBarHeight - field_height) / 2;
	const int group_gap = 20;

	int x = 12;
	auto label = [&](HWND control)
	{
		const int w = MeasureText(m_Font, GetText(control));
		MoveWindow(control, x, button_y, w, button_height, TRUE);
		x += w + 8;
	};

	label(m_LabelGoTo);
	MoveWindow(m_EditGoTo, x, field_y, 96, field_height, TRUE);
	x += 96 + group_gap;

	label(m_LabelFind);
	MoveWindow(m_EditFind, x, field_y, 168, field_height, TRUE);
	x += 168 + group_gap;

	for (HWND button : { m_ButtonHex, m_ButtonText })
	{
		const int w = MeasureText(m_Font, GetText(button)) + 24;
		MoveWindow(button, x, button_y, w, button_height, TRUE);
		x += w + 2;
	}

	x += group_gap;
	label(m_LabelColumns);
	MoveWindow(m_ComboColumns, x, field_y, 56, 200, TRUE);
	x += 56 + group_gap;

	MoveWindow(m_CheckHighlight, x, button_y, MeasureText(m_Font, GetText(m_CheckHighlight)) + 24, button_height, TRUE);

	const int export_width = MeasureText(m_Font, GetText(m_ButtonExport)) + 28;
	const int pause_width = MeasureText(m_Font, GetText(m_ButtonPause)) + 28;
	MoveWindow(m_ButtonExport, width - 12 - export_width, button_y, export_width, button_height, TRUE);
	MoveWindow(m_ButtonPause, width - 12 - export_width - 8 - pause_width, button_y, pause_width, button_height, TRUE);
}

void MemoryWindow::DrawBlockItem(const DRAWITEMSTRUCT* item)
{
	if (item->itemID >= static_cast<UINT>(Block::Count))
	{
		return;
	}

	const BlockInfo& block = Blocks[item->itemID];
	const bool selected = (item->itemState & ODS_SELECTED) != 0;
	const bool focused = (item->itemState & ODS_FOCUS) != 0;

	// The list's DC: put back the pen, brush and font chosen here once drawn
	HDC hdc = item->hDC;
	const int saved = SaveDC(hdc);

	RECT rect = item->rcItem;
	FillRect(hdc, &rect, m_InspectorBrush);

	// Selected: a rounded light blue row, outlined while the list has focus
	if (selected || focused)
	{
		SelectObject(hdc, GetStockObject(DC_BRUSH));
		SelectObject(hdc, GetStockObject(DC_PEN));
		SetDCBrushColor(hdc, selected ? BlockSelectionColour : InspectorColour);
		SetDCPenColor(hdc, focused ? BlockFocusColour : BlockSelectionColour);
		RoundRect(hdc, rect.left, rect.top + 1, rect.right, rect.bottom - 1, 8, 8);
	}

	// Colour strip
	const int strip_height = 18;
	const int strip_top = rect.top + (rect.bottom - rect.top - strip_height) / 2;
	SelectObject(hdc, GetStockObject(DC_BRUSH));
	SelectObject(hdc, GetStockObject(NULL_PEN));
	SetDCBrushColor(hdc, block.colour);
	RoundRect(hdc, rect.left + 10, strip_top, rect.left + 15, strip_top + strip_height + 1, 4, 4);

	SetBkMode(hdc, TRANSPARENT);

	// Address range on the right, then the name in what is left
	RECT range_rect = rect;
	range_rect.right -= 10;
	SelectObject(hdc, m_MonoFont);
	SetTextColor(hdc, BlockRangeColour);
	DrawTextW(hdc, block.range, -1, &range_rect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	const int range_width = MeasureText(m_MonoFont, block.range);

	RECT name_rect = rect;
	name_rect.left += 22;
	name_rect.right = range_rect.right - range_width - 8;
	SelectObject(hdc, selected ? m_BoldFont : m_Font);
	SetTextColor(hdc, InspectorTextColour);
	DrawTextW(hdc, block.name, -1, &name_rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

	RestoreDC(hdc, saved);
}

void MemoryWindow::CreateInspector()
{
	auto create_static = [this](int id, const wchar_t* text, HFONT font)
	{
		HWND hwnd = CreateWindowExW(0, WC_STATICW, text, WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX | SS_ENDELLIPSIS,
			0, 0, 0, 0, m_Hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), nullptr, nullptr);
		SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
		return hwnd;
	};

	for (int i = 0; i < m_InspectorSectionCount; ++i)
	{
		m_InspectorHeadings[i] = create_static(m_ControlInspectorHeadingId + i, InspectorHeadings[i], m_BoldFont);
	}

	for (int i = 0; i < m_FieldCount; ++i)
	{
		m_InspectorLabels[i] = create_static(m_ControlInspectorLabelId + i, Fields[i].label, m_Font);
		m_InspectorValues[i] = create_static(m_ControlInspectorValueId + i, L"", m_MonoFont);
	}
}

void MemoryWindow::LayoutInspector(int left, int top)
{
	const int width = m_InspectorWidth - 32;
	const int label_width = 80;
	const int heading_height = 26;
	const int row_height = 22;
	const int section_gap = 16;

	int y = top;
	for (int section = 0; section < m_InspectorSectionCount; ++section)
	{
		MoveWindow(m_InspectorHeadings[section], left, y, width, 20, TRUE);
		y += heading_height;

		for (int i = 0; i < m_FieldCount; ++i)
		{
			if (Fields[i].section != section)
			{
				continue;
			}

			MoveWindow(m_InspectorLabels[i], left, y + 1, label_width, 20, TRUE);
			MoveWindow(m_InspectorValues[i], left + label_width, y, width - label_width, 20, TRUE);
			y += row_height;
		}

		y += section_gap;
	}
}

void MemoryWindow::Layout()
{
	// Status bar sizes itself to the bottom of the window
	SendMessageW(m_StatusBar, WM_SIZE, 0, 0);

	RECT client;
	GetClientRect(m_Hwnd, &client);

	LayoutCommandBar(client.right);

	RECT status = {};
	GetWindowRect(m_StatusBar, &status);
	const int status_height = status.bottom - status.top;

	// Between the command bar's divider and the status bar
	const int top = m_CommandBarHeight + 1;
	const int bottom = (std::max)(top, static_cast<int>(client.bottom) - status_height);

	// Block list under its heading, then a 1px divider, then the hex view up to the inspector's divider
	const int padding = 8;
	const int heading_height = 22;
	MoveWindow(m_LabelBlocks, padding + 10, top + 10, m_BlockListWidth - padding * 2 - 10, 16, TRUE);

	const int list_top = top + 10 + heading_height;
	MoveWindow(m_BlockList, padding, list_top, m_BlockListWidth - padding * 2, (std::max)(0, bottom - list_top - padding), TRUE);

	const int hex_left = m_BlockListWidth + 1;
	const int hex_right = (std::max)(hex_left, static_cast<int>(client.right) - m_InspectorWidth);
	MoveWindow(m_HexView.GetHwnd(), hex_left, top, hex_right - hex_left, bottom - top, TRUE);

	// Inspector, right of its 1px divider
	LayoutInspector(hex_right + 1 + 16, top + 16);

	InvalidateRect(m_Hwnd, NULL, TRUE);
}

void MemoryWindow::PaintBackground(HDC hdc)
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

	// Block list panel and its divider
	RECT block_panel = client;
	block_panel.top = divider.bottom;
	block_panel.right = m_BlockListWidth;
	FillRect(hdc, &block_panel, m_InspectorBrush);

	RECT block_divider = block_panel;
	block_divider.left = block_panel.right;
	block_divider.right = block_divider.left + 1;
	SetDCBrushColor(hdc, DividerColour);
	FillRect(hdc, &block_divider, dc_brush);

	// White memory area
	RECT body = client;
	body.top = divider.bottom;
	body.left = block_divider.right;
	body.right = client.right - m_InspectorWidth;
	FillRect(hdc, &body, GetSysColorBrush(COLOR_WINDOW));

	// 1px divider and the inspector
	RECT inspector_divider = body;
	inspector_divider.left = body.right;
	inspector_divider.right = inspector_divider.left + 1;
	SetDCBrushColor(hdc, DividerColour);
	FillRect(hdc, &inspector_divider, dc_brush);

	RECT inspector = body;
	inspector.left = inspector_divider.right;
	inspector.right = client.right;
	FillRect(hdc, &inspector, m_InspectorBrush);
}

LRESULT MemoryWindow::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_CLOSE:
			DestroyWindow(hwnd);
			return 0;

		case WM_DESTROY:
			m_Hwnd = NULL;

			if (m_Application->GetMainWindow() != nullptr)
			{
				m_Application->GetMainWindow()->ToggleMemoryWindowMenuItem(false);
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

		case WM_ERASEBKGND:
			PaintBackground(reinterpret_cast<HDC>(wParam));
			return 1;

		case WM_MEASUREITEM:
		{
			// Sent while the list is being created, before m_BlockList is set
			MEASUREITEMSTRUCT* measure = reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);
			if (measure->CtlID == static_cast<UINT>(m_ControlBlockListId))
			{
				measure->itemHeight = BlockItemHeight;
				return TRUE;
			}

			break;
		}

		case WM_DRAWITEM:
		{
			const DRAWITEMSTRUCT* draw = reinterpret_cast<const DRAWITEMSTRUCT*>(lParam);
			if (draw->CtlID == static_cast<UINT>(m_ControlBlockListId))
			{
				DrawBlockItem(draw);
				return TRUE;
			}

			break;
		}

		case WM_CTLCOLORSTATIC:
		{
			// Inspector text sits on the inspector's colour, with the labels in grey
			const int id = GetDlgCtrlID(reinterpret_cast<HWND>(lParam));
			if (id >= m_ControlInspectorHeadingId && id < m_ControlInspectorValueId + m_FieldCount)
			{
				const bool label = id >= m_ControlInspectorLabelId && id < m_ControlInspectorValueId;
				HDC hdc = reinterpret_cast<HDC>(wParam);
				SetTextColor(hdc, label ? InspectorLabelColour : InspectorTextColour);
				SetBkColor(hdc, InspectorColour);
				return reinterpret_cast<LRESULT>(m_InspectorBrush);
			}

			HWND control = reinterpret_cast<HWND>(lParam);
			if (control == m_LabelBlocks)
			{
				HDC hdc = reinterpret_cast<HDC>(wParam);
				SetTextColor(hdc, InspectorLabelColour);
				SetBkColor(hdc, InspectorColour);
				return reinterpret_cast<LRESULT>(m_InspectorBrush);
			}

			// Command bar labels and the checkbox sit on the bar's grey
			if (control == m_LabelGoTo || control == m_LabelFind || control == m_LabelColumns || control == m_CheckHighlight)
			{
				HDC hdc = reinterpret_cast<HDC>(wParam);
				SetBkMode(hdc, TRANSPARENT);
				SetTextColor(hdc, control == m_CheckHighlight ? GetSysColor(COLOR_WINDOWTEXT) : InspectorLabelColour);
				return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_BTNFACE));
			}

			break;
		}

		case WM_CTLCOLORLISTBOX:
			// The block list sits on the panel's colour
			if (reinterpret_cast<HWND>(lParam) == m_BlockList)
			{
				SetBkColor(reinterpret_cast<HDC>(wParam), InspectorColour);
				return reinterpret_cast<LRESULT>(m_InspectorBrush);
			}

			break;

		case WM_COMMAND:
			// Esc closes the window, as in the other tool windows
			if (LOWORD(wParam) == IDCANCEL)
			{
				DestroyWindow(hwnd);
				return 0;
			}

			if (LOWORD(wParam) == m_ControlBlockListId && HIWORD(wParam) == LBN_SELCHANGE)
			{
				const LRESULT index = SendMessageW(m_BlockList, LB_GETCURSEL, 0, 0);
				if (index >= 0 && index < static_cast<LRESULT>(Block::Count))
				{
					SelectBlock(static_cast<Block>(index));
				}

				return 0;
			}

			break;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}
