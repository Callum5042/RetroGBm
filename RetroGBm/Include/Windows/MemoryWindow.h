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
	static const int m_BlockListWidth = 180;
	static const int m_InspectorWidth = 300;

	HFONT m_Font = NULL;
	HBRUSH m_InspectorBrush = NULL;

	HWND m_BlockList = NULL;
	MemoryHexView m_HexView;

	// Status bar: what is shown on the left, live or paused on the right
	HWND m_StatusBar = NULL;

	static const int m_ControlStatusBarId = 1000;
	static const int m_ControlBlockListId = 1001;
	static const int m_ControlHexViewId = 1002;
};
