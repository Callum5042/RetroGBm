#include "CartridgeInfoWindow.h"

// Set theme
// https://docs.microsoft.com/en-gb/windows/win32/controls/cookbook-overview?redirectedfrom=MSDN
#pragma comment(linker,"\"/manifestdependency:type='win32' name = 'Microsoft.Windows.Common-Controls' version = '6.0.0.0' processorArchitecture = '*' publicKeyToken = '6595b64144ccf1df' language = '*'\"")

#define ID_BUTTON 450

void CartridgeInfoWindow::Create(const std::string& title, int width, int height)
{
	Window::Create(title, width, height);

	HWND m_Hwnd = this->GetHwnd();

	// Load font
	HDC hdc = GetDC(NULL);
	long lfHeight = -MulDiv(12, GetDeviceCaps(hdc, LOGPIXELSY), 72);
	ReleaseDC(NULL, hdc);

	m_Font = CreateFont(lfHeight, 0, 0, 0, 0, FALSE, 0, 0, 0, 0, 0, 0, 0, L"Arial");

	// Group box
	HWND groupbox = CreateWindowEx(WS_EX_WINDOWEDGE,
								   L"BUTTON",  // Predefined class; Unicode assumed 
								   L"Group",      // Button text 
								   BS_GROUPBOX | WS_GROUP | WS_CHILD | BS_DEFPUSHBUTTON | WS_VISIBLE,  // Styles 
								   10,         // x position 
								   0,          // y position 
								   width - 20,        // Button width
								   height - 10,        // Button height
								   m_Hwnd,     // Parent window
								   NULL,
								   (HINSTANCE)GetWindowLongPtr(m_Hwnd, GWLP_HINSTANCE),
								   NULL);      // Pointer not needed.

	SendMessage(groupbox, WM_SETFONT, (WPARAM)m_Font, TRUE);

	// Draw text
	PAINTSTRUCT ps;
	hdc = BeginPaint(this->GetHwnd(), &ps);
	FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

	RECT rect;
	GetClientRect(groupbox, &rect);

	rect.left = 20;
	rect.top = 20;

	DrawText(hdc, L"Testing", -1, &rect, DT_SINGLELINE | DT_NOCLIP);

	// Create things
	m_BrushBackground = CreateSolidBrush(RGB(255, 255, 255));
}

LRESULT CartridgeInfoWindow::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_CTLCOLORSTATIC:
		{
			HDC hdc = (HDC)wParam;
			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, RGB(255, 255, 255));
			return (LONG_PTR)(m_BrushBackground);
		}
	}

	return Window::HandleMessage(hwnd, msg, wParam, lParam);
}