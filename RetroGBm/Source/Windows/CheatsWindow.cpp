#include "Windows/CheatsWindow.h"
#include "Application.h"

#include <uxtheme.h>
#include <dwmapi.h>
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")

#include <RetroGBm/Logger.h>
#include <RetroGBm/Emulator.h>
#include <RetroGBm/Cheats.h>
#include <filesystem>

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

	std::wstring GetEditText(HWND edit)
	{
		std::wstring text(static_cast<size_t>(GetWindowTextLength(edit)), L'\0');
		GetWindowText(edit, text.data(), static_cast<int>(text.size()) + 1);
		return text;
	}

	void SetTextIfChanged(HWND control, const std::wstring& text)
	{
		// Avoids repainting (and flickering) on every keystroke
		if (GetEditText(control) != text)
		{
			SetWindowText(control, text.c_str());
		}
	}

	bool IsBlank(const std::wstring& text)
	{
		return text.find_first_not_of(L" \t\r\n") == std::wstring::npos;
	}

	std::wstring ToUpper(std::wstring text)
	{
		for (auto& c : text)
		{
			c = towupper(c);
		}

		return text;
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

	// Show the ROM file name without its extension in the title, e.g. "Cheats - Pokemon Red (UE)".
	// The class name stays "Cheats". CurrentFilename is in the ANSI code page, which path converts back.
	{
		const std::wstring rom_name = std::filesystem::path(m_Application->CurrentFilename).stem().wstring();
		if (!rom_name.empty())
		{
			SetWindowText(m_Hwnd, (L"Cheats \x2014 " + rom_name).c_str());
		}
	}

	// Use the system message font (Segoe UI 9pt on Windows 10/11)
	{
		NONCLIENTMETRICS metrics = { sizeof(NONCLIENTMETRICS) };
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0);
		m_Font = CreateFontIndirect(&metrics.lfMessageFont);

		LOGFONT heading = metrics.lfMessageFont;
		heading.lfWeight = FW_SEMIBOLD;
		m_HeadingFont = CreateFontIndirect(&heading);

		// Consolas 10pt for codes
		HDC hdc = GetDC(NULL);
		LOGFONT code = {};
		code.lfHeight = -MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72);
		code.lfWeight = FW_NORMAL;
		code.lfQuality = CLEARTYPE_QUALITY;
		code.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
		wcscpy_s(code.lfFaceName, L"Consolas");
		m_CodeFont = CreateFontIndirect(&code);
		ReleaseDC(NULL, hdc);
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
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_NOCOLUMNHEADER,
		list_x, 30, list_width, buttons_y - 6 - 30,
		m_Hwnd, (HMENU)m_ControlListViewId, GetModuleHandle(NULL), NULL);

	SendMessageW(m_ListHwnd, WM_SETFONT, (WPARAM)m_Font, TRUE);

	// Explorer theme gives the light blue selection
	SetWindowTheme(m_ListHwnd, L"Explorer", NULL);
	ListView_SetExtendedListViewStyle(m_ListHwnd, LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER);

	// One hidden-header column for the name, sized to fit by UpdateListColumns
	LVCOLUMN lvc = { 0 };
	lvc.mask = LVCF_WIDTH;
	lvc.cx = list_width;
	ListView_InsertColumn(m_ListHwnd, 0, &lvc);

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

	const int form_x = list_x + list_width + 16;
	const int form_width = m_ClientWidth - 12 - form_x;

	// Label: "Edit cheat" / "New cheat"
	m_LabelHeading = CreateWindowW(L"STATIC", L"",
		WS_CHILD | WS_VISIBLE,
		form_x, 8, form_width, 16,
		m_Hwnd, nullptr, nullptr, nullptr);

	SendMessageW(m_LabelHeading, WM_SETFONT, (WPARAM)m_HeadingFont, TRUE);

	// Label: "Name:"
	m_LabelName = CreateWindowW(L"STATIC", L"Name:",
		WS_CHILD | WS_VISIBLE,
		form_x, 30, form_width, 16,
		m_Hwnd, nullptr, nullptr, nullptr);

	SendMessageW(m_LabelName, WM_SETFONT, (WPARAM)m_Font, TRUE);

	// Single-line Edit: Cheat name
	m_EditName = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", nullptr,
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
		form_x, 50, form_width, 23,
		m_Hwnd, (HMENU)m_ControlEditNameId, nullptr, nullptr);

	SendMessageW(m_EditName, WM_SETFONT, (WPARAM)m_Font, TRUE);
	Edit_SetCueBannerTextFocused(m_EditName, L"e.g. Infinite health", TRUE);

	// Label: "GameShark codes (one per line):"
	m_LabelCode = CreateWindowW(L"STATIC", L"GameShark codes (one per line):",
		WS_CHILD | WS_VISIBLE,
		form_x, 83, form_width, 16,
		m_Hwnd, nullptr, nullptr, nullptr);

	SendMessageW(m_LabelCode, WM_SETFONT, (WPARAM)m_Font, TRUE);

	// Multi-line Edit: Cheat code. The space above the buttons is left for the status line.
	const int code_y = 103;
	m_EditCode = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", nullptr,
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | ES_UPPERCASE | WS_VSCROLL,
		form_x, code_y, form_width, buttons_y - 8 - 18 - 6 - code_y,
		m_Hwnd, (HMENU)m_ControlEditCodeId, nullptr, nullptr);

	SendMessageW(m_EditCode, WM_SETFONT, (WPARAM)m_CodeFont, TRUE);

	// Label: "3 valid codes" / "Line 2: ..." under the code box
	m_LabelStatus = CreateWindowW(L"STATIC", L"",
		WS_CHILD | WS_VISIBLE | SS_LEFT | SS_ENDELLIPSIS,
		form_x, buttons_y - 8 - 18 + 1, form_width, 18,
		m_Hwnd, nullptr, nullptr, nullptr);

	SendMessageW(m_LabelStatus, WM_SETFONT, (WPARAM)m_Font, TRUE);

	// Button: Revert / Cancel
	m_ButtonRevert = CreateWindowW(L"BUTTON", L"Revert",
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
		m_ClientWidth - 12 - 75 - 6 - 75, buttons_y, 75, 24,
		m_Hwnd, (HMENU)m_ControlRevertButtonId, nullptr, nullptr);

	SendMessageW(m_ButtonRevert, WM_SETFONT, (WPARAM)m_Font, TRUE);

	// Button: Save / Add, the default button for Enter
	m_ButtonSave = CreateWindowW(L"BUTTON", L"Save",
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
		m_ClientWidth - 12 - 75, buttons_y, 75, 24,
		m_Hwnd, (HMENU)m_ControlSaveButtonId, nullptr, nullptr);

	SendMessageW(m_ButtonSave, WM_SETFONT, (WPARAM)m_Font, TRUE);

	// Footer
	const int footer_top = m_ClientHeight - m_FooterHeight;

	m_LabelFooter = CreateWindowW(L"STATIC", L"Ticked cheats apply immediately and are saved with this game's profile.",
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

	// Start on the first cheat, or ready to type a new one when there are none
	if (ListView_GetItemCount(m_ListHwnd) > 0)
	{
		ListView_SetItemState(m_ListHwnd, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		UpdateFormState();
		SetFocus(m_ListHwnd);
	}
	else
	{
		UpdateFormState();
		SetFocus(m_EditName);
	}
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

	for (HFONT* font : { &m_Font, &m_HeadingFont, &m_CodeFont })
	{
		if (*font != NULL)
		{
			DeleteObject(*font);
			*font = NULL;
		}
	}
}

void CheatsWindow::Focus()
{
	SetForegroundWindow(m_Hwnd);
}

void CheatsWindow::InsertListItem(int index, const CheatCode& cheat)
{
	std::wstring name = ConvertToWString(cheat.name);

	LVITEM lvi = { 0 };
	lvi.mask = LVIF_TEXT;
	lvi.iItem = index;
	lvi.iSubItem = 0;
	lvi.pszText = const_cast<wchar_t*>(name.c_str());
	ListView_InsertItem(m_ListHwnd, &lvi);

	ListView_SetCheckState(m_ListHwnd, index, (cheat.enabled ? TRUE : FALSE));
}

void CheatsWindow::UpdateListColumns()
{
	// Fill the list, less the scroll bar when it's shown, so there's never a horizontal scroll bar
	RECT client;
	GetClientRect(m_ListHwnd, &client);
	ListView_SetColumnWidth(m_ListHwnd, 0, client.right - client.left);
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
	// Deselecting switches the form to New mode and remembers the cheat for Cancel
	ListView_SetItemState(m_ListHwnd, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
	m_SelectedCheatCodeIndex = -1;

	LoadForm();
	SetFocus(m_EditName);
}

void CheatsWindow::DeleteSelected()
{
	if (m_SelectedCheatCodeIndex < 0 || !Emulator::Instance->RemoveGamesharkCode(m_SelectedCheatCodeIndex))
	{
		return;
	}

	int index = m_SelectedCheatCodeIndex;
	m_SelectedCheatCodeIndex = -1;

	// Update UI
	ListView_DeleteItem(m_ListHwnd, index);
	UpdateListColumns();
	UpdateEnabledSummary();

	m_CancelIndex = -1;
	LoadForm();
}

void CheatsWindow::LoadForm()
{
	std::wstring name;
	std::wstring code;

	const std::vector<CheatCode> gameshark_codes = Emulator::Instance->GetGamesharkCodes();
	if (m_SelectedCheatCodeIndex >= 0 && m_SelectedCheatCodeIndex < gameshark_codes.size())
	{
		name = ConvertToWString(gameshark_codes[m_SelectedCheatCodeIndex].name);
		code = ConvertCodeToMultiline(gameshark_codes[m_SelectedCheatCodeIndex].code);
	}

	SetWindowText(m_EditName, name.c_str());
	SetWindowText(m_EditCode, code.c_str());
	UpdateFormState();
}

void CheatsWindow::UpdateFormState()
{
	const bool is_new = m_SelectedCheatCodeIndex < 0;
	const std::wstring name = GetEditText(m_EditName);
	const std::wstring code = GetEditText(m_EditCode);

	bool changed = false;
	if (is_new)
	{
		changed = !name.empty() || !code.empty();
	}
	else
	{
		const std::vector<CheatCode> gameshark_codes = Emulator::Instance->GetGamesharkCodes();
		if (m_SelectedCheatCodeIndex < gameshark_codes.size())
		{
			const CheatCode& cheat = gameshark_codes[m_SelectedCheatCodeIndex];
			changed = name != ConvertToWString(cheat.name) || ToUpper(code) != ToUpper(ConvertCodeToMultiline(cheat.code));
		}
	}

	// Check the codes and show the result under the code box
	const GamesharkValidation validation = ValidateGamesharkCodes(ConvertToString(code));

	std::wstring status;
	COLORREF status_color;
	if (validation.valid)
	{
		const size_t count = validation.codes.size();
		status = std::to_wstring(count) + (count == 1 ? L" valid code" : L" valid codes");
		status_color = RGB(15, 123, 15);

		if (IsBlank(name))
		{
			status += L" \x2014 add a name to save";
		}
	}
	else if (validation.error_line > 0)
	{
		status = L"Line " + std::to_wstring(validation.error_line) + L": \x201C" + ConvertToWString(validation.error_text) + L"\x201D is not an 8-digit hex code";
		status_color = RGB(196, 43, 28);
	}
	else
	{
		status = L"Paste or type GameShark codes, e.g. 01FF47D3";
		status_color = RGB(92, 92, 92);
	}

	if (status_color != m_StatusColor)
	{
		m_StatusColor = status_color;
		InvalidateRect(m_LabelStatus, NULL, TRUE);
	}

	SetTextIfChanged(m_LabelStatus, status);

	const bool complete = !IsBlank(name) && validation.valid;
	const bool can_cancel_to = m_CancelIndex >= 0 && m_CancelIndex < ListView_GetItemCount(m_ListHwnd);

	SetTextIfChanged(m_LabelHeading, is_new ? L"New cheat" : L"Edit cheat");
	SetTextIfChanged(m_ButtonRevert, is_new ? L"Cancel" : L"Revert");
	SetTextIfChanged(m_ButtonSave, is_new ? L"Add" : L"Save");

	// Cancel can also go back to the previously selected cheat when nothing has been typed
	SetButtonEnabled(m_ButtonRevert, changed || (is_new && can_cancel_to));
	SetButtonEnabled(m_ButtonSave, changed && complete);
	SetButtonEnabled(m_ButtonDelete, !is_new);
}

void CheatsWindow::SaveForm()
{
	// Also guards the Enter key, which arrives as IDOK whether or not Save is enabled
	if (!IsWindowEnabled(m_ButtonSave))
	{
		ShowCodeError();
		return;
	}

	// Store the cleaned up codes: uppercase, no whitespace and no blank lines
	const std::wstring cheat_name = GetEditText(m_EditName);
	const std::vector<std::string> codes = ValidateGamesharkCodes(ConvertToString(GetEditText(m_EditCode))).codes;

	if (m_SelectedCheatCodeIndex < 0)
	{
		// Add to the emulator, then the list
		CheatCode cheat = { ConvertToString(cheat_name), codes, false };
		Emulator::Instance->AddGamesharkCode(cheat);

		int index = static_cast<int>(Emulator::Instance->GetGamesharkCodes().size()) - 1;
		InsertListItem(index, cheat);
		UpdateListColumns();
		UpdateEnabledSummary();

		// Selecting the new cheat loads it back into the form in Edit mode
		ListView_SetItemState(m_ListHwnd, index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		ListView_EnsureVisible(m_ListHwnd, index, FALSE);
	}
	else
	{
		if (!Emulator::Instance->UpdateGamesharkCode(m_SelectedCheatCodeIndex, ConvertToString(cheat_name), codes))
		{
			return;
		}

		ListView_SetItemText(m_ListHwnd, m_SelectedCheatCodeIndex, 0, const_cast<wchar_t*>(cheat_name.c_str()));

		// Reload so the form matches what was saved, e.g. without blank lines
		LoadForm();
	}
}

void CheatsWindow::RevertForm()
{
	if (m_SelectedCheatCodeIndex < 0 && m_CancelIndex >= 0 && m_CancelIndex < ListView_GetItemCount(m_ListHwnd))
	{
		// Cancel a new cheat by going back to the one selected before
		ListView_SetItemState(m_ListHwnd, m_CancelIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		ListView_EnsureVisible(m_ListHwnd, m_CancelIndex, FALSE);
		SetFocus(m_ListHwnd);
		return;
	}

	LoadForm();
}

void CheatsWindow::ShowCodeError()
{
	const std::wstring code = GetEditText(m_EditCode);
	const GamesharkValidation validation = ValidateGamesharkCodes(ConvertToString(code));
	if (validation.error_line == 0)
	{
		return;
	}

	// Find the bad line by counting newlines, as EM_LINEINDEX counts wrapped lines
	size_t start = 0;
	for (size_t line = 1; line < validation.error_line; ++line)
	{
		start = code.find(L'\n', start) + 1;
	}

	size_t end = code.find(L'\r', start);
	if (end == std::wstring::npos)
	{
		end = code.size();
	}

	SetFocus(m_EditCode);
	SendMessage(m_EditCode, EM_SETSEL, start, end);

	std::wstring title = L"Invalid code on line " + std::to_wstring(validation.error_line);

	EDITBALLOONTIP tip = { sizeof(EDITBALLOONTIP) };
	tip.pszTitle = title.c_str();
	tip.pszText = L"Each line must be 8 hex digits (0-9, A-F), like 01FF47D3.";
	tip.ttiIcon = TTI_ERROR;
	Edit_ShowBalloonTip(m_EditCode, &tip);
}

void CheatsWindow::SetButtonEnabled(HWND button, bool enabled)
{
	// A disabled button can't keep focus, so move it to the name box first
	if (!enabled && GetFocus() == button)
	{
		SetFocus(m_EditName);
	}

	EnableWindow(button, enabled ? TRUE : FALSE);
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

			if (reinterpret_cast<HWND>(lParam) == m_LabelStatus)
			{
				SetTextColor(hdc, m_StatusColor);
				return (LONG_PTR)GetSysColorBrush(COLOR_WINDOW);
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
			else if (wmId == IDOK)
			{
				// Enter key, sent by IsDialogMessage
				SaveForm();
				return 0;
			}
			else if (wmId == m_ControlSaveButtonId)
			{
				SaveForm();
			}
			else if (wmId == m_ControlRevertButtonId)
			{
				RevertForm();
			}
			else if ((wmId == m_ControlEditNameId || wmId == m_ControlEditCodeId) && HIWORD(wParam) == EN_CHANGE)
			{
				UpdateFormState();
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
								m_SelectedCheatCodeIndex = pnm->iItem;
								LoadForm();
							}
							else
							{
								// Deselected, e.g. by clicking empty space in the list. Cancel can come back to it.
								m_SelectedCheatCodeIndex = -1;
								m_CancelIndex = pnm->iItem;
								LoadForm();
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

					case LVN_GETEMPTYMARKUP:
					{
						// Text shown in the list when there are no cheats
						NMLVEMPTYMARKUP* markup = reinterpret_cast<NMLVEMPTYMARKUP*>(lParam);
						markup->dwFlags = EMF_CENTERED;
						wcscpy_s(markup->szMarkup, L"No cheats for this game yet.\nAdd one using the form on the right.");
						return TRUE;
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