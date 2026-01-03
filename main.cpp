#include <windows.h>
#include <dwmapi.h>
#include <iostream>
#include "Manifest.h"
#include <CommCtrl.h>

#pragma comment(lib, "dwmapi.lib")
#define ID_EDITBOX 1001
#define _WIN32_WINNT 0x0601

const wchar_t CLASS_NAME[] = L"SearchBar";

HBITMAP g_hBackground = NULL;
HWND g_hEdit = NULL;

void LoadBackground()
{
    g_hBackground = (HBITMAP)LoadImageW(
        NULL,
        L"makimaaaa.bmp",
        IMAGE_BITMAP,
        0, 0,
        LR_LOADFROMFILE | LR_CREATEDIBSECTION
    );

    if (!g_hBackground)
    {
        DWORD err = GetLastError();
        wchar_t buf[256];
        wsprintfW(buf, L"LoadImage failed. GetLastError = %lu", err);
        MessageBoxW(NULL, buf, L"ERROR", MB_OK);
    }
}
void EnableBlur(HWND hwnd)
{
    DWM_BLURBEHIND bb = {};
    bb.dwFlags = DWM_BB_ENABLE;
    bb.fEnable = TRUE;
    bb.hRgnBlur = NULL;

    DwmEnableBlurBehindWindow(hwnd, &bb);
}

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

        

        return 0;
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
    WCHAR placeholderText[] = L"Search anything bozo...";
    SendMessage(g_hEdit, EM_SETCUEBANNER, 1, (LPARAM)placeholderText);
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}
