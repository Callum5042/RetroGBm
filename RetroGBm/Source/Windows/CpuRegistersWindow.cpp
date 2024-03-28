#include "Windows/CpuRegistersWindow.h"
#include "Application.h"

#include "Utilities/Utilities.h"
#include <RetroGBm/Emulator.h>
#include <RetroGBm/Cpu.h>

#include <string>
#include <chrono>
#include <thread>

// Set theme
// https://docs.microsoft.com/en-gb/windows/win32/controls/cookbook-overview?redirectedfrom=MSDN
#pragma comment(linker,"\"/manifestdependency:type='win32' name = 'Microsoft.Windows.Common-Controls' version = '6.0.0.0' processorArchitecture = '*' publicKeyToken = '6595b64144ccf1df' language = '*'\"")

namespace
{
	static CpuRegisterWindow* GetWindow(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		CpuRegisterWindow* window = nullptr;
		if (uMsg == WM_NCCREATE)
		{
			CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
			window = reinterpret_cast<CpuRegisterWindow*>(pCreate->lpCreateParams);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
		}
		else
		{
			window = reinterpret_cast<CpuRegisterWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		}

		return window;
	}

	static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		CpuRegisterWindow* window = GetWindow(hwnd, msg, wParam, lParam);
		if (window == nullptr)
		{
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}

		return window->HandleMessage(hwnd, msg, wParam, lParam);
	}

	static std::wstring ConvertUint16ToHexString(uint16_t value)
	{
		std::wstringstream stream;
		stream << std::uppercase << std::hex << std::setw(4) << std::setfill(L'0') << static_cast<int>(value);
		return stream.str();
	}

	static std::wstring ConvertUint8ToHexString(uint8_t value)
	{
		std::wstringstream stream;
		stream << std::uppercase << std::hex << std::setw(2) << std::setfill(L'0') << static_cast<int>(value);
		return stream.str();
	}
}

CpuRegisterWindow::CpuRegisterWindow()
{
}

CpuRegisterWindow::~CpuRegisterWindow()
{
	// Cleanup edit boxes
	for (auto& text : m_TextBoxes)
	{
		DestroyWindow(text.second);
	}

	// Cleanup window
	this->Destroy();
}

void CpuRegisterWindow::Create()
{
	const std::string title = "CPU Registers";
	const int width = 250;
	const int height = 200;

	WindowCreate(title, width, height);

	// Create font
	{
		HDC hdc = GetDC(NULL);
		long lfHeight = -MulDiv(12, GetDeviceCaps(hdc, LOGPIXELSY), 72);
		ReleaseDC(NULL, hdc);

		m_Font = CreateFont(lfHeight, 0, 0, 0, 0, FALSE, 0, 0, 0, 0, 0, 0, 0, L"Arial");
	}
	// Group box
	m_BrushBackground = CreateSolidBrush(RGB(255, 255, 255));

	m_GroupBox = CreateWindowEx(WS_EX_WINDOWEDGE,
		L"BUTTON",  // Predefined class; Unicode assumed 
		L"Registers",      // Button text 
		BS_GROUPBOX | WS_GROUP | WS_CHILD | BS_DEFPUSHBUTTON | WS_VISIBLE,  // Styles 
		10,         // x position 
		0,          // y position 
		width - 20,        // Button width
		height - 10,        // Button height
		m_Hwnd,     // Parent window
		NULL,
		(HINSTANCE)GetWindowLongPtr(m_Hwnd, GWLP_HINSTANCE),
		NULL);      // Pointer not needed.

	SendMessage(m_GroupBox, WM_SETFONT, (WPARAM)m_Font, TRUE);

	// CPU Details
	/*
		PC: 0x10	SP: 0x10
		A: 0x10		F: 0x20
		B: 0x30		C: 0x40
		D: 0x30		E: 0x40
		H: 0x30		L: 0x40

		Zero: 1		Sub: 0
		Carry: 1	Half: 0
	*/

	// Draw text
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(m_Hwnd, &ps);
	FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
	SelectObject(hdc, m_Font);

	// Calculate position
	RECT rect;
	GetClientRect(m_GroupBox, &rect);
	rect.left = 20;
	rect.top = 26;

	// Data
	std::vector<std::string> text;
	text.push_back("PC");
	text.push_back("SP");
	text.push_back("A");
	text.push_back("F");
	text.push_back("B");
	text.push_back("C");
	text.push_back("D");
	text.push_back("E");
	text.push_back("H");
	text.push_back("L");

	int y = 0;
	for (int i = 0; i < text.size(); i += 2)
	{
		rect.top = 26 + (y * 32);

		// Column 1
		std::wstring text1 = Utilities::ConvertToWString(std::string(text[i] + ": "));
		DrawText(hdc, text1.c_str(), -1, &rect, DT_SINGLELINE | DT_NOCLIP);

		HWND textbox1 = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_DISABLED, rect.left + 40, rect.top, 56, 24, m_Hwnd, NULL, (HINSTANCE)GetWindowLongPtr(m_Hwnd, GWLP_HINSTANCE), NULL);
		SendMessage(textbox1, WM_SETFONT, (WPARAM)m_Font, TRUE);

		m_TextBoxes[text[i]] = textbox1;

		// Column 2
		RECT col2 = rect;
		col2.left = 130;

		std::wstring text2 = Utilities::ConvertToWString(std::string(text[i + 1] + ": "));
		DrawText(hdc, text2.c_str(), -1, &col2, DT_SINGLELINE | DT_NOCLIP);

		HWND textbox2 = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_DISABLED, col2.left + 40, col2.top, 56, 24, m_Hwnd, NULL, (HINSTANCE)GetWindowLongPtr(m_Hwnd, GWLP_HINSTANCE), NULL);
		SendMessage(textbox2, WM_SETFONT, (WPARAM)m_Font, TRUE);

		m_TextBoxes[text[i + 1]] = textbox2;

		y++;
	}
}

void CpuRegisterWindow::Destroy()
{
	HINSTANCE hInstance = GetModuleHandle(NULL);
	UnregisterClass(m_RegisterClassName.c_str(), hInstance);

	DestroyWindow(m_Hwnd);
}

LRESULT CpuRegisterWindow::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_CLOSE:
			Destroy();
			Application::Instance->GetMainWindow()->ToggleCpuRegistersWindowMenuItem(false);
			return 0;

		case WM_CTLCOLORSTATIC:
		{
			HDC hdc = (HDC)wParam;
			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, RGB(255, 255, 255));
			return (LONG_PTR)(m_BrushBackground);
		}
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void CpuRegisterWindow::WindowCreate(const std::string& title, int width, int height)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);

	// Convert to wstring
	const std::wstring window_title = Utilities::ConvertToWString(title);
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
	HWND parent_window = Application::Instance->GetMainWindow()->GetHwnd();

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

void CpuRegisterWindow::Update()
{
	Emulator* emulator = Application::Instance->GetEmulator();
	if (emulator != nullptr && emulator->IsRunning())
	{
		Cpu* cpu = emulator->GetCpu();
		SetWindowText(m_TextBoxes["PC"], ConvertUint16ToHexString(cpu->ProgramCounter).c_str());
		SetWindowText(m_TextBoxes["SP"], ConvertUint16ToHexString(cpu->StackPointer).c_str());
		SetWindowText(m_TextBoxes["A"], ConvertUint8ToHexString(cpu->GetRegister(RegisterType8::REG_A)).c_str());
		SetWindowText(m_TextBoxes["F"], ConvertUint8ToHexString(cpu->GetRegister(RegisterType8::REG_F)).c_str());
		SetWindowText(m_TextBoxes["B"], ConvertUint8ToHexString(cpu->GetRegister(RegisterType8::REG_B)).c_str());
		SetWindowText(m_TextBoxes["C"], ConvertUint8ToHexString(cpu->GetRegister(RegisterType8::REG_C)).c_str());
		SetWindowText(m_TextBoxes["D"], ConvertUint8ToHexString(cpu->GetRegister(RegisterType8::REG_D)).c_str());
		SetWindowText(m_TextBoxes["E"], ConvertUint8ToHexString(cpu->GetRegister(RegisterType8::REG_E)).c_str());
		SetWindowText(m_TextBoxes["H"], ConvertUint8ToHexString(cpu->GetRegister(RegisterType8::REG_H)).c_str());
		SetWindowText(m_TextBoxes["L"], ConvertUint8ToHexString(cpu->GetRegister(RegisterType8::REG_L)).c_str());
	}
}

void CpuRegisterWindow::Clear()
{
	SetWindowText(m_TextBoxes["PC"], L"");
	SetWindowText(m_TextBoxes["SP"], L"");
	SetWindowText(m_TextBoxes["A"], L"");
	SetWindowText(m_TextBoxes["F"], L"");
	SetWindowText(m_TextBoxes["B"], L"");
	SetWindowText(m_TextBoxes["C"], L"");
	SetWindowText(m_TextBoxes["D"], L"");
	SetWindowText(m_TextBoxes["E"], L"");
	SetWindowText(m_TextBoxes["H"], L"");
	SetWindowText(m_TextBoxes["L"], L"");
}