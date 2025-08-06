#include "Windows/CheatsWindow.h"
#include "Application.h"

#include <uxtheme.h>
#include <dwmapi.h>
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")

#include <RetroGBm/Logger.h>

#include <stdio.h>

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
}

CheatsWindow::CheatsWindow(Application* application) : m_Application(application)
{
	m_CheatCodes.push_back({ L"Wild Celebi", L"01FB04D2" });
	m_CheatCodes.push_back({ L"Shiny Pokemon", L"010730D2" });
}

CheatsWindow::~CheatsWindow()
{
	this->Destroy();
}

void CheatsWindow::Create()
{
	HINSTANCE hInstance = GetModuleHandle(NULL);

	this->WindowCreate("Cheats", 500, 300);

	static HFONT m_Font;

	// Create font
	{
		HDC hdc = GetDC(NULL);
		long lfHeight = -MulDiv(12, GetDeviceCaps(hdc, LOGPIXELSY), 72);
		ReleaseDC(NULL, hdc);

		m_Font = CreateFont(lfHeight, 0, 0, 0, 0, FALSE, 0, 0, 0, 0, 0, 0, 0, L"Arial");

		/*m_Font = CreateFont(
			-MulDiv(9, GetDeviceCaps(hdc, LOGPIXELSY), 72),
			0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Segoe UI"));*/
	}

	int width = 200 + 20;
	int height = 200;

	/*m_ListHwnd = CreateWindow(WC_LISTVIEW, L"",
		WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_NOSORTHEADER,
		0, 0, width, height, m_Hwnd, reinterpret_cast<HMENU>((UINT_PTR)m_ListMenuId), hInstance, this);*/

	// Initialize common controls
	INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_LISTVIEW_CLASSES };
	InitCommonControlsEx(&icex);

	// Create ListView control
	m_ListHwnd = CreateWindowEx(
		WS_EX_CLIENTEDGE, WC_LISTVIEW, L"",
		WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
		10, 10, 200, 300 - 20,
		m_Hwnd, (HMENU)m_ControlListViewId, GetModuleHandle(NULL), NULL);

	// Enable checkboxes
	ListView_SetExtendedListViewStyle(m_ListHwnd, LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);

	// Add a dummy column (required for LVS_REPORT layout)
	LVCOLUMN lvc = { 0 };
	lvc.mask = LVCF_WIDTH | LVCF_TEXT;
	lvc.cx = 160; // width of column
	lvc.pszText = (LPWSTR)L"Cheats"; // column header text
	ListView_InsertColumn(m_ListHwnd, 0, &lvc);

	// Add some items
	for (int i = 0; i < m_CheatCodes.size(); ++i)
	{
		LVITEM lvi = { 0 };
		lvi.mask = LVIF_TEXT;
		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.pszText = const_cast<wchar_t*>(m_CheatCodes[i].name.c_str());
		ListView_InsertItem(m_ListHwnd, &lvi);

		// ListView_SetItemText(m_ListHwnd, i, 1, const_cast<wchar_t*>(m_CheatCodes[i].code.c_str()));
	}


	/// The Form Controls ////////////////////////////////
	//////////////////////////////////////////////////////
	//////////////////////////////////////////////////////
	//////////////////////////////////////////////////////

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
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
		width, 105, 270, 120,
		m_Hwnd, (HMENU)m_ControlEditCodeId, nullptr, nullptr);

	SendMessageW(m_EditCode, WM_SETFONT, (WPARAM)m_Font, TRUE);

	// Button: Add
	int button_x = 220;
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

	// Button: Delete
	button_x += button_width;

	m_ButtonDelete = CreateWindowW(L"BUTTON", L"Delete",
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
		button_x, 235, button_width, 30,
		m_Hwnd, (HMENU)m_ControlDeleteButtonId, nullptr, nullptr);

	SendMessageW(m_ButtonDelete, WM_SETFONT, (WPARAM)m_Font, TRUE);

	// Disable Update and Delete buttons initially
	EnableWindow(m_ButtonUpdate, FALSE);
	EnableWindow(m_ButtonDelete, FALSE);
}

void CheatsWindow::Destroy()
{
	HINSTANCE hInstance = GetModuleHandle(NULL);
	UnregisterClass(m_RegisterClassName.c_str(), hInstance);

	DestroyWindow(m_Hwnd);
}

void CheatsWindow::WindowCreate(const std::string& title, int width, int height)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);


	m_BrushBackground = CreateSolidBrush(RGB(255, 255, 255));

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
	RECT window_rect = { 0, 0, width, height };
	AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW, false);
	int window_width = window_rect.right - window_rect.left;
	int window_height = window_rect.bottom - window_rect.top;

	// Create window
	DWORD exStyle = WS_EX_DLGMODALFRAME;
	DWORD dwStyle = (WS_OVERLAPPED | WS_CAPTION | WS_POPUP | WS_SYSMENU);
	HWND parent_window = m_Application->GetMainWindow()->GetHwnd();

	// Get the dimensions of the parent window
	RECT parentRect;
	GetClientRect(parent_window, &parentRect);

	// Calculate the position for the child window
	int x = (parentRect.right - window_width) / 2;
	int y = (parentRect.bottom - window_height) / 2;

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
		case WM_CREATE:
		{
			HWND hHeader = (HWND)SendMessage(m_ListHwnd, LVM_GETHEADER, 0, 0);
			SetWindowPos(hHeader, 0, 0, 0, 0, 0, SWP_HIDEWINDOW);

			break;
		}

		case WM_CLOSE:
			Destroy();
			return 0;

		case WM_CTLCOLORSTATIC:
		{
			HDC hdc = (HDC)wParam;
			SetTextColor(hdc, RGB(0, 0, 0));
			SetBkMode(hdc, TRANSPARENT);
			return (LONG_PTR)GetSysColorBrush(COLOR_WINDOW);
		}

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);

			// Fill screen with colour to avoid horrible effect
			FillRect(hdc, &ps.rcPaint, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));

			EndPaint(hwnd, &ps);

			break;
		}
		case WM_COMMAND:
		{
			int wmId = LOWORD(wParam);

			if (wmId == m_ControlAddButtonId)
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

				// Add to the list
				LVITEM lvi = { 0 };
				lvi.mask = LVIF_TEXT;
				lvi.iItem = m_CheatCodes.size();
				lvi.iSubItem = 0;
				lvi.pszText = const_cast<wchar_t*>(cheat_name.c_str());
				ListView_InsertItem(m_ListHwnd, &lvi);

				m_CheatCodes.push_back({ cheat_name, cheat_code });

				// Enable/disable buttons
				EnableWindow(m_ButtonAdd, TRUE);
				EnableWindow(m_ButtonUpdate, TRUE);
				EnableWindow(m_ButtonDelete, TRUE);
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
				m_CheatCodes[m_SelectedCheatCodeIndex].name = cheat_name;
				m_CheatCodes[m_SelectedCheatCodeIndex].code = cheat_code;

				ListView_SetItemText(m_ListHwnd, m_SelectedCheatCodeIndex, 0, const_cast<wchar_t*>(cheat_name.c_str()));

			}
			else if (wmId == m_ControlDeleteButtonId)
			{
				// Remove item
				m_CheatCodes.erase(m_CheatCodes.begin() + m_SelectedCheatCodeIndex);

				// Update UI
				ListView_DeleteItem(m_ListHwnd, m_SelectedCheatCodeIndex);

				// Clear form control
				m_SelectedCheatCodeIndex = -1;
				SetWindowText(m_EditName, L"");
				SetWindowText(m_EditCode, L"");

				// Enable/disable buttons
				EnableWindow(m_ButtonAdd, TRUE);
				EnableWindow(m_ButtonUpdate, FALSE);
				EnableWindow(m_ButtonDelete, FALSE);
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
								std::wstring name = m_CheatCodes[m_SelectedCheatCodeIndex].name;
								std::wstring code = m_CheatCodes[m_SelectedCheatCodeIndex].code;

								SetWindowText(m_EditName, name.c_str());
								SetWindowText(m_EditCode, code.c_str());

								// Enable/disable buttons
								EnableWindow(m_ButtonAdd, TRUE);
								EnableWindow(m_ButtonUpdate, TRUE);
								EnableWindow(m_ButtonDelete, TRUE);
							}
						}

						if (pnm->uChanged & LVIF_STATE)
						{
							BOOL wasChecked = (pnm->uOldState & INDEXTOSTATEIMAGEMASK(2)) != 0;
							BOOL isChecked = (pnm->uNewState & INDEXTOSTATEIMAGEMASK(2)) != 0;

							int iItem = pnm->iItem;

							if (!wasChecked && isChecked)
							{
								// Logger::Info("Checkbox enabled");
							}
							else
							{
								// Logger::Info("Checkbox disabled");
							}
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