#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <dwmapi.h>
#include <iostream>
#include <CommCtrl.h>
#include "Manifest.h"
#include "FileFinder.h"



#pragma comment(lib, "dwmapi.lib")
#define ID_EDITBOX 1001
#define _WIN32_WINNT 0x0601

const wchar_t CLASS_NAME[] = L"SearchBar";

HBITMAP g_hBackground = NULL;
HWND g_hEdit = NULL;



LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    INITCOMMONCONTROLSEX icex;

	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icex);

    switch (msg)
    {
    case WM_CREATE:
    {

        g_hEdit = CreateWindowExW(
            0,
            L"EDIT",
            L"",
            WS_BORDER | WS_CHILD | WS_VISIBLE | ES_LEFT  ,
            0, 0, 800, 28,
            hwnd,
            (HMENU)ID_EDITBOX,
            ((LPCREATESTRUCT)lParam)->hInstance,
            NULL
        );

        
        WCHAR placeholderText[] = L"Search anything bozo...";
        SendMessage(g_hEdit, EM_SETCUEBANNER, 1, (LPARAM)placeholderText);

        

        return 0;
    }

    case WM_COMMAND:
    {
        if (LOWORD(wParam) == ID_EDITBOX && HIWORD(wParam) == EN_CHANGE) {

            int len = GetWindowTextLengthW(g_hEdit);
            std::wstring text(len + 1, L'\0');

            GetWindowTextW(g_hEdit, &text[0], len + 1);
            text.resize(len);

            std::wcout << text << std::endl;
        }
    }


    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        
        if (g_hBackground)
        {
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, g_hBackground);

            BITMAP bm;
            GetObject(g_hBackground, sizeof(bm), &bm);

            StretchBlt(
                hdc,
                0, 0,
                ps.rcPaint.right,
                ps.rcPaint.bottom,
                memDC,
                0, 0,
                bm.bmWidth,
                bm.bmHeight,
                SRCCOPY
            );

            SelectObject(memDC, oldBmp);
            DeleteDC(memDC);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        if (g_hBackground)
            DeleteObject(g_hBackground);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    PWSTR,
    int nCmdShow)
{
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    freopen("CONIN$", "r", stdin);

    std::cout << "Hello World\n";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"My Win32 App",
        WS_POPUP | WS_BORDER | WS_EX_APPWINDOW,
        (screenWidth / 2) - 750/2, (screenHeight / 2) - 280/2,
        750, 280,
        NULL,
        NULL,
        hInstance,
        NULL
    );
    
    if (!hwnd)
        return 0;
    ShowWindow(hwnd, nCmdShow);
    
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}
