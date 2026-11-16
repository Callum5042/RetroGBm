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
#include <cwctype>
#include <filesystem>
#include <format>
#include <optional>
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

	int FontHeight(HFONT font)
	{
		HDC hdc = GetDC(NULL);
		HGDIOBJ previous = SelectObject(hdc, font);

		TEXTMETRIC metrics = {};
		GetTextMetrics(hdc, &metrics);

		SelectObject(hdc, previous);
		ReleaseDC(NULL, hdc);
		return metrics.tmHeight;
	}

	const COLORREF FieldBorderColour = RGB(209, 209, 209);
	const COLORREF FieldFocusColour = RGB(0, 95, 184);
	const COLORREF CommandLabelColour = RGB(74, 74, 74);

	std::wstring GetText(HWND control)
	{
		std::wstring text(static_cast<size_t>(GetWindowTextLength(control)), L'\0');
		GetWindowText(control, text.data(), static_cast<int>(text.size()) + 1);
		return text;
	}

	int HexDigit(wchar_t c)
	{
		if (c >= L'0' && c <= L'9') return c - L'0';
		if (c >= L'a' && c <= L'f') return c - L'a' + 10;
		if (c >= L'A' && c <= L'F') return c - L'A' + 10;
		return -1;
	}

	// Up to max_digits hex digits after an optional "$" or "0x"
	bool ParseHex(std::wstring text, size_t max_digits, size_t* value)
	{
		if (text.starts_with(L"$"))
		{
			text.erase(0, 1);
		}
		else if (text.starts_with(L"0x") || text.starts_with(L"0X"))
		{
			text.erase(0, 2);
		}

		if (text.empty() || text.size() > max_digits)
		{
			return false;
		}

		size_t result = 0;
		for (wchar_t c : text)
		{
			const int digit = HexDigit(c);
			if (digit < 0)
			{
				return false;
			}

			result = result * 16 + digit;
		}

		*value = result;
		return true;
	}

	std::wstring WithoutSpaces(const std::wstring& text)
	{
		std::wstring result;
		for (wchar_t c : text)
		{
			if (!iswspace(c))
			{
				result += c;
			}
		}

		return result;
	}

	const COLORREF InspectorLabelColour = RGB(92, 92, 92);
	const COLORREF InspectorTextColour = RGB(26, 26, 26);
	const COLORREF InspectorKeyColour = RGB(74, 74, 74);
	const COLORREF InspectorRowLineColour = RGB(236, 236, 236);

	// Same order as MemoryWindow::Field
	const wchar_t* FieldLabels[] = { L"u8", L"s8", L"Binary", L"ASCII" };

	// The legend: what the hex view's colours mean, matching MemoryHexView's
	const wchar_t* LegendLabels[] = { L"Changed in the last half second", L"Selection", L"Zero bytes are dimmed" };
	const COLORREF LegendChangedColour = RGB(255, 217, 184);
	const COLORREF LegendSelectionColour = RGB(204, 228, 247);
	const COLORREF LegendSelectionOutline = RGB(109, 166, 224);
	const COLORREF LegendZeroColour = RGB(176, 176, 176);
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

		// The inspector's heading: the selected range, 20px semibold
		mono.lfHeight = -MulDiv(15, GetDeviceCaps(hdc, LOGPIXELSY), 72);
		mono.lfWeight = FW_SEMIBOLD;
		m_TitleFont = CreateFontIndirect(&mono);
		ReleaseDC(NULL, hdc);
	}

	m_InspectorBrush = CreateSolidBrush(InspectorColour);

	INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_STANDARD_CLASSES | ICC_BAR_CLASSES };
	InitCommonControlsEx(&icex);

	this->WindowCreate(m_ClientWidth, m_ClientHeight);

	// A new window starts live, with the plain title
	m_Paused = false;
	m_PausedData.clear();
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
	for (HFONT* font : { &m_Font, &m_BoldFont, &m_MonoFont, &m_TitleFont })
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

	m_InspectorTitleText.clear();
	m_InspectorSubtitleText.clear();

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

	// Paused: the new block is frozen as it is now
	if (m_Paused)
	{
		TakeSnapshot();
	}

	m_HexView.SetBlock(Blocks[static_cast<int>(block)].addressing, [this, block]() { return ViewBlock(block); });
	UpdateEmptyText();
	UpdateInspector();
}

void MemoryWindow::SetPaused(bool paused)
{
	m_Paused = paused;
	if (paused)
	{
		TakeSnapshot();
	}
	else
	{
		// A whole ROM can be several MiB
		m_PausedData.clear();
		m_PausedData.shrink_to_fit();
	}

	m_HexView.Refresh();
	UpdateInspector();
	UpdateStatus();
}

void MemoryWindow::TakeSnapshot()
{
	const std::span<const uint8_t> live = ReadBlock(m_Block);
	m_PausedData.assign(live.begin(), live.end());
}

std::span<const uint8_t> MemoryWindow::ViewBlock(Block block) const
{
	if (!m_Paused || block != m_Block)
	{
		return ReadBlock(block);
	}

	// Nothing to show once the game has gone
	if (ReadBlock(block).empty())
	{
		return {};
	}

	return m_PausedData;
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
	// Only set when changed, as this is called every frame
	auto set_text = [](HWND control, std::wstring* current, const std::wstring& text)
	{
		if (*current != text)
		{
			*current = text;
			SetWindowTextW(control, text.c_str());
		}
	};

	const std::span<const uint8_t> data = ViewBlock(m_Block);
	if (data.empty() || !m_HexView.HasSelection())
	{
		set_text(m_InspectorTitle, &m_InspectorTitleText, L"");
		set_text(m_InspectorSubtitle, &m_InspectorSubtitleText, L"");
		for (int i = 0; i < m_FieldCount; ++i)
		{
			SetInspectorValue(static_cast<Field>(i), L"");
		}

		return;
	}

	// Heading: the range, then its length and the block, e.g. "16 bytes selected · Work RAM"
	const size_t start = (std::min)(m_HexView.GetSelectionStart(), data.size() - 1);
	const size_t end = (std::min)(m_HexView.GetSelectionEnd(), data.size() - 1);
	const size_t length = end - start + 1;

	const std::wstring range = start == end ? m_HexView.FormatAddress(start) : m_HexView.FormatAddress(start) + L"-" + m_HexView.FormatAddress(end);
	const std::wstring selected = length == 1 ? L"1 byte selected" : std::format(L"{} bytes selected", length);
	set_text(m_InspectorTitle, &m_InspectorTitleText, range);
	set_text(m_InspectorSubtitle, &m_InspectorSubtitleText, std::format(L"{} \x00B7 {}", selected, Blocks[static_cast<int>(m_Block)].name));

	// The byte at the start of the selection
	const uint8_t value = data[start];
	const int8_t signed_value = static_cast<int8_t>(value);
	const bool printable = value >= 0x20 && value < 0x7F;

	SetInspectorValue(Field::Unsigned, std::format(L"{}  (${:02X})", value, value));
	SetInspectorValue(Field::Signed, signed_value > 0 ? std::format(L"+{}", signed_value) : std::format(L"{}", signed_value));
	SetInspectorValue(Field::Binary, std::format(L"{:04b} {:04b}", value >> 4, value & 0x0F));
	SetInspectorValue(Field::Ascii, printable ? std::format(L"'{}'", static_cast<wchar_t>(value)) : L"Not printable");
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

bool MemoryWindow::WriteBlock(Block block, size_t offset, uint8_t value)
{
	Emulator* emulator = m_Application->GetEmulator();
	if (emulator == nullptr || !emulator->IsRunning() || emulator->GetCartridge() == nullptr)
	{
		return false;
	}

	switch (block)
	{
		case Block::Rom:
			emulator->GetCartridge()->PokeRom(offset, value);
			break;

		case Block::VideoRam:
			emulator->GetPpu()->PokeVideoRam(offset, value);
			break;

		case Block::CartridgeRam:
			emulator->GetCartridge()->PokeExternalRam(offset, value);
			break;

		case Block::WorkRam:
			emulator->GetRam()->PokeWorkRam(offset, value);
			break;

		case Block::Oam:
			emulator->GetPpu()->PokeOam(offset, value);
			break;

		case Block::HighRam:
			emulator->GetRam()->PokeHighRam(offset, value);
			break;

		default:
			return false;
	}

	// Paused, the copy on screen takes the edit too
	if (m_Paused && block == m_Block && offset < m_PausedData.size())
	{
		m_PausedData[offset] = value;
	}

	UpdateInspector();
	return true;
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

	// Another game while paused: freeze that one instead
	if (m_Paused)
	{
		TakeSnapshot();
	}
}

void MemoryWindow::UpdateStatus()
{
	Emulator* emulator = m_Application->GetEmulator();
	const bool running = emulator != nullptr && emulator->IsRunning();
	const std::wstring live = !running ? L"No game loaded" : (m_Paused ? L"Paused" : L"Live, updates every frame");

	// Only set when changed, as this is called every frame
	auto set_part = [this](int part, const std::wstring& text)
	{
		wchar_t current[256] = {};
		if (LOWORD(SendMessageW(m_StatusBar, SB_GETTEXTLENGTHW, part, 0)) < 256)
		{
			SendMessageW(m_StatusBar, SB_GETTEXTW, part, reinterpret_cast<LPARAM>(current));
		}

		if (text != current)
		{
			SendMessageW(m_StatusBar, SB_SETTEXTW, part, reinterpret_cast<LPARAM>(text.c_str()));
		}
	};

	// With no Go to or Find result to show, say whether typing writes
	std::wstring message = m_StatusMessage;
	if (message.empty() && running && !ViewBlock(m_Block).empty())
	{
		message = m_Block == Block::Rom
			? L"Type over a byte to write it. Edits change the loaded ROM, not the file."
			: L"Type over a byte to write it";
	}

	set_part(0, message);
	set_part(1, live);
}

void MemoryWindow::SetStatusMessage(const std::wstring& message)
{
	m_StatusMessage = message;
	UpdateStatus();
}

bool MemoryWindow::ParseAddress(const std::wstring& text, Block* block, size_t* offset, std::wstring* error) const
{
	auto fail = [error](const std::wstring& message)
	{
		*error = message;
		return false;
	};

	const std::wstring input = WithoutSpaces(text);
	if (input.empty())
	{
		return fail(L"Type an address to go to, e.g. $C040 or 01:D040");
	}

	// An optional bank before a colon, as the address column shows it
	size_t bank = 0;
	bool has_bank = false;
	size_t address = 0;

	const size_t colon = input.find(L':');
	if (colon != std::wstring::npos)
	{
		if (!ParseHex(input.substr(0, colon), 3, &bank))
		{
			return fail(std::format(L"\"{}\" is not a bank and address, e.g. 01:D040", text));
		}

		has_bank = true;
	}

	if (!ParseHex(colon == std::wstring::npos ? input : input.substr(colon + 1), 4, &address))
	{
		return fail(std::format(L"\"{}\" is not an address, e.g. $C040", text));
	}

	// Echo RAM mirrors work RAM
	const bool echo = address >= 0xE000 && address <= 0xFDFF;
	if (echo)
	{
		address -= 0x2000;
	}

	Emulator* emulator = m_Application->GetEmulator();
	if (emulator == nullptr || !emulator->IsRunning())
	{
		return fail(L"No game loaded");
	}

	const bool colour = emulator->GetDisplay()->IsColour();

	if (address <= 0x3FFF)
	{
		if (has_bank && bank != 0)
		{
			return fail(L"$0000-$3FFF is always ROM bank 00");
		}

		*block = Block::Rom;
		*offset = address;
	}
	else if (address <= 0x7FFF)
	{
		const size_t rom_bank = has_bank ? bank : 1;
		if (rom_bank == 0)
		{
			return fail(L"ROM bank 00 is at $0000-$3FFF, the switchable banks start at 01");
		}

		*block = Block::Rom;
		*offset = rom_bank * 0x4000 + (address - 0x4000);
	}
	else if (address <= 0x9FFF)
	{
		const size_t vram_bank = has_bank ? bank : (colour ? (emulator->GetPpu()->GetVideoRamBank() & 1) : 0);
		*block = Block::VideoRam;
		*offset = vram_bank * 0x2000 + (address - 0x8000);
	}
	else if (address <= 0xBFFF)
	{
		*block = Block::CartridgeRam;
		*offset = (has_bank ? bank : 0) * 0x2000 + (address - 0xA000);
	}
	else if (address <= 0xCFFF)
	{
		if (has_bank && bank != 0)
		{
			return fail(L"$C000-$CFFF is always work RAM bank 00");
		}

		*block = Block::WorkRam;
		*offset = address - 0xC000;
	}
	else if (address <= 0xDFFF)
	{
		const size_t wram_bank = has_bank ? bank : emulator->GetRam()->GetWorkRamBank();
		if (wram_bank == 0)
		{
			return fail(L"Work RAM bank 00 is at $C000-$CFFF, the switchable banks start at 01");
		}

		*block = Block::WorkRam;
		*offset = wram_bank * 0x1000 + (address - 0xD000);
	}
	else if (address <= 0xFE9F)
	{
		if (has_bank)
		{
			return fail(L"OAM has no banks");
		}

		*block = Block::Oam;
		*offset = address - 0xFE00;
	}
	else if (address <= 0xFEFF)
	{
		return fail(L"$FEA0-$FEFF is not usable memory");
	}
	else if (address <= 0xFF7F)
	{
		return fail(L"I/O registers are not shown in this window");
	}
	else if (address <= 0xFFFE)
	{
		if (has_bank)
		{
			return fail(L"High RAM has no banks");
		}

		*block = Block::HighRam;
		*offset = address - 0xFF80;
	}
	else
	{
		return fail(L"The interrupt enable register is not shown in this window");
	}

	const std::span<const uint8_t> data = ViewBlock(*block);
	const wchar_t* name = Blocks[static_cast<int>(*block)].name;
	if (data.empty())
	{
		return fail(*block == Block::CartridgeRam ? L"This cartridge has no RAM" : std::format(L"{} is empty", name));
	}

	if (*offset >= data.size())
	{
		return fail(std::format(L"\"{}\" is past the end of {}", text, name));
	}

	// On success, a note for the status bar
	*error = echo ? L"Echo RAM mirrors work RAM" : L"";
	return true;
}

bool MemoryWindow::ParseFind(const std::wstring& text, bool hex, std::vector<uint8_t>* bytes, std::wstring* error) const
{
	bytes->clear();

	if (hex)
	{
		// Spaces, commas and "$" are allowed between bytes
		std::wstring digits;
		for (wchar_t c : text)
		{
			if (!iswspace(c) && c != L',' && c != L'$')
			{
				digits += c;
			}
		}

		if (digits.empty())
		{
			*error = L"Type the bytes to find, e.g. 3E C4";
			return false;
		}

		for (wchar_t c : digits)
		{
			if (HexDigit(c) < 0)
			{
				*error = std::format(L"\"{}\" is not hex bytes. Choose Text to find text.", text);
				return false;
			}
		}

		if (digits.size() % 2 != 0)
		{
			*error = L"Each byte needs two hex digits, e.g. 3E C4";
			return false;
		}

		for (size_t i = 0; i < digits.size(); i += 2)
		{
			bytes->push_back(static_cast<uint8_t>(HexDigit(digits[i]) * 16 + HexDigit(digits[i + 1])));
		}

		return true;
	}

	if (text.empty())
	{
		*error = L"Type the text to find";
		return false;
	}

	for (wchar_t c : text)
	{
		if (c < 0x20 || c > 0x7E)
		{
			*error = L"Text can only use ASCII characters";
			return false;
		}

		bytes->push_back(static_cast<uint8_t>(c));
	}

	return true;
}

void MemoryWindow::GoTo()
{
	Block block = m_Block;
	size_t offset = 0;
	std::wstring note;
	if (!ParseAddress(GetText(m_EditGoTo), &block, &offset, &note))
	{
		SetStatusMessage(note);
		return;
	}

	if (block != m_Block)
	{
		SelectBlock(block);
	}

	m_HexView.SelectRange(offset, offset, false);
	UpdateInspector();

	// Echo RAM says where it went instead
	SetStatusMessage(note.empty() ? L"" : std::format(L"{}, showing {}", note, m_HexView.FormatAddress(offset)));
	SetFocus(m_HexView.GetHwnd());
}

void MemoryWindow::Find(bool backwards)
{
	std::vector<uint8_t> needle;
	std::wstring error;
	const bool hex = Button_GetCheck(m_ButtonHex) == BST_CHECKED;
	if (!ParseFind(GetText(m_EditFind), hex, &needle, &error))
	{
		SetStatusMessage(error);
		return;
	}

	const std::span<const uint8_t> data = ViewBlock(m_Block);
	const wchar_t* name = Blocks[static_cast<int>(m_Block)].name;
	if (data.empty() || needle.size() > data.size())
	{
		SetStatusMessage(std::format(L"Not found in {}", name));
		return;
	}

	// Positions in [from, to) where the bytes start: the first, or the last going backwards
	const size_t last_start = data.size() - needle.size();
	auto search = [&](size_t from, size_t to, bool last) -> std::optional<size_t>
	{
		to = (std::min)(to, last_start + 1);
		if (from >= to)
		{
			return std::nullopt;
		}

		const auto begin = data.begin() + from;
		const auto end = data.begin() + (to - 1 + needle.size());
		const auto found = last ? std::find_end(begin, end, needle.begin(), needle.end()) : std::search(begin, end, needle.begin(), needle.end());
		if (found == end)
		{
			return std::nullopt;
		}

		return static_cast<size_t>(found - data.begin());
	};

	// On from the selection, wrapping round the block
	const size_t start = m_HexView.HasSelection() ? m_HexView.GetSelectionStart() : 0;
	bool wrapped = false;
	std::optional<size_t> found = backwards ? search(0, start, true) : search(start + 1, data.size(), false);
	if (!found)
	{
		found = backwards ? search(start, data.size(), true) : search(0, start + 1, false);
		wrapped = found.has_value();
	}

	if (!found)
	{
		SetStatusMessage(std::format(L"Not found in {}", name));
		return;
	}

	m_HexView.SelectRange(*found, *found + needle.size() - 1, !hex);
	UpdateInspector();

	std::wstring message = std::format(L"Found at {}", m_HexView.FormatAddress(*found));
	if (wrapped)
	{
		message += backwards ? L", wrapped round to the end" : L", wrapped round to the start";
	}

	SetStatusMessage(message);
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
	m_HexView.SelectionChanged = [this]()
	{
		UpdateInspector();
		SetStatusMessage(L"");
	};

	m_HexView.WriteRequested = [this](size_t offset, uint8_t value)
	{
		return WriteBlock(m_Block, offset, value);
	};

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

	// Go to an address, and find bytes or text
	m_LabelGoTo = create(WC_STATICW, L"Go to", SS_CENTERIMAGE, 0, m_Font);
	m_EditGoTo = create(WC_EDITW, L"", WS_TABSTOP | ES_AUTOHSCROLL, m_ControlGoToId, m_MonoFont);
	Edit_SetCueBannerText(m_EditGoTo, L"$C040");

	m_LabelFind = create(WC_STATICW, L"Find", SS_CENTERIMAGE, 0, m_Font);
	m_EditFind = create(WC_EDITW, L"", WS_TABSTOP | ES_AUTOHSCROLL, m_ControlFindId, m_Font);
	Edit_SetCueBannerText(m_EditFind, L"Hex bytes or text");

	// The field supplies the padding
	for (HWND edit : { m_EditGoTo, m_EditFind })
	{
		SendMessageW(edit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, 0);
	}

	// What Find looks for, hex bytes or text: push-like radio buttons act as a segmented control
	m_ButtonHex = create(WC_BUTTONW, L"Hex", WS_TABSTOP | WS_GROUP | BS_AUTORADIOBUTTON | BS_PUSHLIKE, m_ControlHexId, m_Font);
	m_ButtonText = create(WC_BUTTONW, L"Text", BS_AUTORADIOBUTTON | BS_PUSHLIKE, m_ControlTextId, m_Font);
	Button_SetCheck(m_ButtonHex, BST_CHECKED);

	// WS_GROUP ends the Hex/Text radio group
	m_CheckHighlight = create(WC_BUTTONW, L"Highlight changes", WS_TABSTOP | WS_GROUP | BS_AUTOCHECKBOX, m_ControlHighlightId, m_Font);
	Button_SetCheck(m_CheckHighlight, BST_CHECKED);

	// Pause updates, on the right
	m_ButtonPause = create(WC_BUTTONW, L"Pause updates", WS_TABSTOP | BS_AUTOCHECKBOX | BS_PUSHLIKE, m_ControlPauseId, m_Font);
}

void MemoryWindow::LayoutCommandBar(int width)
{
	const int button_height = 26;
	const int button_y = (m_CommandBarHeight - button_height) / 2;
	const int group_gap = 20;

	int x = 12;
	auto label = [&](HWND control, int gap)
	{
		const int w = MeasureText(m_Font, GetText(control));
		MoveWindow(control, x, button_y, w, button_height, TRUE);
		x += w + gap;
	};

	// A field as tall as the buttons, its edit one line of text high in the middle
	auto field = [&](HWND edit, HFONT font, int width, RECT* rect)
	{
		*rect = { x, button_y, x + width, button_y + button_height };
		const int padding = 8;
		const int text_height = FontHeight(font);
		MoveWindow(edit, x + padding, button_y + (button_height - text_height) / 2, width - padding * 2, text_height, TRUE);
		x += width + group_gap;
	};

	label(m_LabelGoTo, 6);
	field(m_EditGoTo, m_MonoFont, 120, &m_GoToField);

	label(m_LabelFind, 6);
	field(m_EditFind, m_Font, 180, &m_FindField);

	for (HWND button : { m_ButtonHex, m_ButtonText })
	{
		const int w = MeasureText(m_Font, GetText(button)) + 24;
		MoveWindow(button, x, button_y, w, button_height, TRUE);
		x += w + 2;
	}

	x += group_gap;
	MoveWindow(m_CheckHighlight, x, button_y, MeasureText(m_Font, GetText(m_CheckHighlight)) + 24, button_height, TRUE);

	const int pause_width = MeasureText(m_Font, GetText(m_ButtonPause)) + 28;
	MoveWindow(m_ButtonPause, width - 12 - pause_width, button_y, pause_width, button_height, TRUE);
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

	m_InspectorTitle = create_static(m_ControlInspectorTitleId, L"", m_TitleFont);
	m_InspectorSubtitle = create_static(m_ControlInspectorSubtitleId, L"", m_Font);

	for (int i = 0; i < m_FieldCount; ++i)
	{
		m_InspectorLabels[i] = create_static(m_ControlInspectorLabelId + i, FieldLabels[i], m_Font);
		m_InspectorValues[i] = create_static(m_ControlInspectorValueId + i, L"", m_MonoFont);
	}

	m_LegendHeading = create_static(m_ControlLegendHeadingId, L"Legend", m_BoldFont);
	for (int i = 0; i < m_LegendCount; ++i)
	{
		m_LegendLabels[i] = create_static(m_ControlLegendLabelId + i, LegendLabels[i], m_Font);
	}
}

void MemoryWindow::LayoutInspector(int left, int top)
{
	const int width = m_InspectorWidth - 32;
	const int key_width = 110;
	const int row_height = 26;
	const int text_height = 18;
	const int section_gap = 16;

	// Heading and the line under it
	int y = top;
	MoveWindow(m_InspectorTitle, left, y, width, 26, TRUE);
	y += 28;
	MoveWindow(m_InspectorSubtitle, left, y, width, text_height, TRUE);
	y += text_height + section_gap;

	// Table rows, each with a line along its bottom
	for (int i = 0; i < m_FieldCount; ++i)
	{
		const int text_y = y + (row_height - text_height) / 2;
		MoveWindow(m_InspectorLabels[i], left, text_y, key_width, text_height, TRUE);
		MoveWindow(m_InspectorValues[i], left + key_width, text_y, width - key_width, text_height, TRUE);
		m_InspectorRowLines[i] = { left, y + row_height - 1, left + width, y + row_height };
		y += row_height;
	}

	y += section_gap;

	// Legend: a swatch and what it means
	MoveWindow(m_LegendHeading, left, y, width, text_height, TRUE);
	y += text_height + 6;

	for (int i = 0; i < m_LegendCount; ++i)
	{
		m_LegendSwatches[i] = { left, y + 2, left + 18, y + 16 };
		MoveWindow(m_LegendLabels[i], left + 26, y, width - 26, text_height, TRUE);
		y += 22;
	}
}

void MemoryWindow::PaintInspector(HDC hdc)
{
	HBRUSH dc_brush = reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH));

	for (const RECT& line : m_InspectorRowLines)
	{
		SetDCBrushColor(hdc, InspectorRowLineColour);
		FillRect(hdc, &line, dc_brush);
	}

	// Changed, as the hex view's highlight
	SetDCBrushColor(hdc, LegendChangedColour);
	FillRect(hdc, &m_LegendSwatches[0], dc_brush);

	// Selection, with the design's outline
	SetDCBrushColor(hdc, LegendSelectionColour);
	FillRect(hdc, &m_LegendSwatches[1], dc_brush);
	SetDCBrushColor(hdc, LegendSelectionOutline);
	FrameRect(hdc, &m_LegendSwatches[1], dc_brush);

	// A dimmed "00"
	RECT zero = m_LegendSwatches[2];
	zero.right += 4;
	const int saved = SaveDC(hdc);
	SelectObject(hdc, m_MonoFont);
	SetBkMode(hdc, TRANSPARENT);
	SetTextColor(hdc, LegendZeroColour);
	DrawTextW(hdc, L"00", -1, &zero, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	RestoreDC(hdc, saved);
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

	// Go to and Find results on the left, live or not on the right
	int parts[] = { (std::max)(0, static_cast<int>(client.right) - 220), -1 };
	SendMessageW(m_StatusBar, SB_SETPARTS, 2, reinterpret_cast<LPARAM>(parts));

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

void MemoryWindow::PaintField(HDC hdc, const RECT& field, HWND edit)
{
	if (IsRectEmpty(&field))
	{
		return;
	}

	const int saved = SaveDC(hdc);
	SelectObject(hdc, GetStockObject(DC_BRUSH));
	SelectObject(hdc, GetStockObject(DC_PEN));
	SetDCBrushColor(hdc, GetSysColor(COLOR_WINDOW));
	SetDCPenColor(hdc, GetFocus() == edit ? FieldFocusColour : FieldBorderColour);
	RoundRect(hdc, field.left, field.top, field.right, field.bottom, 8, 8);
	RestoreDC(hdc, saved);
}

HWND MemoryWindow::FieldEditAt(POINT point) const
{
	if (PtInRect(&m_GoToField, point))
	{
		return m_EditGoTo;
	}

	if (PtInRect(&m_FindField, point))
	{
		return m_EditFind;
	}

	return NULL;
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

	PaintField(hdc, m_GoToField, m_EditGoTo);
	PaintField(hdc, m_FindField, m_EditFind);

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

	PaintInspector(hdc);
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

		case WM_LBUTTONDOWN:
		{
			// A click in a field's padding focuses its edit
			const POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			if (HWND edit = FieldEditAt(point))
			{
				SetFocus(edit);
				return 0;
			}

			break;
		}

		case WM_SETCURSOR:
			// Text cursor over a field's padding too
			if (reinterpret_cast<HWND>(wParam) == hwnd && LOWORD(lParam) == HTCLIENT)
			{
				POINT point;
				GetCursorPos(&point);
				ScreenToClient(hwnd, &point);
				if (FieldEditAt(point) != NULL)
				{
					SetCursor(LoadCursor(NULL, IDC_IBEAM));
					return TRUE;
				}
			}

			break;

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
			// Inspector text sits on the inspector's colour: the subtitle grey, keys and legend dark grey
			const int id = GetDlgCtrlID(reinterpret_cast<HWND>(lParam));
			if (id >= m_ControlInspectorTitleId && id < m_ControlInspectorValueId + m_FieldCount)
			{
				COLORREF colour = InspectorTextColour;
				if (id == m_ControlInspectorSubtitleId)
				{
					colour = InspectorLabelColour;
				}
				else if ((id >= m_ControlLegendLabelId && id < m_ControlLegendLabelId + m_LegendCount) ||
					(id >= m_ControlInspectorLabelId && id < m_ControlInspectorLabelId + m_FieldCount))
				{
					colour = InspectorKeyColour;
				}

				HDC hdc = reinterpret_cast<HDC>(wParam);
				SetTextColor(hdc, colour);
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
			if (control == m_LabelGoTo || control == m_LabelFind || control == m_CheckHighlight)
			{
				HDC hdc = reinterpret_cast<HDC>(wParam);
				SetBkMode(hdc, TRANSPARENT);
				SetTextColor(hdc, control == m_CheckHighlight ? GetSysColor(COLOR_WINDOWTEXT) : CommandLabelColour);
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

			if (LOWORD(wParam) == m_ControlHighlightId && HIWORD(wParam) == BN_CLICKED)
			{
				m_HexView.SetHighlightChanges(Button_GetCheck(m_CheckHighlight) == BST_CHECKED);
				return 0;
			}

			if (LOWORD(wParam) == m_ControlPauseId && HIWORD(wParam) == BN_CLICKED)
			{
				SetPaused(Button_GetCheck(m_ButtonPause) == BST_CHECKED);
				return 0;
			}

			// A field's border turns blue while its edit has focus
			if ((LOWORD(wParam) == m_ControlGoToId || LOWORD(wParam) == m_ControlFindId) &&
				(HIWORD(wParam) == EN_SETFOCUS || HIWORD(wParam) == EN_KILLFOCUS))
			{
				InvalidateRect(hwnd, LOWORD(wParam) == m_ControlGoToId ? &m_GoToField : &m_FindField, TRUE);
				return 0;
			}

			// Enter: the dialog manager sends IDOK, as there is no default button
			if (LOWORD(wParam) == IDOK)
			{
				HWND focus = GetFocus();
				if (focus == m_EditGoTo)
				{
					GoTo();
				}
				else if (focus == m_EditFind)
				{
					// Shift+Enter finds the previous match
					Find((GetKeyState(VK_SHIFT) & 0x8000) != 0);
				}

				return 0;
			}

			if (LOWORD(wParam) == m_ControlBlockListId && HIWORD(wParam) == LBN_SELCHANGE)
			{
				const LRESULT index = SendMessageW(m_BlockList, LB_GETCURSEL, 0, 0);
				if (index >= 0 && index < static_cast<LRESULT>(Block::Count))
				{
					SelectBlock(static_cast<Block>(index));
					SetStatusMessage(L"");
				}

				return 0;
			}

			break;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}
