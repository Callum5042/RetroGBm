#include "Windows/PropertiesWindow.h"
#include "Application.h"
#include "Utilities/Utilities.h"

#include <CommCtrl.h>
#include <uxtheme.h>
#pragma comment(lib, "uxtheme.lib")

#include <format>
#include <fstream>
#include <string>

// Set theme
// https://docs.microsoft.com/en-gb/windows/win32/controls/cookbook-overview?redirectedfrom=MSDN
#pragma comment(linker,"\"/manifestdependency:type='win32' name = 'Microsoft.Windows.Common-Controls' version = '6.0.0.0' processorArchitecture = '*' publicKeyToken = '6595b64144ccf1df' language = '*'\"")

namespace
{
	static PropertiesWindow* GetWindow(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		PropertiesWindow* window = nullptr;
		if (uMsg == WM_NCCREATE)
		{
			CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
			window = reinterpret_cast<PropertiesWindow*>(pCreate->lpCreateParams);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
		}
		else
		{
			window = reinterpret_cast<PropertiesWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		}

		return window;
	}

	static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		PropertiesWindow* window = GetWindow(hwnd, msg, wParam, lParam);
		if (window == nullptr)
		{
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}

		return window->HandleMessage(hwnd, msg, wParam, lParam);
	}

	const COLORREF TextGrey = RGB(92, 92, 92);
	const COLORREF TextGreen = RGB(15, 123, 15);
	const COLORREF TextRed = RGB(196, 43, 28);
	const COLORREF PageBorder = RGB(200, 200, 200);
	const COLORREF DividerColour = RGB(229, 229, 229);

	const wchar_t* Divider = L"-";

	// Header text is ASCII, so a plain widen is enough
	std::wstring Widen(const std::string& text)
	{
		return Utilities::ConvertToWString(text);
	}

	std::wstring Hex8(uint8_t value)
	{
		return Widen(std::format("${:02X}", value));
	}

	std::wstring YesNo(bool value)
	{
		return value ? L"Yes" : L"No";
	}

	std::wstring FormatTimePlayed(int minutes)
	{
		return Widen(std::format("{}h {:02}m", minutes / 60, minutes % 60));
	}

	std::wstring FormatLastPlayed(const std::string& last_played)
	{
		// Stored as local time with its offset, e.g. 2026-11-13T09:39:10.016+0000
		if (last_played.size() < 16 || last_played[4] != '-' || last_played[10] != 'T')
		{
			return L"Unknown";
		}

		const std::string day = last_played.substr(8, 2);
		const std::string month = last_played.substr(5, 2);
		const std::string year = last_played.substr(0, 4);
		const std::string time = last_played.substr(11, 5);
		return Widen(day + "/" + month + "/" + year + " " + time);
	}

	bool ReadFile(const std::filesystem::path& path, std::vector<uint8_t>* data)
	{
		std::ifstream file(path, std::ios::binary | std::ios::ate);
		if (!file)
		{
			return false;
		}

		std::streamsize size = file.tellg();
		if (size < 0)
		{
			return false;
		}

		data->resize(static_cast<size_t>(size));
		file.seekg(0, std::ios::beg);
		return static_cast<bool>(file.read(reinterpret_cast<char*>(data->data()), size));
	}

	HWND CreateLabel(HWND parent, HFONT font, DWORD style)
	{
		HWND label = CreateWindowW(L"STATIC", L"", WS_CHILD | style, 0, 0, 0, 0, parent, nullptr, nullptr, nullptr);
		SendMessageW(label, WM_SETFONT, (WPARAM)font, TRUE);
		return label;
	}

	HWND CreateButton(HWND parent, HFONT font, const wchar_t* text, DWORD style, UINT id, int x, int y, int width, int height)
	{
		HWND button = CreateWindowW(L"BUTTON", text, WS_CHILD | WS_VISIBLE | WS_TABSTOP | style,
			x, y, width, height, parent, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)), nullptr, nullptr);
		SendMessageW(button, WM_SETFONT, (WPARAM)font, TRUE);
		return button;
	}
}

PropertiesWindow::PropertiesWindow(Application* application) : m_Application(application)
{
}

PropertiesWindow::~PropertiesWindow()
{
	this->Destroy();
}

void PropertiesWindow::Show(const std::filesystem::path& rom_path)
{
	HWND owner = m_Hwnd != NULL ? m_Hwnd : m_Application->GetMainWindow()->GetHwnd();

	std::vector<uint8_t> data;
	if (!ReadFile(rom_path, &data))
	{
		MessageBoxW(owner, (L"Could not open " + rom_path.filename().wstring() + L".").c_str(), L"Properties", MB_OK | MB_ICONWARNING);
		return;
	}

	std::optional<CartridgeHeader> header = ReadCartridgeHeader(data);
	if (!header.has_value())
	{
		MessageBoxW(owner, (rom_path.filename().wstring() + L" is too small to be a Game Boy ROM.").c_str(), L"Properties", MB_OK | MB_ICONWARNING);
		return;
	}

	m_RomPath = rom_path;
	m_Header = header.value();

	if (m_Hwnd == NULL)
	{
		Create();
	}

	UpdateContent();
	SetForegroundWindow(m_Hwnd);
}

void PropertiesWindow::Create()
{
	this->WindowCreate(m_ClientWidth, m_ClientHeight);

	// Use the system message font (Segoe UI 9pt on Windows 10/11)
	{
		NONCLIENTMETRICS metrics = { sizeof(NONCLIENTMETRICS) };
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0);
		m_Font = CreateFontIndirect(&metrics.lfMessageFont);

		LOGFONT title = metrics.lfMessageFont;
		title.lfWeight = FW_SEMIBOLD;
		title.lfHeight = MulDiv(title.lfHeight, 3, 2);
		m_TitleFont = CreateFontIndirect(&title);
	}

	INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES };
	InitCommonControlsEx(&icex);

	const int buttons_y = m_ClientHeight - m_Margin - m_ButtonHeight;

	// Tab strip only. The page under it is drawn by this window, so it can be white.
	m_Tabs = CreateWindowW(WC_TABCONTROL, L"",
		WS_CHILD | WS_VISIBLE | WS_TABSTOP,
		m_Margin, 10, m_ClientWidth - m_Margin * 2, 40,
		m_Hwnd, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(m_ControlTabsId)), nullptr, nullptr);

	SendMessageW(m_Tabs, WM_SETFONT, (WPARAM)m_Font, TRUE);

	TCITEMW tab = { 0 };
	tab.mask = TCIF_TEXT;
	tab.pszText = const_cast<wchar_t*>(L"General");
	TabCtrl_InsertItem(m_Tabs, 0, &tab);
	tab.pszText = const_cast<wchar_t*>(L"Header");
	TabCtrl_InsertItem(m_Tabs, 1, &tab);

	// Cut the control down to its tabs, overlapping the page's top border
	RECT item;
	TabCtrl_GetItemRect(m_Tabs, 0, &item);
	SetWindowPos(m_Tabs, NULL, 0, 0, m_ClientWidth - m_Margin * 2, item.bottom + 2, SWP_NOMOVE | SWP_NOZORDER);

	m_Page = { m_Margin, 10 + item.bottom, m_ClientWidth - m_Margin, buttons_y - m_Margin };

	// General tab
	m_LabelTitle = CreateLabel(m_Hwnd, m_TitleFont, SS_LEFT | SS_ENDELLIPSIS | SS_NOPREFIX);
	m_LabelPath = CreateLabel(m_Hwnd, m_Font, SS_LEFT | SS_PATHELLIPSIS | SS_NOPREFIX);

	// Header tab: fills the page inside its border
	m_HeaderList = CreateWindowExW(0, WC_LISTVIEW, L"",
		WS_CHILD | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_NOSORTHEADER | LVS_SHOWSELALWAYS,
		m_Page.left + 1, m_Page.top + 1, (m_Page.right - m_Page.left) - 2, (m_Page.bottom - m_Page.top) - 2,
		m_Hwnd, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(m_ControlHeaderListId)), nullptr, nullptr);

	SendMessageW(m_HeaderList, WM_SETFONT, (WPARAM)m_Font, TRUE);
	SetWindowTheme(m_HeaderList, L"Explorer", NULL);
	ListView_SetExtendedListViewStyle(m_HeaderList, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	RECT list_client;
	GetClientRect(m_HeaderList, &list_client);

	LVCOLUMNW column = { 0 };
	column.mask = LVCF_TEXT | LVCF_WIDTH;
	column.pszText = const_cast<wchar_t*>(L"Address");
	column.cx = 70;
	ListView_InsertColumn(m_HeaderList, 0, &column);
	column.pszText = const_cast<wchar_t*>(L"Field");
	column.cx = 130;
	ListView_InsertColumn(m_HeaderList, 1, &column);
	column.pszText = const_cast<wchar_t*>(L"Value");
	column.cx = (list_client.right - list_client.left) - 70 - 130;
	ListView_InsertColumn(m_HeaderList, 2, &column);

	// Buttons on the grey, under the page. Close uses IDCANCEL so Esc shares its path.
	m_ButtonCopy = CreateButton(m_Hwnd, m_Font, L"Copy info", BS_PUSHBUTTON, m_ControlCopyButtonId,
		m_ClientWidth - m_Margin - m_ButtonWidth * 2 - 8, buttons_y, m_ButtonWidth, m_ButtonHeight);

	m_ButtonClose = CreateButton(m_Hwnd, m_Font, L"Close", BS_DEFPUSHBUTTON, IDCANCEL,
		m_ClientWidth - m_Margin - m_ButtonWidth, buttons_y, m_ButtonWidth, m_ButtonHeight);

	m_SelectedTab = 0;
	SetFocus(m_Tabs);
}

void PropertiesWindow::Destroy()
{
	// WM_DESTROY clears m_Hwnd
	if (m_Hwnd != NULL)
	{
		DestroyWindow(m_Hwnd);
	}

	HINSTANCE hInstance = GetModuleHandle(NULL);
	UnregisterClass(m_RegisterClassName.c_str(), hInstance);

	for (HFONT* font : { &m_Font, &m_TitleFont })
	{
		if (*font != NULL)
		{
			DeleteObject(*font);
			*font = NULL;
		}
	}
}

bool PropertiesWindow::HandleDialogMessage(MSG* msg)
{
	return m_Hwnd != NULL && IsDialogMessage(m_Hwnd, msg);
}

std::vector<std::pair<std::wstring, std::wstring>> PropertiesWindow::BuildGeneralRows() const
{
	const CartridgeHeader& header = m_Header;

	// Play history from the profile, matched by file name like the ROM list
	std::wstring last_played = L"Never played";
	std::wstring time_played = L"No time played";
	for (const auto& game : m_Application->ProfileDataList.gameData)
	{
		if (!m_RomPath.empty() && m_RomPath.filename() == std::filesystem::path(game.filename))
		{
			last_played = FormatLastPlayed(game.lastPlayed);
			time_played = FormatTimePlayed(game.totalPlayTimeMinutes);
			break;
		}
	}

	std::wstring checksums;
	if (header.IsHeaderChecksumValid() && header.IsGlobalChecksumValid())
	{
		checksums = L"\x2713 Header and global OK";
	}
	else if (header.IsHeaderChecksumValid())
	{
		checksums = L"\x2717 Global doesn't match";
	}
	else if (header.IsGlobalChecksumValid())
	{
		checksums = L"\x2717 Header doesn't match";
	}
	else
	{
		checksums = L"\x2717 Header and global don't match";
	}

	std::vector<std::pair<std::wstring, std::wstring>> rows =
	{
		{ L"Hardware", Widen(CartridgeHardwareName(header)) },
		{ L"Cartridge", Widen(CartridgeTypeName(header.cartridge_type)) },
		{ L"Publisher", Widen(CartridgePublisherName(header)) },
		{ L"ROM", Widen(CartridgeRomSizeName(header.rom_size_code)) },
		{ L"Cartridge RAM", Widen(CartridgeRamName(header)) },
		{ L"Battery", YesNo(CartridgeTypeHasBattery(header.cartridge_type)) },
		{ L"Real-time clock", YesNo(CartridgeTypeHasTimer(header.cartridge_type)) },
	};

	// Only worth a row when the game won't run
	if (!IsCartridgeTypeSupported(header.cartridge_type))
	{
		rows.push_back({ L"RetroGBm", L"Cartridge type not supported yet" });
	}

	rows.push_back({ Divider, L"" });
	rows.push_back({ L"Last played", last_played });
	rows.push_back({ L"Time played", time_played });
	rows.push_back({ L"Checksums", checksums });
	return rows;
}

std::vector<std::pair<std::wstring, std::wstring>> PropertiesWindow::BuildHeaderRows(std::vector<std::wstring>* addresses) const
{
	const CartridgeHeader& header = m_Header;

	auto with_code = [](uint8_t code, const std::string& name)
	{
		return Hex8(code) + L"  " + Widen(name);
	};

	std::wstring new_licensee = L"\"" + Widen(header.new_licensee_code) + L"\"  " +
		(header.UsesNewLicensee() ? Widen(CartridgeNewLicenseeName(header.new_licensee_code)) : L"Not used");

	std::wstring header_checksum = Hex8(header.header_checksum) + L"  " +
		(header.IsHeaderChecksumValid() ? L"\x2713 Valid" : L"\x2717 Doesn't match, expected " + Hex8(header.computed_header_checksum));

	std::wstring global_checksum = Widen(std::format("${:04X}", header.global_checksum)) + L"  " +
		(header.IsGlobalChecksumValid() ? L"\x2713 Valid" : Widen(std::format("Doesn't match, expected ${:04X}", header.computed_global_checksum)).insert(0, L"\x2717 "));

	std::vector<std::pair<std::wstring, std::wstring>> rows =
	{
		{ L"Logo", header.logo_matches ? L"\x2713 Matches" : L"\x2717 Doesn't match" },
		{ L"Title", header.title.empty() ? L"None" : Widen(header.title) },
		{ L"Manufacturer code", header.manufacturer_code.empty() ? L"None" : Widen(header.manufacturer_code) },
		{ L"CGB flag", with_code(header.cgb_flag, CartridgeCgbFlagName(header.cgb_flag)) },
		{ L"New licensee code", new_licensee },
		{ L"SGB flag", with_code(header.sgb_flag, CartridgeSgbFlagName(header.sgb_flag)) },
		{ L"Cartridge type", with_code(header.cartridge_type, CartridgeTypeName(header.cartridge_type)) },
		{ L"ROM size", with_code(header.rom_size_code, CartridgeRomSizeName(header.rom_size_code)) },
		{ L"RAM size", with_code(header.ram_size_code, CartridgeRamSizeName(header.ram_size_code)) },
		{ L"Destination", with_code(header.destination_code, CartridgeDestinationName(header.destination_code)) },
		{ L"Old licensee code", with_code(header.old_licensee_code, CartridgeOldLicenseeName(header.old_licensee_code)) },
		{ L"Version", Hex8(header.version) },
		{ L"Header checksum", header_checksum },
		{ L"Global checksum", global_checksum },
	};

	if (addresses != nullptr)
	{
		*addresses = { L"$0104", L"$0134", L"$013F", L"$0143", L"$0144", L"$0146", L"$0147", L"$0148", L"$0149", L"$014A", L"$014B", L"$014C", L"$014D", L"$014E" };
	}

	return rows;
}

void PropertiesWindow::UpdateContent()
{
	SetWindowTextW(m_Hwnd, (L"Properties - " + m_RomPath.stem().wstring()).c_str());

	// The file name reads better than the header title, which is short and often cryptic.
	// The header title is still on the Header tab.
	SetWindowTextW(m_LabelTitle, m_RomPath.stem().wstring().c_str());
	SetWindowTextW(m_LabelPath, std::filesystem::absolute(m_RomPath).wstring().c_str());

	LayoutGeneral(BuildGeneralRows());

	// Header
	std::vector<std::wstring> addresses;
	const auto rows = BuildHeaderRows(&addresses);

	ListView_DeleteAllItems(m_HeaderList);
	for (int i = 0; i < static_cast<int>(rows.size()); ++i)
	{
		LVITEMW item = { 0 };
		item.mask = LVIF_TEXT;
		item.iItem = i;
		item.pszText = const_cast<wchar_t*>(addresses[i].c_str());
		ListView_InsertItem(m_HeaderList, &item);

		ListView_SetItemText(m_HeaderList, i, 1, const_cast<wchar_t*>(rows[i].first.c_str()));
		ListView_SetItemText(m_HeaderList, i, 2, const_cast<wchar_t*>(rows[i].second.c_str()));
	}

	SelectTab(m_SelectedTab);
	InvalidateRect(m_Hwnd, NULL, TRUE);
}

void PropertiesWindow::LayoutGeneral(const std::vector<std::pair<std::wstring, std::wstring>>& rows)
{
	const int x = m_Page.left + 16;
	const int width = (m_Page.right - 16) - x;
	const int label_width = 110;
	const int row_height = 22;

	MoveWindow(m_LabelTitle, x, m_Page.top + 14, width, 26, FALSE);
	MoveWindow(m_LabelPath, x, m_Page.top + 42, width, 18, FALSE);

	// The first divider sits under the path, the rest come from the rows
	int y = m_Page.top + 70;
	size_t divider = 0;
	auto place_divider = [&]()
	{
		if (divider == m_Dividers.size())
		{
			m_Dividers.push_back(CreateLabel(m_Hwnd, m_Font, 0));
		}

		MoveWindow(m_Dividers[divider++], x, y, width, 1, FALSE);
		y += 12;
	};

	place_divider();

	size_t row = 0;
	m_ValueChecksums = NULL;
	m_ValueSupport = NULL;

	for (const auto& [label, value] : rows)
	{
		if (label == Divider)
		{
			y += 2;
			place_divider();
			continue;
		}

		if (row == m_GeneralLabels.size())
		{
			m_GeneralLabels.push_back(CreateLabel(m_Hwnd, m_Font, SS_LEFT | SS_NOPREFIX));
			m_GeneralValues.push_back(CreateLabel(m_Hwnd, m_Font, SS_LEFT | SS_ENDELLIPSIS | SS_NOPREFIX));
		}

		SetWindowTextW(m_GeneralLabels[row], label.c_str());
		SetWindowTextW(m_GeneralValues[row], value.c_str());
		MoveWindow(m_GeneralLabels[row], x, y, label_width, 18, FALSE);
		MoveWindow(m_GeneralValues[row], x + label_width + 8, y, width - label_width - 8, 18, FALSE);

		if (label == L"Checksums")
		{
			m_ValueChecksums = m_GeneralValues[row];
		}
		else if (label == L"RetroGBm")
		{
			m_ValueSupport = m_GeneralValues[row];
		}

		++row;
		y += row_height;
	}

	m_RowsShown = row;
	m_DividersShown = divider;
}

void PropertiesWindow::SelectTab(int index)
{
	m_SelectedTab = index;
	TabCtrl_SetCurSel(m_Tabs, index);

	const bool general = index == 0;
	ShowWindow(m_LabelTitle, general ? SW_SHOW : SW_HIDE);
	ShowWindow(m_LabelPath, general ? SW_SHOW : SW_HIDE);

	for (size_t i = 0; i < m_Dividers.size(); ++i)
	{
		ShowWindow(m_Dividers[i], (general && i < m_DividersShown) ? SW_SHOW : SW_HIDE);
	}

	for (size_t i = 0; i < m_GeneralLabels.size(); ++i)
	{
		const int show = (general && i < m_RowsShown) ? SW_SHOW : SW_HIDE;
		ShowWindow(m_GeneralLabels[i], show);
		ShowWindow(m_GeneralValues[i], show);
	}

	ShowWindow(m_HeaderList, index == 1 ? SW_SHOW : SW_HIDE);
}

void PropertiesWindow::CopyInfo()
{
	std::wstring text = L"File: " + m_RomPath.filename().wstring() + L"\r\n";

	for (const auto& row : BuildGeneralRows())
	{
		if (row.first != Divider)
		{
			text += row.first + L": " + row.second + L"\r\n";
		}
	}

	text += L"\r\nHeader\r\n";

	std::vector<std::wstring> addresses;
	const auto rows = BuildHeaderRows(&addresses);
	for (size_t i = 0; i < rows.size(); ++i)
	{
		text += addresses[i] + L"  " + rows[i].first + L": " + rows[i].second + L"\r\n";
	}

	if (!OpenClipboard(m_Hwnd))
	{
		return;
	}

	EmptyClipboard();

	const size_t bytes = (text.size() + 1) * sizeof(wchar_t);
	HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
	if (memory != NULL)
	{
		void* destination = GlobalLock(memory);
		if (destination != nullptr)
		{
			memcpy(destination, text.c_str(), bytes);
			GlobalUnlock(memory);

			if (SetClipboardData(CF_UNICODETEXT, memory) == NULL)
			{
				GlobalFree(memory);
			}
		}
		else
		{
			GlobalFree(memory);
		}
	}

	CloseClipboard();
}

void PropertiesWindow::PaintBackground(HDC hdc)
{
	RECT client;
	GetClientRect(m_Hwnd, &client);

	// Grey window
	FillRect(hdc, &client, GetSysColorBrush(COLOR_BTNFACE));

	// White page with a 1px border
	SetDCBrushColor(hdc, PageBorder);
	FillRect(hdc, &m_Page, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));

	RECT inner = m_Page;
	InflateRect(&inner, -1, -1);
	FillRect(hdc, &inner, GetSysColorBrush(COLOR_WINDOW));
}

void PropertiesWindow::WindowCreate(int width, int height)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);

	WNDCLASS wc = {};
	wc.style = CS_VREDRAW | CS_HREDRAW;
	wc.lpfnWndProc = MainWndProc;
	wc.hInstance = hInstance;
	wc.hIcon = NULL;
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.lpszClassName = m_RegisterClassName.c_str();
	wc.lpszMenuName = NULL;
	RegisterClass(&wc);

	// Compute window rectangle dimensions based on requested client area dimensions.
	DWORD exStyle = WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT;
	DWORD dwStyle = (WS_CAPTION | WS_POPUP | WS_SYSMENU | WS_CLIPCHILDREN);

	RECT window_rect = { 0, 0, width, height };
	AdjustWindowRectEx(&window_rect, dwStyle, FALSE, exStyle);
	int window_width = window_rect.right - window_rect.left;
	int window_height = window_rect.bottom - window_rect.top;

	// Centre over the main window, kept inside the monitor's work area
	HWND parent_window = m_Application->GetMainWindow()->GetHwnd();

	RECT parentRect;
	GetWindowRect(parent_window, &parentRect);

	int x = parentRect.left + ((parentRect.right - parentRect.left) - window_width) / 2;
	int y = parentRect.top + ((parentRect.bottom - parentRect.top) - window_height) / 2;

	MONITORINFO monitor = { sizeof(MONITORINFO) };
	if (GetMonitorInfo(MonitorFromWindow(parent_window, MONITOR_DEFAULTTONEAREST), &monitor))
	{
		if (x + window_width > monitor.rcWork.right) x = monitor.rcWork.right - window_width;
		if (y + window_height > monitor.rcWork.bottom) y = monitor.rcWork.bottom - window_height;
		if (x < monitor.rcWork.left) x = monitor.rcWork.left;
		if (y < monitor.rcWork.top) y = monitor.rcWork.top;
	}

	m_Hwnd = CreateWindowEx(exStyle, wc.lpszClassName, L"Properties", dwStyle, x, y, window_width, window_height, parent_window, NULL, hInstance, this);
	if (m_Hwnd == NULL)
	{
		throw std::exception("CreateWindow Failed");
	}

	ShowWindow(m_Hwnd, SW_SHOWNORMAL);
}

LRESULT PropertiesWindow::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_CLOSE:
			DestroyWindow(hwnd);
			return 0;

		case WM_DESTROY:
			m_Hwnd = NULL;
			m_GeneralLabels.clear();
			m_GeneralValues.clear();
			m_Dividers.clear();
			m_RowsShown = 0;
			m_DividersShown = 0;
			return 0;

		case WM_CTLCOLORSTATIC:
		{
			HDC hdc = (HDC)wParam;
			HWND control = reinterpret_cast<HWND>(lParam);
			SetBkMode(hdc, TRANSPARENT);

			// Divider lines are empty labels filled with the line colour
			for (HWND divider : m_Dividers)
			{
				if (divider == control)
				{
					SetDCBrushColor(hdc, DividerColour);
					return (LONG_PTR)GetStockObject(DC_BRUSH);
				}
			}

			COLORREF colour = GetSysColor(COLOR_WINDOWTEXT);
			if (control == m_LabelPath)
			{
				colour = TextGrey;
			}
			else if (control == m_ValueChecksums)
			{
				colour = (m_Header.IsHeaderChecksumValid() && m_Header.IsGlobalChecksumValid()) ? TextGreen : TextRed;
			}
			else if (control == m_ValueSupport)
			{
				colour = TextRed;
			}
			else
			{
				for (HWND label : m_GeneralLabels)
				{
					if (label == control)
					{
						colour = TextGrey;
						break;
					}
				}
			}

			SetTextColor(hdc, colour);
			return (LONG_PTR)GetSysColorBrush(COLOR_WINDOW);
		}

		case WM_ERASEBKGND:
			PaintBackground(reinterpret_cast<HDC>(wParam));
			return 1;

		case WM_PRINTCLIENT:
			// Themed buttons and tabs ask the parent to draw the background behind them
			PaintBackground(reinterpret_cast<HDC>(wParam));
			return 0;

		case WM_COMMAND:
		{
			const int id = LOWORD(wParam);
			if (id == IDCANCEL)
			{
				DestroyWindow(hwnd);
				return 0;
			}
			else if (id == static_cast<int>(m_ControlCopyButtonId))
			{
				CopyInfo();
				return 0;
			}

			break;
		}

		case WM_NOTIFY:
		{
			LPNMHDR nmhdr = reinterpret_cast<LPNMHDR>(lParam);
			if (nmhdr->idFrom == m_ControlTabsId && nmhdr->code == TCN_SELCHANGE)
			{
				SelectTab(TabCtrl_GetCurSel(m_Tabs));
				return 0;
			}

			break;
		}
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}
