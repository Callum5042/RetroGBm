#include "Windows/CheatsWindow.h"
#include "Application.h"

#include <uxtheme.h>
#include <dwmapi.h>
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")

#include <RetroGBm/Logger.h>
#include <RetroGBm/Emulator.h>

#include <stdio.h>
#include <string>

// Set theme
// https://docs.microsoft.com/en-gb/windows/win32/controls/cookbook-overview?redirectedfrom=MSDN
#pragma comment(linker,"\"/manifestdependency:type='win32' name = 'Microsoft.Windows.Common-Controls' version = '6.0.0.0' processorArchitecture = '*' publicKeyToken = '6595b64144ccf1df' language = '*'\"")

namespace
{
	static CheatsWindow* GetWindow(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		CheatsWindow* window = nullptr;
		if (uMsg == WM_NCCREATE)
		{
			CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
			window = reinterpret_cast<CheatsWindow*>(pCreate->lpCreateParams);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
		}
		else
		{
			window = reinterpret_cast<CheatsWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		}

		return window;
	}

	static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		CheatsWindow* window = GetWindow(hwnd, msg, wParam, lParam);
		if (window == nullptr)
		{
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}

		return window->HandleMessage(hwnd, msg, wParam, lParam);
	}

	static std::wstring ConvertToWString(const std::string& str)
	{
		size_t size = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0);

		std::vector<wchar_t> buffer(size);
		int chars_converted = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), buffer.data(), static_cast<int>(buffer.size()));

		return std::wstring(buffer.data(), chars_converted);
	}

	static std::string ConvertToString(const std::wstring& wstr)
	{
		size_t size = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), NULL, 0, NULL, NULL);

		std::vector<char> buffer(size);
		int chars_converted = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), buffer.data(), static_cast<int>(buffer.size()), NULL, NULL);

		return std::string(buffer.data(), chars_converted);
	}

	std::vector<std::string> SplitString(std::string code)
	{
		std::vector<std::string> tokens;
		size_t pos = 0;

		std::string token;
		const std::string delimiter = "\r\n";
		while ((pos = code.find(delimiter)) != std::string::npos)
		{
			token = code.substr(0, pos);
			tokens.push_back(token);
			code.erase(0, pos + delimiter.length());
		}

		tokens.push_back(code);
		return tokens;
	}

	std::wstring ConvertCodeToMultiline(std::vector<std::string> codes)
	{
		std::string result;
		for (auto& code : codes)
		{
			if (!code.empty())
			{
				if (!result.empty())
				{
					// Add newline between codes
					result += "\r\n"; 
				}

				// Append the code
				result += code; 
			}
		}

		return ConvertToWString(result);
	}

	int CountCodes(const std::vector<std::string>& codes)
	{
		int count = 0;
		for (const auto& code : codes)
		{
			if (!code.empty())
			{
				++count;
			}
		}

		return count;
	}
}

CheatsWindow::CheatsWindow(Application* application) : m_Application(application)
{
}

CheatsWindow::~CheatsWindow()
{
	this->Destroy();
}

void CheatsWindow::Create()
{
	HINSTANCE hInstance = GetModuleHandle(NULL);

	this->WindowCreate("Cheats", m_ClientWidth, m_ClientHeight);

	// Use the system message font (Segoe UI 9pt on Windows 10/11)
	{
		NONCLIENTMETRICS metrics = { sizeof(NONCLIENTMETRICS) };
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0);
		m_Font = CreateFontIndirect(&metrics.lfMessageFont);
	}

	// Initialize common controls
	INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_LISTVIEW_CLASSES };
	InitCommonControlsEx(&icex);

	/// The List /////////////////////////////////////////
	//////////////////////////////////////////////////////

	const int list_x = 12;
	const int list_width = 236;
	const int buttons_y = m_ClientHeight - m_FooterHeight - 12 - 24;

	// Label: "Cheats for this game:"
	m_LabelList = CreateWindowW(L"STATIC", L"Cheats for this game:",
		WS_CHILD | WS_VISIBLE,
		list_x, 8, 150, 16,
		m_Hwnd, nullptr, nullptr, nullptr);

	SendMessageW(m_LabelList, WM_SETFONT, (WPARAM)m_Font, TRUE);

	// Label: "2 of 4 on"
	m_LabelSummary = CreateWindowW(L"STATIC", L"",
		WS_CHILD | WS_VISIBLE | SS_RIGHT,
		list_x + list_width - 80, 8, 80, 16,
		m_Hwnd, nullptr, nullptr, nullptr);

	SendMessageW(m_LabelSummary, WM_SETFONT, (WPARAM)m_Font, TRUE);

	// Create ListView control
	m_ListHwnd = CreateWindowEx(
		WS_EX_CLIENTEDGE, WC_LISTVIEW, L"",
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER,
		list_x, 30, list_width, buttons_y - 6 - 30,
		m_Hwnd, (HMENU)m_ControlListViewId, GetModuleHandle(NULL), NULL);

	SendMessageW(m_ListHwnd, WM_SETFONT, (WPARAM)m_Font, TRUE);

	// Explorer theme gives the light blue selection
	SetWindowTheme(m_ListHwnd, L"Explorer", NULL);
	ListView_SetExtendedListViewStyle(m_ListHwnd, LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER);

	// Columns are sized to fit by UpdateListColumns, so stop them being dragged
	HWND header = ListView_GetHeader(m_ListHwnd);
	SetWindowLongPtr(header, GWL_STYLE, GetWindowLongPtr(header, GWL_STYLE) | HDS_NOSIZING);

	LVCOLUMN lvc = { 0 };
	lvc.mask = LVCF_WIDTH | LVCF_TEXT;
	lvc.cx = 160;
	lvc.pszText = const_cast<LPWSTR>(L"Name");
	ListView_InsertColumn(m_ListHwnd, 0, &lvc);

	lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT;
	lvc.fmt = LVCFMT_RIGHT;
	lvc.cx = m_CodesColumnWidth;
	lvc.pszText = const_cast<LPWSTR>(L"Codes");
	ListView_InsertColumn(m_ListHwnd, 1, &lvc);

	const std::vector<CheatCode> gameshark_codes = Emulator::Instance->GetGamesharkCodes();
	for (int i = 0; i < gameshark_codes.size(); ++i)
	{
		InsertListItem(i, gameshark_codes[i]);
	}

	UpdateListColumns();
	UpdateEnabledSummary();

	// Button: New
	m_ButtonNew = CreateWindowW(L"BUTTON", L"New",
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
		list_x, buttons_y, 75, 24,
		m_Hwnd, (HMENU)m_ControlNewButtonId, nullptr, nullptr);

	SendMessageW(m_ButtonNew, WM_SETFONT, (WPARAM)m_Font, TRUE);

	// Button: Delete
	m_ButtonDelete = CreateWindowW(L"BUTTON", L"Delete",
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
		list_x + 75 + 6, buttons_y, 75, 24,
		m_Hwnd, (HMENU)m_ControlDeleteButtonId, nullptr, nullptr);

	SendMessageW(m_ButtonDelete, WM_SETFONT, (WPARAM)m_Font, TRUE);

	/// The Form Controls ////////////////////////////////
	//////////////////////////////////////////////////////

	int width = list_x + list_width + 16;

	// Label: "Cheat name"
	m_LabelName = CreateWindowW(L"STATIC", L"Cheat name:",
		WS_CHILD | WS_VISIBLE,
		width, 20, 270, 20,
		m_Hwnd, nullptr, nullptr, nullptr);

	SendMessageW(m_LabelName, WM_SETFONT, (WPARAM)m_Font, TRUE);

	// Single-line Edit: Cheat name
	m_EditName = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", nullptr,
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
		width, 45, 270, 25,
		m_Hwnd, (HMENU)m_ControlEditNameId, nullptr, nullptr);

	SendMessageW(m_EditName, WM_SETFONT, (WPARAM)m_Font, TRUE);

	// Label: "Cheat code"
	m_LabelCode = CreateWindowW(L"STATIC", L"Cheat code:",
		WS_CHILD | WS_VISIBLE,
		width, 80, 270, 20,
		m_Hwnd, nullptr, nullptr, nullptr);

	SendMessageW(m_LabelCode, WM_SETFONT, (WPARAM)m_Font, TRUE);

	// Multi-line Edit: Cheat code
	m_EditCode = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", nullptr,
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | WS_VSCROLL,
		width, 105, 270, 120,
		m_Hwnd, (HMENU)m_ControlEditCodeId, nullptr, nullptr);

	SendMessageW(m_EditCode, WM_SETFONT, (WPARAM)m_Font, TRUE);

	// Button: Add
	int button_x = width;
	int button_width = 80;

	m_ButtonAdd = CreateWindowW(L"BUTTON", L"Add",
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
		button_x, 235, button_width, 30, // x, y, width, height
		m_Hwnd, (HMENU)m_ControlAddButtonId, nullptr, nullptr);

	SendMessageW(m_ButtonAdd, WM_SETFONT, (WPARAM)m_Font, TRUE);

	// Button: Update
	button_x += button_width;

	m_ButtonUpdate = CreateWindowW(L"BUTTON", L"Update",
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
		button_x, 235, button_width, 30, // x, y, width, height
		m_Hwnd, (HMENU)m_ControlUpdateButtonId, nullptr, nullptr);

	SendMessageW(m_ButtonUpdate, WM_SETFONT, (WPARAM)m_Font, TRUE);

	// Disable Update and Delete buttons initially
	EnableWindow(m_ButtonUpdate, FALSE);
	EnableWindow(m_ButtonDelete, FALSE);

	// Footer
	const int footer_top = m_ClientHeight - m_FooterHeight;

	m_LabelFooter = CreateWindowW(L"STATIC", L"Ticked cheats apply immediately.",
		WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
		12, footer_top + 1, 480, m_FooterHeight - 1,
		m_Hwnd, nullptr, nullptr, nullptr);

	SendMessageW(m_LabelFooter, WM_SETFONT, (WPARAM)m_Font, TRUE);

	// Uses IDCANCEL so the Esc key and the button share the same path
	m_ButtonClose = CreateWindowW(L"BUTTON", L"Close",
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
		m_ClientWidth - 12 - 75, footer_top + (m_FooterHeight - 24) / 2, 75, 24,
		m_Hwnd, (HMENU)IDCANCEL, nullptr, nullptr);

	SendMessageW(m_ButtonClose, WM_SETFONT, (WPARAM)m_Font, TRUE);

	SetFocus(m_ListHwnd);
}

void CheatsWindow::Destroy()
{
	// WM_DESTROY clears m_Hwnd
	if (m_Hwnd != NULL)
	{
		DestroyWindow(m_Hwnd);
	}

	HINSTANCE hInstance = GetModuleHandle(NULL);
	UnregisterClass(m_RegisterClassName.c_str(), hInstance);

	if (m_Font != NULL)
	{
		DeleteObject(m_Font);
		m_Font = NULL;
	}
}

void CheatsWindow::Focus()
{
	SetForegroundWindow(m_Hwnd);
}

void CheatsWindow::InsertListItem(int index, const CheatCode& cheat)
{
	std::wstring name = ConvertToWString(cheat.name);
	std::wstring count = std::to_wstring(CountCodes(cheat.code));

	LVITEM lvi = { 0 };
	lvi.mask = LVIF_TEXT;
	lvi.iItem = index;
	lvi.iSubItem = 0;
	lvi.pszText = const_cast<wchar_t*>(name.c_str());
	ListView_InsertItem(m_ListHwnd, &lvi);

	ListView_SetItemText(m_ListHwnd, index, 1, const_cast<wchar_t*>(count.c_str()));
	ListView_SetCheckState(m_ListHwnd, index, (cheat.enabled ? TRUE : FALSE));
}

void CheatsWindow::UpdateListColumns()
{
	// Name takes whatever the Codes column and the scroll bar (when shown) leave
	RECT client;
	GetClientRect(m_ListHwnd, &client);
	ListView_SetColumnWidth(m_ListHwnd, 0, (client.right - client.left) - m_CodesColumnWidth);
}

void CheatsWindow::UpdateEnabledSummary()
{
	const std::vector<CheatCode> codes = Emulator::Instance->GetGamesharkCodes();

	std::wstring summary = L"none";
	if (!codes.empty())
	{
		size_t enabled = 0;
		for (const auto& code : codes)
		{
			if (code.enabled)
			{
				++enabled;
			}
		}

		summary = std::to_wstring(enabled) + L" of " + std::to_wstring(codes.size()) + L" on";
	}

	SetWindowText(m_LabelSummary, summary.c_str());
}

void CheatsWindow::StartNew()
{
	// Deselecting disables Update and Delete
	ListView_SetItemState(m_ListHwnd, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
	m_SelectedCheatCodeIndex = -1;

	SetWindowText(m_EditName, L"");
	SetWindowText(m_EditCode, L"");
	SetFocus(m_EditName);
}

void CheatsWindow::DeleteSelected()
{
	if (m_SelectedCheatCodeIndex < 0 || !Emulator::Instance->RemoveGamesharkCode(m_SelectedCheatCodeIndex))
	{
		return;
	}

	// Update UI
	ListView_DeleteItem(m_ListHwnd, m_SelectedCheatCodeIndex);
	UpdateListColumns();
	UpdateEnabledSummary();

	// Clear form control
	m_SelectedCheatCodeIndex = -1;
	SetWindowText(m_EditName, L"");
	SetWindowText(m_EditCode, L"");

	// Enable/disable buttons
	EnableWindow(m_ButtonAdd, TRUE);
	EnableWindow(m_ButtonUpdate, FALSE);
	EnableWindow(m_ButtonDelete, FALSE);
}

bool CheatsWindow::HandleDialogMessage(MSG* msg)
{
	return m_Hwnd != NULL && IsDialogMessage(m_Hwnd, msg);
}

void CheatsWindow::PaintBackground(HDC hdc)
{
	RECT client;
	GetClientRect(m_Hwnd, &client);

	// White content area
	RECT content = client;
	content.bottom -= m_FooterHeight;
	FillRect(hdc, &content, GetSysColorBrush(COLOR_WINDOW));

	// 1px divider above the footer
	RECT divider = client;
	divider.top = content.bottom;
	divider.bottom = divider.top + 1;
	SetDCBrushColor(hdc, RGB(223, 223, 223));
	FillRect(hdc, &divider, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));

	// Grey footer
	RECT footer = client;
	footer.top = divider.bottom;
	FillRect(hdc, &footer, GetSysColorBrush(COLOR_BTNFACE));
}

void CheatsWindow::WindowCreate(const std::string& title, int width, int height)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);

	// Convert to wstring
	const std::wstring window_title = ConvertToWString(title);
	m_RegisterClassName = window_title;

	// Setup window class
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
	DWORD dwStyle = (WS_CAPTION | WS_POPUP | WS_SYSMENU);

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

	// Create window

	m_Hwnd = CreateWindowEx(exStyle, wc.lpszClassName, window_title.c_str(), dwStyle, x, y, window_width, window_height, parent_window, NULL, hInstance, this);
	if (m_Hwnd == NULL)
	{
		throw std::exception("CreateWindow Failed");
	}

	// Show window
	ShowWindow(m_Hwnd, SW_SHOWNORMAL);
}

LRESULT CheatsWindow::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_CLOSE:
			DestroyWindow(hwnd);
			return 0;

		case WM_DESTROY:
			m_Hwnd = NULL;
			m_LastFocus = NULL;
			return 0;

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
				SetFocus(m_LastFocus != NULL ? m_LastFocus : m_ListHwnd);
			}

			return 0;
		}

		case WM_CTLCOLORSTATIC:
		{
			HDC hdc = (HDC)wParam;
			SetBkMode(hdc, TRANSPARENT);

			if (reinterpret_cast<HWND>(lParam) == m_LabelFooter)
			{
				SetTextColor(hdc, RGB(76, 76, 76));
				return (LONG_PTR)GetSysColorBrush(COLOR_BTNFACE);
			}

			if (reinterpret_cast<HWND>(lParam) == m_LabelSummary)
			{
				SetTextColor(hdc, RGB(92, 92, 92));
				return (LONG_PTR)GetSysColorBrush(COLOR_WINDOW);
			}

			SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
			return (LONG_PTR)GetSysColorBrush(COLOR_WINDOW);
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
			int wmId = LOWORD(wParam);

			if (wmId == IDCANCEL)
			{
				DestroyWindow(hwnd);
				return 0;
			}
			else if (wmId == m_ControlAddButtonId)
			{
				std::wstring cheat_name;
				std::wstring cheat_code;

				HWND control = NULL;
				int length = 0;

				// Get cheat name from the edit control
				control = GetDlgItem(hwnd, m_ControlEditNameId);
				length = GetWindowTextLength(control) + 1;

				cheat_name.resize(static_cast<size_t>(length) - 1);
				GetWindowText(control, const_cast<LPWSTR>(cheat_name.data()), length);

				// Get cheat code from the edit control
				control = GetDlgItem(hwnd, m_ControlEditCodeId);
				length = GetWindowTextLength(control) + 1;

				cheat_code.resize(static_cast<size_t>(length) - 1);
				GetWindowText(control, const_cast<LPWSTR>(cheat_code.data()), length);

				// Check if its valid
				if (cheat_name.empty() || cheat_code.empty())
				{
					MessageBox(hwnd, L"Please enter a valid cheat name and code.", L"Error", MB_OK | MB_ICONERROR);
					return 0;
				}

				// Add to the emulator, then the list
				CheatCode cheat = { ConvertToString(cheat_name), SplitString(ConvertToString(cheat_code)), false };
				Emulator::Instance->AddGamesharkCode(cheat);

				int index = static_cast<int>(Emulator::Instance->GetGamesharkCodes().size()) - 1;
				InsertListItem(index, cheat);
				UpdateListColumns();
				UpdateEnabledSummary();

				// Select the new cheat, which loads it into the form and enables Update/Delete
				ListView_SetItemState(m_ListHwnd, index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
				ListView_EnsureVisible(m_ListHwnd, index, FALSE);
			}
			else if (wmId == m_ControlUpdateButtonId)
			{
				std::wstring cheat_name;
				std::wstring cheat_code;

				HWND control = NULL;
				int length = 0;

				// Get cheat name from the edit control
				control = GetDlgItem(hwnd, m_ControlEditNameId);
				length = GetWindowTextLength(control) + 1;

				cheat_name.resize(static_cast<size_t>(length) - 1);
				GetWindowText(control, const_cast<LPWSTR>(cheat_name.data()), length);

				// Get cheat code from the edit control
				control = GetDlgItem(hwnd, m_ControlEditCodeId);
				length = GetWindowTextLength(control) + 1;

				cheat_code.resize(static_cast<size_t>(length) - 1);
				GetWindowText(control, const_cast<LPWSTR>(cheat_code.data()), length);

				// Check if its valid
				if (cheat_name.empty() || cheat_code.empty())
				{
					MessageBox(hwnd, L"Please enter a valid cheat name and code.", L"Error", MB_OK | MB_ICONERROR);
					return 0;
				}

				// Update the selected cheat code
				std::vector<std::string> codes = SplitString(ConvertToString(cheat_code));
				if (!Emulator::Instance->UpdateGamesharkCode(m_SelectedCheatCodeIndex, ConvertToString(cheat_name), codes))
				{
					return 0;
				}

				std::wstring count = std::to_wstring(CountCodes(codes));
				ListView_SetItemText(m_ListHwnd, m_SelectedCheatCodeIndex, 0, const_cast<wchar_t*>(cheat_name.c_str()));
				ListView_SetItemText(m_ListHwnd, m_SelectedCheatCodeIndex, 1, const_cast<wchar_t*>(count.c_str()));
			}
			else if (wmId == m_ControlNewButtonId)
			{
				StartNew();
			}
			else if (wmId == m_ControlDeleteButtonId)
			{
				DeleteSelected();
			}

			break;
		}
		case WM_NOTIFY:
		{
			LPNMHDR nmhdr = (LPNMHDR)lParam;

			// Ensure it's from your ListView (e.g., ID = 1)
			if (nmhdr->idFrom == m_ControlListViewId)
			{
				switch (nmhdr->code)
				{
					// Selection or check state changed
					case LVN_ITEMCHANGED: 
					{
						NMLISTVIEW* pnm = (NMLISTVIEW*)lParam;

						if ((pnm->uChanged & LVIF_STATE) && ((pnm->uNewState & LVIS_SELECTED) != (pnm->uOldState & LVIS_SELECTED)))
						{
							if (pnm->uNewState & LVIS_SELECTED)
							{
								// Item was selected
								m_SelectedCheatCodeIndex = pnm->iItem;

								// Display the selected cheat code in the edit controls
								const std::vector<CheatCode> gameshark_codes = Emulator::Instance->GetGamesharkCodes();
								if (m_SelectedCheatCodeIndex < 0 || m_SelectedCheatCodeIndex >= gameshark_codes.size())
								{
									break;
								}

								std::wstring name = ConvertToWString(gameshark_codes[m_SelectedCheatCodeIndex].name);
								std::wstring code = ConvertCodeToMultiline(gameshark_codes[m_SelectedCheatCodeIndex].code);

								SetWindowText(m_EditName, name.c_str());
								SetWindowText(m_EditCode, code.c_str());

								// Enable/disable buttons
								EnableWindow(m_ButtonAdd, TRUE);
								EnableWindow(m_ButtonUpdate, TRUE);
								EnableWindow(m_ButtonDelete, TRUE);
							}
							else
							{
								// Item was deselected, e.g. by clicking empty space in the list
								m_SelectedCheatCodeIndex = -1;
								EnableWindow(m_ButtonUpdate, FALSE);
								EnableWindow(m_ButtonDelete, FALSE);
							}
						}

						// Only act when the checkbox itself changes. Selection changes also arrive here, and
						// an old image of 0 is the checkbox being created when an item is inserted.
						UINT old_image = pnm->uOldState & LVIS_STATEIMAGEMASK;
						UINT new_image = pnm->uNewState & LVIS_STATEIMAGEMASK;

						if ((pnm->uChanged & LVIF_STATE) && old_image != 0 && new_image != old_image)
						{
							BOOL isChecked = new_image == INDEXTOSTATEIMAGEMASK(2);

							int iItem = pnm->iItem;

							if (iItem >= 0 && Emulator::Instance->SetGamesharkCodeEnabled(iItem, isChecked))
							{
								std::string cheat_name = Emulator::Instance->GetGamesharkCodes()[iItem].name;
								Logger::Info("Toggled cheat: " + cheat_name + " to " + std::to_string(isChecked));

								UpdateEnabledSummary();
							}
						}

						break;
					}

					case LVN_KEYDOWN:
					{
						NMLVKEYDOWN* key = reinterpret_cast<NMLVKEYDOWN*>(lParam);
						if (key->wVKey == VK_DELETE)
						{
							DeleteSelected();
						}

						break;
					}
				}
			}

			break;
		}
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}