#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "Windows/MemoryHexView.h"

#include <cstdint>
#include <span>
#include <string>

class Application;

// Tools > Memory: a live, read-only hex view of the Game Boy's memory, one block at a time,
// read straight from the memory behind it rather than through the CPU bus
class MemoryWindow
{
public:
	MemoryWindow(Application* application);
	virtual ~MemoryWindow();

	void Create();
	void Destroy();

	inline bool IsOpen() const { return m_Hwnd != NULL; }
	void Focus();

	// Called every frame while the emulator is running
	void Update();

	// Handles Tab and Esc for this window. Returns true if the message was consumed.
	bool HandleDialogMessage(MSG* msg);

	LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	Application* m_Application = nullptr;

	HWND m_Hwnd = NULL;

	void WindowCreate(int width, int height);
	void CreateControls();
	void Layout();
	void PaintBackground(HDC hdc);

	// Fonts and brushes, released once the window and its controls are gone
	void ReleaseResources();

	// Title shows the loaded ROM, which can change while the window is open
	std::string m_TitleFilename;
	void UpdateTitle();

	void UpdateStatus();

	// Memory blocks, in the order the block list shows them
	enum class Block
	{
		Rom,
		VideoRam,
		CartridgeRam,
		WorkRam,
		Oam,
		HighRam,
		Count
	};

	Block m_Block = Block::Rom;
	void SelectBlock(Block block);
	std::span<const uint8_t> ReadBlock(Block block) const;
	void UpdateEmptyText();

	// Layout
	static const int m_ClientWidth = 1280;
	static const int m_ClientHeight = 780;
	static const int m_CommandBarHeight = 48;
	static const int m_BlockListWidth = 236;
	static const int m_InspectorWidth = 300;

	HFONT m_Font = NULL;
	HBRUSH m_InspectorBrush = NULL;

	// Command bar, laid out as in the design. Each control stays disabled until its feature is built.
	HWND m_LabelGoTo = NULL;
	HWND m_EditGoTo = NULL;
	HWND m_LabelFind = NULL;
	HWND m_EditFind = NULL;
	HWND m_ButtonHex = NULL;
	HWND m_ButtonText = NULL;
	HWND m_LabelColumns = NULL;
	HWND m_ComboColumns = NULL;
	HWND m_CheckHighlight = NULL;
	HWND m_ButtonPause = NULL;
	HWND m_ButtonExport = NULL;

	void CreateCommandBar();
	void LayoutCommandBar(int width);

	// Block list: owner drawn, each row a colour strip, the block's name and its address range
	HWND m_LabelBlocks = NULL;
	HWND m_BlockList = NULL;
	void DrawBlockItem(const DRAWITEMSTRUCT* item);

	MemoryHexView m_HexView;

	// Inspector: what is selected, and how the byte at the start of the selection reads
	enum class Field
	{
		Start,
		End,
		Length,
		Hex8,
		Unsigned8,
		Signed8,
		Binary8,
		Ascii8,
		Count
	};

	static const int m_InspectorSectionCount = 2;
	static const int m_FieldCount = static_cast<int>(Field::Count);

	HFONT m_BoldFont = NULL;
	HFONT m_MonoFont = NULL;

	HWND m_InspectorHeadings[m_InspectorSectionCount] = {};
	HWND m_InspectorLabels[m_FieldCount] = {};
	HWND m_InspectorValues[m_FieldCount] = {};
	std::wstring m_InspectorText[m_FieldCount];

	void CreateInspector();
	void LayoutInspector(int left, int top);
	void UpdateInspector();
	void SetInspectorValue(Field field, const std::wstring& text);

	// Status bar: what is shown on the left, live or paused on the right
	HWND m_StatusBar = NULL;

	static const int m_ControlStatusBarId = 1000;
	static const int m_ControlBlockListId = 1001;
	static const int m_ControlHexViewId = 1002;
	static const int m_ControlGoToId = 1003;
	static const int m_ControlFindId = 1004;
	static const int m_ControlHexId = 1005;
	static const int m_ControlTextId = 1006;
	static const int m_ControlColumnsId = 1007;
	static const int m_ControlHighlightId = 1008;
	static const int m_ControlPauseId = 1009;
	static const int m_ControlExportId = 1010;
	static const int m_ControlInspectorHeadingId = 1100;
	static const int m_ControlInspectorLabelId = 1200;
	static const int m_ControlInspectorValueId = 1300;
};
