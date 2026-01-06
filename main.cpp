#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <dwmapi.h>
#include <iostream>
#include <CommCtrl.h>
#include <chrono>
#include <thread>
#include <fstream>
#include "Manifest.h"
#include "FileFinder.h"
#include "IndexFiles.h"


#pragma comment(lib, "dwmapi.lib")
#define ID_EDITBOX 1001
#define _WIN32_WINNT 0x0601

/* UI Vars */
constexpr COLORREF BG_COLOR = RGB(30, 30, 30);
constexpr COLORREF PANEL_COLOR = RGB(38, 38, 38);
constexpr COLORREF TEXT_COLOR = RGB(230, 230, 230);
constexpr COLORREF ACCENT_COLOR = RGB(90, 170, 255);




const wchar_t CLASS_NAME[] = L"SearchBar";
WCHAR placeholderText[] = L"Search anything bozo...";
HBITMAP g_hBackground = NULL;
HWND g_hEdit = NULL;
HWND g_hResult = NULL;
HFONT g_hFont = NULL;
IndexFiles indexer;
FileFinder fileFinder;

/* file handling functionalities ---DO NOT TOUCH--- */



void StartIndexing() {

    std::thread([]{
        fileFinder.BuildIndexAtStartup(L"C:\\Users\\aksha\\OneDrive\\Pictures", indexer);
        fileFinder.BuildIndexAtStartup(L"C:\\Users\\aksha\\OneDrive\\Documents", indexer);
        fileFinder.BuildIndexAtStartup(L"C:\\Users\\aksha\\Downloads", indexer);
        fileFinder.BuildIndexAtStartup(L"C:\\Users\\aksha\\OneDrive\\Desktop", indexer);
        std::wcout << "Indexing is Done, you may test your input\n";
     }).detach();
}

void SearchFile(const std::wstring& target) {
    const auto& index = indexer.getIndex();
    auto iterator = index.find(target);
    if (iterator != index.end()) {
        std::wcout << L"Found: " << iterator->second << L"\n";
        SendMessageW(g_hResult, LB_RESETCONTENT, 0, 0);
        SendMessageW(g_hResult, LB_ADDSTRING, 0, (LPARAM)iterator->second.c_str());
    }
    else {
        SendMessageW(g_hResult, LB_RESETCONTENT, 0, 0);
    }

}




DWORD WINAPI MULTITHREADER(LPVOID lpParam) {
    std::wstring* text = static_cast<std::wstring*>(lpParam);

    FileFinder fileFinderObj = FileFinder();

    auto start = std::chrono::steady_clock::now();

    fileFinderObj.findFilesForMe(L"C:\\", *text);

    auto end = std::chrono::steady_clock::now();

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::wcout << L"Search time: " << ms.count() << L" ms\n";


    delete text;

    return 0;
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
        g_hFont = CreateFontW(
            18, 0, 0, 0,
            FW_NORMAL,
            FALSE, FALSE, FALSE,
            DEFAULT_CHARSET,
            OUT_OUTLINE_PRECIS,
            CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,
            VARIABLE_PITCH,
            L"Segoe UI"
        );



        g_hEdit = CreateWindowExW(
            0,
            L"EDIT",
            L"",
            WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
            20, 20, 710, 36,
            hwnd,
            (HMENU)ID_EDITBOX,
            ((LPCREATESTRUCT)lParam)->hInstance,
            NULL
        );

        SendMessageW(g_hEdit, WM_SETFONT, (WPARAM)g_hFont, TRUE);
        SendMessageW(g_hEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(12, 12));
        
        
        SendMessage(g_hEdit, EM_SETCUEBANNER, 1, (LPARAM)placeholderText);

        g_hResult = CreateWindowExW(
            0,
            L"LISTBOX",
            NULL,
            WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL,
            20, 70, 710, 180,
            hwnd,
            NULL,
            ((LPCREATESTRUCT)lParam)->hInstance,
            NULL
        );

        SendMessageW(g_hResult, WM_SETFONT, (WPARAM)g_hFont, TRUE);



        return 0;
    }

    case WM_COMMAND:
    {
        if (LOWORD(wParam) == ID_EDITBOX && HIWORD(wParam) == EN_CHANGE) {



            int len = GetWindowTextLengthW(g_hEdit);
            std::wstring text(len + 1, L'\0');

            GetWindowTextW(g_hEdit, &text[0], len + 1);
            text.resize(len);


            // heap allocated copy of text
            /*std::wstring* heapText = new std::wstring(text);

            CreateThread(
                NULL,
                0,
                MULTITHREADER,
                heapText,
                0,
                NULL
            );*/

            SearchFile(text);
            


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

    case WM_CTLCOLOREDIT:
    {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, TEXT_COLOR);
        SetBkColor(hdc, PANEL_COLOR);
        static HBRUSH hBrush = CreateSolidBrush(PANEL_COLOR);
        return (INT_PTR)hBrush;
    }

    case WM_CTLCOLORLISTBOX:
    {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, TEXT_COLOR);
        SetBkColor(hdc, PANEL_COLOR);
        static HBRUSH hBrush = CreateSolidBrush(PANEL_COLOR);
        return (INT_PTR)hBrush;
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

    std::cout << "mem allocated to heap : [&text]\n";
    std::cout << "Searching tree...\n";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(BG_COLOR);
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    StartIndexing();
    
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
