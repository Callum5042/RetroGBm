#pragma once

#include "Window.h"
#include <string>

class Application;
struct CheatCode;

class CheatsWindow
{
	Application* m_Application = nullptr;

public:
	CheatsWindow(Application* application);
	virtual ~CheatsWindow();

	void Create();
	void Destroy();

	inline bool IsOpen() const { return m_Hwnd != NULL; }
	void Focus();

	// Handles Tab, Enter and Esc for this window. Returns true if the message was consumed.
	bool HandleDialogMessage(MSG* msg);

	LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:

	// Create window
	void WindowCreate(const std::string& title, int width, int height);
	void PaintBackground(HDC hdc);
	HWND m_Hwnd = NULL;
	std::wstring m_RegisterClassName;

	// Layout
	static const int m_ClientWidth = 640;
	static const int m_ClientHeight = 388;
	static const int m_FooterHeight = 44;

	HFONT m_Font = NULL;
	HFONT m_HeadingFont = NULL;
	HFONT m_CodeFont = NULL;
	HWND m_LastFocus = NULL;

	// Footer
	HWND m_LabelFooter = NULL;
	HWND m_ButtonClose = NULL;

	// List
	HWND m_LabelList = NULL;
	HWND m_LabelSummary = NULL;
	HWND m_ButtonNew = NULL;
	HWND m_ButtonDelete = NULL;

	static const int m_CodesColumnWidth = 52;

	void InsertListItem(int index, const CheatCode& cheat);
	void UpdateListColumns();
	void UpdateEnabledSummary();
	void StartNew();
	void DeleteSelected();

	// Form
	HWND m_LabelHeading = NULL;
	HWND m_LabelName = NULL;
	HWND m_EditName = NULL;

	HWND m_LabelCode = NULL;
	HWND m_EditCode = NULL;

	HWND m_ButtonRevert = NULL;
	HWND m_ButtonSave = NULL;

	// Result of checking the codes as they're typed
	HWND m_LabelStatus = NULL;
	COLORREF m_StatusColor = RGB(92, 92, 92);

	// Selects the first invalid line and points a balloon tip at it
	void ShowCodeError();

	// Loads the selected cheat into the form, or clears it when nothing is selected
	void LoadForm();

	// Updates the heading and buttons for New/Edit mode and whether there are changes
	void UpdateFormState();

	void SaveForm();
	void RevertForm();
	void SetButtonEnabled(HWND button, bool enabled);

	const int m_ControlListViewId = 1000;
	const int m_ControlEditNameId = 1001;
	const int m_ControlEditCodeId = 1002;
	const int m_ControlSaveButtonId = 2001;
	const int m_ControlRevertButtonId = 2002;
	const int m_ControlDeleteButtonId = 2003;
	const int m_ControlNewButtonId = 2004;

	// ListView
	HWND m_ListHwnd = NULL;

	// Cheat codes. -1 means no selection, so the form is adding a new cheat.
	int m_SelectedCheatCodeIndex = -1;

	// The cheat that was selected before starting a new one, reselected by Cancel
	int m_CancelIndex = -1;
};