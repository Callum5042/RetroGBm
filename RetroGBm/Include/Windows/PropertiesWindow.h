#pragma once

#include "Window.h"

#include <RetroGBm/Cartridge/CartridgeHeader.h>

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

class Application;

// Shows the header details of a ROM file. Works from the file alone, so it can describe
// a game from the ROM list without starting it, and cartridge types the emulator can't run.
class PropertiesWindow
{
	Application* m_Application = nullptr;

public:
	PropertiesWindow(Application* application);
	virtual ~PropertiesWindow();

	// Opens the window for this ROM, or switches the open window to it
	void Show(const std::filesystem::path& rom_path);
	void Destroy();

	inline bool IsOpen() const { return m_Hwnd != NULL; }

	// Handles Tab and Esc for this window. Returns true if the message was consumed.
	bool HandleDialogMessage(MSG* msg);

	LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	void Create();
	void WindowCreate(int width, int height);
	void PaintBackground(HDC hdc);
	HWND m_Hwnd = NULL;
	std::wstring m_RegisterClassName = L"RetroGBm Properties";

	// Layout: a grey window with a white, bordered page under the tab strip
	static const int m_ClientWidth = 460;
	static const int m_ClientHeight = 500;
	static const int m_Margin = 12;
	static const int m_ButtonWidth = 80;
	static const int m_ButtonHeight = 26;
	RECT m_Page = {};

	HFONT m_Font = NULL;
	HFONT m_TitleFont = NULL;

	// Content
	std::filesystem::path m_RomPath;
	CartridgeHeader m_Header;
	void UpdateContent();
	void LayoutGeneral(const std::vector<std::pair<std::wstring, std::wstring>>& rows);
	void SelectTab(int index);
	void CopyInfo();

	// Label/value pairs shown on the General tab and copied by Copy info.
	// A row labelled "-" is a divider line.
	std::vector<std::pair<std::wstring, std::wstring>> BuildGeneralRows() const;
	std::vector<std::pair<std::wstring, std::wstring>> BuildHeaderRows(std::vector<std::wstring>* addresses) const;

	// Tabs
	HWND m_Tabs = NULL;
	static const UINT m_ControlTabsId = 101;
	int m_SelectedTab = 0;

	// General tab
	HWND m_LabelTitle = NULL;
	HWND m_LabelPath = NULL;
	std::vector<HWND> m_GeneralLabels;
	std::vector<HWND> m_GeneralValues;
	std::vector<HWND> m_Dividers;
	size_t m_RowsShown = 0;
	size_t m_DividersShown = 0;
	HWND m_ValueChecksums = NULL;
	HWND m_ValueSupport = NULL;

	// Header tab
	HWND m_HeaderList = NULL;
	static const UINT m_ControlHeaderListId = 102;

	// Buttons
	HWND m_ButtonCopy = NULL;
	HWND m_ButtonClose = NULL;
	static const UINT m_ControlCopyButtonId = 103;
};
