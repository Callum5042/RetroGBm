#include "Application.h"
#include <memory>
#pragma comment(lib, "comctl32.lib")

// Useful docs
// https://www.pastraiser.com/cpu/gameboy/gameboy_opcodes.html
// https://rgbds.gbdev.io/docs/v0.5.1/gbz80.7/#HALT
// https://archive.org/details/GameBoyProgManVer1.1/page/n1/mode/2up
// https://gekkio.fi/files/gb-docs/gbctr.pdf

#ifdef _CONSOLE
int main(int argc, char** argv)
#else
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow)
#endif
{
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	// _CrtSetBreakAlloc(972);
#endif

	std::unique_ptr<Application> application = std::make_unique<Application>();
	return application->Start();
}


//#include <windows.h>
//#include <commctrl.h>
//  
//#include <string>
//
//#pragma comment(lib, "comctl32.lib")  // Required for InitCommonControlsEx and ListView
//
//const wchar_t CLASS_NAME[] = L"ListViewExample";
//
//LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
//{
//    // static HWND hListView;
//
//    switch (msg)
//    {
//        case WM_DESTROY:
//            PostQuitMessage(0);
//            return 0;
//    }
//
//    return DefWindowProc(hwnd, msg, wParam, lParam);
//}
//
//int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow)
//{
//    WNDCLASS wc = { };
//    wc.lpfnWndProc = WndProc;
//    wc.hInstance = hInst;
//    wc.lpszClassName = CLASS_NAME;
//    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
//
//    RegisterClass(&wc);
//
//    HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"ListView Example",
//        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 450, 400,
//        NULL, NULL, hInst, NULL);
//
//    // Init common controls
//    INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_LISTVIEW_CLASSES };
//    InitCommonControlsEx(&icex);
//
//    // Create ListView
//    HWND hListView = CreateWindowW(WC_LISTVIEW, L"",
//        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
//        10, 10, 400, 300,
//        hwnd, (HMENU)1, hInst, NULL);
//
//    // Add columns
//    LVCOLUMN lvCol = { 0 };
//    lvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
//
//    const wchar_t* title = L"Title";
//    lvCol.pszText = const_cast<wchar_t*>(title);
//    lvCol.cx = 250;
//    ListView_InsertColumn(hListView, 0, &lvCol);
//
//    const wchar_t* time = L"Time";
//    lvCol.pszText = const_cast<wchar_t*>(time);
//    lvCol.cx = 100;
//    ListView_InsertColumn(hListView, 1, &lvCol);
//
//    // Add sample items
//    std::wstring titles[] = {
//        L"Meeting with Alice", L"Lunch Break", L"Call with Bob", L"Code Review"
//    };
//    std::wstring times[] = {
//        L"10:00 AM", L"12:30 PM", L"3:00 PM", L"4:30 PM"
//    };
//
//    for (int i = 0; i < 4; ++i)
//    {
//        LVITEM lvItem = { 0 };
//        lvItem.mask = LVIF_TEXT;
//        lvItem.iItem = i;
//        lvItem.pszText = (LPWSTR)titles[i].c_str();
//        ListView_InsertItem(hListView, &lvItem);
//
//        ListView_SetItemText(hListView, i, 1, (LPWSTR)times[i].c_str());
//    }
//
//    ShowWindow(hwnd, nCmdShow);
//    UpdateWindow(hwnd);
//
//    MSG msg = { };
//    while (GetMessage(&msg, NULL, 0, 0))
//    {
//        TranslateMessage(&msg);
//        DispatchMessage(&msg);
//    }
//
//    return 0;
//}
